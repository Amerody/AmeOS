/**
 * @file console_fb.h
 * @brief Console sink that writes to a serial port.
 *
 * Implemented by src/drivers/console/console_fb.c. It is portable: it uses
 * only hal_fb, so it works unchanged on any platform that provides an implementation
 * of that HAL. Everything hardware-specific is hidden inside the platform's hal_fb
 * implementation.
 *
 * Called from kmain() after console_init():
 *
 * @code
 * console_init();
 * console_fb_init();
 * @endcode
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief Initialise a framebuffer and register it as a console sink
	 *
	 * @return 0 on success, -EBUSY if already initialised, or the error from
	 *         hal_fb_init() (-ENODEV, -EINVAL) or console_register() (-ENOSPC).
	 */
	int console_fb_init();

	void console_set_color(uint32_t color);

	void console_set_scale(uint32_t scale);

#ifdef __cplusplus
}
#endif