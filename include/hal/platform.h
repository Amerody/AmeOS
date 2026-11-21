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
 * code on failure (-EINVAL, -EBUSY, -ENODEV, ...). Functions that cannot
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
	 *    firmware-provided information such as a device tree).
	 *
	 * Does not touch the UART (see hal_uart_init()) or the timer (see
	 * hal_timer_init()), and does not unmask CPU interrupts.
	 *
	 * Nothing can be printed yet while this runs. If bring-up fails in a way that
	 * cannot be recovered, the implementation calls arch_halt().
	 *
	 * @todo The device tree isn't parsed yet, so platform_mem_regions() has no data
	 *       after this returns.
	 */
	void platform_init();

	/**
	 * @brief Short platform name for the boot banner, e.g. "rpi4".
	 * @return A static string; never NULL.
	 */
	const char* platform_name();

	/** @brief A contiguous range of physical memory. */
	struct mem_region
	{
		uintptr_t base; /**< First byte, page-aligned. */
		size_t size;    /**< Length in bytes, a multiple of the page size. */
	};

	/**
	 * @brief Describe the RAM the kernel may use.
	 *
	 * Regions are sorted by ascending address and do not overlap. They exclude
	 * memory owned by firmware or the GPU, but include the kernel image itself:
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
	 *
	 * @todo Not implemented yet: always returns 0 and writes nothing, until the
	 *       device tree is parsed.
	 */
	size_t platform_mem_regions(struct mem_region* out, size_t max);

#ifdef __cplusplus
}
#endif