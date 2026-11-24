// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file platform.h
 * @brief Board-level contract: bring-up, identity and the RAM map.
 *
 * Implemented by src/platform/\<PLATFORM\>/ . This header is also
 * the home of the conventions shared by every hal_*.h header.
 *
 * ---- HAL conventions ----------------------------------------------------
 *
 * Boot order (kmain):
 *   1. arch_init()          exception vectors installed, IRQs masked
 *   2. platform_init()      interrupt controller up, memory map valid
 *   3. hal_uart_init(1, ..) console UART usable (called by console_uart_init(),
 *                           after console_init())
 *   4. hal_timer_init()     monotonic time and tick available
 *   5. arch_irq_enable()    CPU starts taking interrupts
 * A hal_* function used before the step that enables it is a bug.
 *
 * Errors: functions that can fail return 0 on success and a negative error
 * code on failure (-ERROR_INVALID_ARG, -ERROR_BUSY, -ERROR_NO_DEVICE, ...). Functions that cannot
 * meaningfully fail return void or the requested value.
 *
 * Contexts: unless a function says otherwise, it may be called from thread
 * context only. Functions that may also be called from interrupt handlers or
 * the panic path say so explicitly.
 *
 * Implementation detail (register layouts, pin muxing, clock setup, firmware
 * quirks) never appears in these headers. It stays private to the platform.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief Board-level bring-up that must happen before anything else uses
	 *        the HAL.
	 *
	 * Called once from kmain(), after arch_init(). On return:
	 *  - the interrupt controller is initialised, so hal_irq_register() /
	 *    hal_irq_enable() work (CPU IRQs are still masked);
	 *  - platform_mem_regions() returns valid data (including any parsing of
	 *    firmware-provided information such as a device tree);
	 *  - platform_get_info() returns the board description (or a note that it isn't
	 *    available).
	 *
	 * Does not touch the UART (see hal_uart_init()) or the timer (see
	 * hal_timer_init()), and does not unmask CPU interrupts.
	 *
	 * Nothing can be printed yet while this runs. If bring-up fails in a way that
	 * cannot be recovered, the implementation calls arch_halt().
	 *
	 * @param firmware_arg Firmware argument
	 * 
	 * @retval 0 Success
	 * @retval -1 The platform didn't initialize successfully (rpi4: never returned; a
	 *         failed board-information query isn't treated as an error)
	 */
	int platform_init(uintptr_t firmware_arg);

	/**
	 * @brief Reset the whole board, as if it had just been powered on.
	 *
	 * The firmware runs again and boots the kernel from the start. Nothing is
	 * saved, printed or flushed first: the caller does that (see kernel_shutdown(),
	 * which flushes the console before calling this).
	 *
	 * Never returns. Uses no kernel service, so it may be called with CPU IRQs
	 * masked. Valid at any point after boot; it doesn't depend on platform_init().
	 */
	[[noreturn]] void platform_reboot();

	/**
	 * @brief Power the board off, or get as close to that as the board can.
	 *
	 * A board that can't cut its own power stops in its lowest-power state and
	 * doesn't boot the kernel again until it is power-cycled. As with
	 * platform_reboot(), the caller saves and flushes anything it needs first.
	 *
	 * Never returns. Uses no kernel service, so it may be called with CPU IRQs
	 * masked. Valid at any point after boot; it doesn't depend on platform_init().
	 */
	[[noreturn]] void platform_shutdown();

	/**
	 * @brief Short platform name for the boot banner, e.g. "rpi4".
	 * @return A static string; never NULL.
	 */
	const char* platform_name();

	const char* platform_get_init_errors();

	/**
	 * @brief Human-readable description of the board, one item per line.
	 *
	 * The content is chosen by each platform (rpi4: firmware revision, board model,
	 * revision and serial, and the ARM and VideoCore memory). Lines are separated by
	 * @c '\\n'.
	 *
	 * Valid after platform_init(), which fills it in once; it doesn't change afterwards,
	 * so it can be read from any context. Before platform_init() it is an empty string.
	 *
	 * @return A static string; never NULL.
	 */
	const char* platform_get_info();

	/** @brief A contiguous range of physical memory. */
	struct mem_region
	{
		uintptr_t base; /**< First byte, page-aligned. */
		size_t    size; /**< Length in bytes, a multiple of the page size. */
	};

	/**
	 * @brief Describe the RAM the kernel may use.
	 *
	 * Regions are sorted by ascending address and do not overlap. They exclude
	 * memory owned by firmware or the GPU (rpi4: the device tree
	 * reservations and the DTB itself), but include the kernel image itself:
	 * core subtracts the image (using linker symbols) before handing pages to the
	 * physical allocator.
	 *
	 * Follows the snprintf convention: the return value is the total number of
	 * regions, which may exceed @p max. At most @p max entries are written.
	 *
	 * Valid after platform_init().
	 *
	 * @param out  Array to fill; may be NULL when @p max is 0 (to just count).
	 * @param max  Capacity of @p out in entries.
	 * @return     Total number of usable regions.
	 */
	size_t platform_mem_regions(struct mem_region* out, size_t max);

#ifdef __cplusplus
}
#endif
