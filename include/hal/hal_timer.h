// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file hal_timer.h
 * @brief Time source contract: monotonic clock and a periodic tick.
 * @ingroup timer
 *
 * Implemented by whichever layer owns the hardware. On aarch64 that is
 * src/arch/aarch64/ (the ARM generic timer is part of the CPU and is the same
 * on every aarch64 board), so any aarch64 platform gets it for free. Another
 * platform or architecture supplies its own implementation behind the same
 * header.
 *
 * See platform.h for the shared conventions (errors, contexts, init order).
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/// @brief Nanoseconds per microsecond: divide a nanosecond count by it to get microseconds
#define NS_TO_US 1000u
/// @brief Nanoseconds per millisecond: divide a nanosecond count by it to get milliseconds
#define NS_TO_MS 1000000u
/// @brief Nanoseconds per second: divide a nanosecond count by it to get seconds
#define NS_TO_SEC 1000000000u

	/**
	 * @brief Initialise the timer hardware and start the monotonic clock.
	 *
	 * Needs the interrupt controller, so call it after platform_init(). Does not
	 * start the tick (see hal_timer_set_tick()) and does not unmask CPU IRQs.
	 *
	 * Calling it again after a successful call does nothing and returns 0.
	 *
	 * @retval 0 The timer is running (or already was)
	 * @retval -ERROR_NO_DEVICE The timer hardware is unusable (on aarch64: the counter
	 *                 frequency, CNTFRQ_EL0, reads as 0)
	 * @return Otherwise the negative error from hal_irq_register() for the timer
	 *         interrupt
	 */
	int hal_timer_init();

	/**
	 * @brief Monotonic time in nanoseconds since hal_timer_init().
	 *
	 * Never goes backwards and is unaffected by tick configuration. The
	 * resolution is implementation-defined but at least 1 microsecond. A 64-bit
	 * nanosecond count does not wrap for centuries, so callers need not handle
	 * overflow.
	 *
	 * Safe to call from any context, including interrupt handlers and the panic
	 * path. Returns 0 before hal_timer_init().
	 *
	 * @return Nanoseconds elapsed.
	 */
	uint64_t hal_timer_now_ns();

	/**
	 * @brief Monotonic time in whole microseconds since hal_timer_init().
	 *
	 * hal_timer_now_ns() divided by #NS_TO_US, rounded down. Same context rules as
	 * hal_timer_now_ns(); returns 0 before hal_timer_init().
	 *
	 * @return Microseconds elapsed.
	 */
	static inline uint64_t hal_timer_now_us() { return hal_timer_now_ns() / NS_TO_US; }

	/**
	 * @brief Monotonic time in whole milliseconds since hal_timer_init().
	 *
	 * hal_timer_now_ns() divided by #NS_TO_MS, rounded down. Same context rules as
	 * hal_timer_now_ns(); returns 0 before hal_timer_init().
	 *
	 * @return Milliseconds elapsed.
	 */
	static inline uint64_t hal_timer_now_ms() { return hal_timer_now_ns() / NS_TO_MS; }

	/**
	 * @brief Monotonic time in whole seconds since hal_timer_init().
	 *
	 * hal_timer_now_ns() divided by #NS_TO_SEC, rounded down. Same context rules as
	 * hal_timer_now_ns(); returns 0 before hal_timer_init().
	 *
	 * @return Seconds elapsed.
	 */
	static inline uint64_t hal_timer_now_sec() { return hal_timer_now_ns() / NS_TO_SEC; }

	/**
	 * @brief Start, change or stop the periodic tick.
	 *
	 * @p callback runs @p hz times per second in interrupt context with CPU IRQs
	 * masked, so it must be short and must not block. Each deadline is computed
	 * from the previous deadline rather than from "now", so the tick does not
	 * drift when a callback runs late.
	 *
	 * Calling it again replaces the previous rate and callback. Passing @p hz == 0
	 * stops the tick (the callback is then ignored and may be NULL). Does not
	 * unmask CPU IRQs: nothing fires until arch_irq_enable().
	 *
	 * @param hz        Ticks per second; 0 stops the tick.
	 * @param callback  Function to call on every tick; required when @p hz != 0.
	 *
	 * Stopping the tick (@p hz == 0) always succeeds, even before hal_timer_init().
	 *
	 * @retval 0 The tick was started, changed or stopped
	 * @retval -ERROR_NO_DEVICE @p hz != 0 and hal_timer_init() hasn't succeeded yet
	 * @retval -ERROR_INVALID_ARG The rate is unsupported (on aarch64: higher than the counter
	 *                 frequency) or @p callback is NULL
	 */
	int hal_timer_set_tick(uint32_t hz, void (*callback)(void));

#ifdef __cplusplus
}
#endif