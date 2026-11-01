#include "arch/aarch64/timer.h"

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

static inline uint64_t timer_read_cntpct()
{
	uint64_t count;
	__asm__ volatile("mrs %0, cntpct_el0" : "=r"(count));
	return count;
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
	{
		timer_init();
	}

	uint64_t start = timer_get_ticks();
	uint64_t ticks_per_ms = timer_freq / 1000;
	uint64_t end = start + (ticks_per_ms * ms);

	while(timer_get_ticks() < end)
	{
		__asm__ volatile("nop"); // Busy wait. TODO: interrupts to sleep eficiently
	}
}

/** @} */