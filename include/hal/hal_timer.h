/**
 * @file hal_timer.h
 * @brief Time source contract: monotonic clock and a periodic tick.
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

#define NS_TO_US 1000u
#define NS_TO_MS 1000000u
#define NS_TO_SEC 1000000000u

	/**
	 * @brief Initialise the timer hardware and start the monotonic clock.
	 *
	 * Needs the interrupt controller, so call it after platform_init(). Does not
	 * start the tick (see hal_timer_set_tick()) and does not unmask CPU IRQs.
	 *
	 * @return 0 on success, a negative error code if the timer is unusable.
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

	static inline uint64_t hal_timer_now_us() { return hal_timer_now_ns() / NS_TO_US; }

	static inline uint64_t hal_timer_now_ms() { return hal_timer_now_ns() / NS_TO_MS; }

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
	 * @return 0 on success, -EINVAL if the rate is unsupported or the callback is
	 *         missing.
	 */
	int hal_timer_set_tick(uint32_t hz, void (*callback)(void));

#ifdef __cplusplus
}
#endif