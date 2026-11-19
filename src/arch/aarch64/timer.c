/**
 * @file timer.c
 * @brief hal_timer implementation on the ARM generic timer (aarch64).
 *
 * Uses the EL1 *virtual* timer (CNTV_*, CNTVCT_EL0). Unlike the physical timer
 * it is always accessible from EL1, so no EL2 setup (CNTHCTL_EL2) is needed.
 * Its interrupt is GIC PPI 27, which the architecture fixes, so this file can
 * name the number itself without depending on any platform.
 *
 * The timer works in "compare" mode: it raises its interrupt when the counter
 * reaches the absolute value in CNTV_CVAL_EL0. The periodic tick therefore
 * keeps an absolute deadline and advances it by one period each time, which is
 * what makes it drift-free.
 *
 * The contract and its documentation live in include/hal/hal_timer.h.
 *
 * @ingroup timer
 */

#include "arch/irq.h"
#include <hal/hal_irq.h>
#include <hal/hal_timer.h>
#include <kernel/errno.h>

#include <stddef.h>
#include <stdint.h>

/// @brief Nanoseconds in one second
#define NS_PER_SEC 1000000000ULL
#define TIMER_IRQ_VIRT 27u ///< GIC PPI of the EL1 virtual timer

/* CNTV_CTL_EL0 bits */
/// @brief CNTV_CTL_EL0.ENABLE: timer enabled
#define CTL_ENABLE (1u << 0)
#define CTL_IMASK (1u << 1)   ///< CNTV_CTL_EL0.IMASK. Unused: we always run unmasked
#define CTL_ISTATUS (1u << 2) ///< CNTV_CTL_EL0.ISTATUS: condition met (read-only)

/* ---- state ------------------------------------------------------------- */

/// @brief true once hal_timer_init() has succeeded
static bool s_ready;
static uint64_t s_freq;  ///< Counter frequency in Hz
static uint64_t s_start; ///< Counter value at hal_timer_init()

static void (*s_tick_cb)(void); ///< Tick callback; NULL while the tick is stopped
static uint64_t s_hz;           ///< Requested ticks per second
static uint64_t s_interval;     ///< Whole counter ticks per period
static uint64_t s_rem;          ///< s_freq % s_hz, spread over periods
static uint64_t s_acc;          ///< Fractional accumulator for s_rem
static uint64_t s_deadline;     ///< Next absolute CNTVCT value to fire at

/* ---- system register access ------------------------------------------- */

/**
 * @brief Reads CNTFRQ_EL0, the counter frequency.
 * @return Counter frequency in Hz, as set by the firmware or boot code
 */
static inline uint64_t read_cntfrq()
{
	uint64_t v;
	__asm__ volatile("mrs %0, cntfrq_el0" : "=r"(v));
	return v;
}

/**
 * @brief Reads CNTVCT_EL0, the virtual counter.
 *
 * An @c isb comes first so the read can't happen earlier than where it is written.
 *
 * @return Current counter value, in counter ticks
 */
static inline uint64_t read_cntvct()
{
	uint64_t v;
	/* The isb stops the counter read from being speculated earlier. */
	__asm__ volatile("isb\n\tmrs %0, cntvct_el0" : "=r"(v)::"memory");
	return v;
}

/**
 * @brief Reads CNTV_CTL_EL0, the virtual timer control register.
 * @return Register value; see CTL_ENABLE, CTL_IMASK and CTL_ISTATUS
 */
static inline uint32_t read_cntv_ctl()
{
	uint64_t v;
	__asm__ volatile("mrs %0, cntv_ctl_el0" : "=r"(v));
	return (uint32_t) v;
}

/**
 * @brief Writes CNTV_CTL_EL0, followed by an @c isb.
 * @param v New value; see CTL_ENABLE and CTL_IMASK
 */
static inline void write_cntv_ctl(uint32_t v)
{
	__asm__ volatile("msr cntv_ctl_el0, %0\n\tisb" ::"r"((uint64_t) v) : "memory");
}

/**
 * @brief Writes CNTV_CVAL_EL0, the compare value, followed by an @c isb.
 * @param v Absolute counter value at which the timer condition is met
 */
static inline void write_cntv_cval(uint64_t v)
{
	__asm__ volatile("msr cntv_cval_el0, %0\n\tisb" ::"r"(v) : "memory");
}

/* ---- helpers ----------------------------------------------------------- */

