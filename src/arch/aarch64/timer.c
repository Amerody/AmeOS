#include "arch/aarch64/timer.h"

#include "arch/aarch64/irq.h"

/**
 * @addtogroup timer
 * @{
 */

/**
 * @file timer.h
 *
 * @brief ARMv8-A Timer driver implementation
 */

#include <stdint.h>

static uint64_t timer_freq = 0;
static uint64_t tick_interval = 0;

static inline uint64_t timer_read_cntpct()
{
	uint64_t count;
	__asm__ volatile("mrs %0, cntpct_el0" : "=r"(count));
	return count;
}

static inline uint64_t timer_read_cntvct()
{
	uint64_t v;
	__asm__ volatile("mrs %0, cntvct_el0" : "=r"(v));
	return v;
}

static inline uint64_t timer_read_freq()
{
	uint64_t freq;
	__asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
	return freq;
}

void timer_init()
{
	timer_freq = timer_read_freq(); // Typically 54MHz on the rpi4
}

uint64_t timer_get_ticks()
{
	return timer_read_cntpct();
}

void timer_sleep(uint32_t ms)
{
	if(timer_freq == 0)
		timer_init();

	uint64_t end = timer_get_ticks() + (timer_freq / 1000) * ms;

	while(timer_get_ticks() < end)
	{
		// Busy-wait
	}
}

void timer_interrupt_init(uint32_t hz)
{
	if(hz == 0)
		return;

	// Read timer frequency (typically 54 MHz)
	if(timer_freq == 0)
		timer_init();

	tick_interval = timer_freq / hz;

	// Set trigger for 1 second from now
	uint64_t compare = timer_read_cntvct() + tick_interval;
	__asm__ volatile("msr cntv_cval_el0, %0" ::"r"(compare));
}

void timer_interrupt_enable()
{
	// Enable timer (bit 0 = enable, bit 1 = mask)
	uint64_t ctrl = (1 << 0); // Enable, don't mask
	__asm__ volatile("msr cntv_ctl_el0, %0" ::"r"(ctrl));

	enable_irq();
}

/** @} */