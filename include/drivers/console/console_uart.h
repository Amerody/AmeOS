/**
 * @file console_uart.h
 * @brief Console sink that writes to a serial port.
 *
 * Implemented by src/drivers/console/console_uart.c. It is portable: it uses
 * only hal_uart_*, so it works unchanged on any platform that provides a UART.
 * Everything hardware-specific (pin muxing, clocks, baud divisors) is hidden
 * inside the platform's hal_uart implementation.
 *
 * Called from kmain() after console_init():
 *
 * @code
 * console_init();
 * console_uart_init(1);
 * @endcode
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief Initialise a UART and register it as a console sink.
	 *
	 * Calls hal_uart_init(`port`, `baud`) (8N1) and then `console_register()` with
	 * a sink named "uart" that sends bytes through hal_uart_putc().
	 *
	 * Newline handling is done here: every '\n' is sent as "\r\n" so serial
	 * terminals start a new line at column 0. A lone '\r' is passed through
	 * unchanged.
	 *
	 * Only one UART console is supported: the port is kept in a private static, so
	 * a second call returns -EBUSY.
	 *
	 * @param port  Platform UART number; 0 is the console UART.
	 * @return 0 on success, -EBUSY if already initialised, or the error from
	 *         hal_uart_init() (-ENODEV, -EINVAL) or console_register() (-ENOSPC).
	 */
	int console_uart_init(unsigned port);

#ifdef __cplusplus
}
#endif