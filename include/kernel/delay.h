// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#pragma once

/**
 * @file delay.h
 * @brief Busy-wait delays based on the HAL timer.
 * @ingroup kernel
 *
 * The delays spin on hal_timer_now_ns() until the time has passed. They keep the CPU
 * busy and do not mask interrupts, so interrupt handlers that run meanwhile can make
 * the delay longer than asked. The time is measured with hal_timer_now_ns(), so the
 * precision is that of the HAL timer.
 *
 * Precondition: call only after hal_timer_init(). Before that, hal_timer_now_ns()
 * returns 0, so any non-zero delay never ends.
 *
 * Context: they block (by spinning) for the whole delay. Called from an interrupt
 * handler, they hold up all other interrupts for that time.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief Busy-waits for at least @p ns nanoseconds.
	 *
	 * @param ns Delay in nanoseconds
	 */
	void delay_ns(uint64_t ns);

	/**
	 * @brief Busy-waits for at least @p us microseconds.
	 *
	 * @param us Delay in microseconds
	 */
	static inline void delay_us(uint64_t us) { delay_ns(us * 1000u); }

	/**
	 * @brief Busy-waits for at least @p ms milliseconds.
	 *
	 * @param ms Delay in milliseconds
	 */
	static inline void delay_ms(uint64_t ms) { delay_ns(ms * 1000000u); }

#ifdef __cplusplus
}
#endif