/**
 * @brief Converts counter ticks to nanoseconds.
 *
 * Computes ticks * 1e9 / freq without overflow and without 128-bit division (which
 * would pull in libgcc). Split into whole seconds plus a remainder smaller
 * than one second: rem * 1e9 stays below 2^64 for any freq up to ~18 GHz.
 *
 * @param ticks Number of counter ticks
 * @return The same duration in nanoseconds
 */
static uint64_t ticks_to_ns(uint64_t ticks)
{
	uint64_t sec = ticks / s_freq;
	uint64_t rem = ticks % s_freq;
	return sec * NS_PER_SEC + rem * NS_PER_SEC / s_freq;
}

/**
 * @brief Moves #s_deadline forward by exactly one period.
 *
 * When freq is not a multiple of hz, the leftover (s_rem counter ticks per period) is
 * carried in s_acc and paid out one counter tick at a time, so the long-term rate is
 * exact instead of being off by the truncation error.
 *
 * Callers must keep the tick IRQ from running meanwhile: the IRQ handler itself, or
 * code that has masked IRQs.
 */
static void advance_deadline()
{
	s_deadline += s_interval;
	s_acc += s_rem;
	if (s_acc >= s_hz)
	{
		s_acc -= s_hz;
		s_deadline++;
	}
}

/**
 * @brief Interrupt handler of the virtual timer (registered for TIMER_IRQ_VIRT).
 *
 * Runs in interrupt context, CPU IRQs masked (called by hal_irq_dispatch).
 *
 * Ignores the interrupt if the timer condition isn't met. If the tick was stopped
 * while the IRQ was in flight, turns the timer off. Otherwise moves the deadline on by
 * whole periods until it is in the future (skipping missed ticks instead of firing a
 * burst), programs it, then calls the tick callback once.
 *
 * @param arg Unused (registered as NULL)
 */
static void tick_irq_handler(void* arg)
{
	(void) arg;

	if (!(read_cntv_ctl() & CTL_ISTATUS))
		return; /* nothing pending: ignore */

	if (!s_tick_cb)
	{ /* stopped while the IRQ was in flight */
		write_cntv_ctl(0);
		return;
	}

	/*
	 * Always advance by whole periods so the phase never changes. If we were
	 * so late that the next deadline has also passed, skip those ticks instead
	 * of firing a burst to catch up.
	 */
	uint64_t now = read_cntvct();
	do
	{
		advance_deadline();
	}
	while (s_deadline <= now);

	/* Writing a future compare value also clears the interrupt condition. */
	write_cntv_cval(s_deadline);

	s_tick_cb();
}

/* ---- hal_timer API ----------------------------------------------------- */

int hal_timer_init()
{
	if (s_ready)
		return 0; /* idempotent */

	uint64_t freq = read_cntfrq();
	if (freq == 0)
		return -ENODEV; /* firmware/boot code never set it */

	write_cntv_ctl(0); /* timer off, any stale condition cleared */

	int rc = hal_irq_register(TIMER_IRQ_VIRT, tick_irq_handler, NULL);
	if (rc < 0)
		return rc;
	hal_irq_enable(TIMER_IRQ_VIRT); /* harmless: the timer itself is off */

	s_freq = freq;
	s_start = read_cntvct();
	s_ready = true;
	return 0;
}

uint64_t hal_timer_now_ns()
{
	if (!s_ready)
		return 0;
	return ticks_to_ns(read_cntvct() - s_start);
}

int hal_timer_set_tick(uint32_t hz, void (*callback)(void))
{
	if (hz == 0)
	{ /* stop the tick */
		arch_irq_flags_t f = arch_irq_save();
		write_cntv_ctl(0);
		s_tick_cb = NULL;
		arch_irq_restore(f);
		return 0;
	}

	if (!s_ready)
		return -ENODEV;
	if (callback == NULL || hz > s_freq)
		return -EINVAL;

	arch_irq_flags_t f = arch_irq_save(); /* the IRQ handler reads this state */

	s_hz = hz;
	s_interval = s_freq / hz;
	s_rem = s_freq % hz;
	s_acc = 0;
	s_tick_cb = callback;

	s_deadline = read_cntvct();
	advance_deadline(); /* first tick one period from now */
	write_cntv_cval(s_deadline);
	write_cntv_ctl(CTL_ENABLE); /* enabled, interrupt unmasked */

	arch_irq_restore(f);
	return 0;
}
