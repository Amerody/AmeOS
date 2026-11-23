// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file console_uart.h
 * @brief Console sink that writes to a serial port and reads what it receives.
 * @ingroup console
 *
 * Implemented by src/drivers/console_uart.c. It is portable: it uses
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
	 * Registers a sink named "uart" with console_register(), then calls
	 * hal_uart_init(@p port, 115200) (8N1). The sink sends bytes through
	 * hal_uart_putc() and reads received bytes through hal_uart_getc().
	 *
	 * @c '\\n' is sent as @c "\r\n"; every other byte is sent unchanged. The sink's
	 * @c clear (see console_clear()) sends the ANSI sequence @c "ESC[2J ESC[H"
	 * (erase the screen, move the cursor home).
	 *
	 * Only one UART console is supported: the port is kept in a private static, so
	 * a second call returns -ERROR_BUSY.
	 *
	 * Call after console_init().
	 *
	 * @param port Platform UART number; 1 is the console UART.
	 *
	 * @retval 0 The UART is set up and registered
	 * @retval -ERROR_BUSY A UART console was already initialised, even if that earlier
	 *                call failed
	 * @retval -ERROR_NO_SPACE No more console sinks can be registered
	 * @return Otherwise the negative error from hal_uart_init()
	 */
	int console_uart_init(unsigned port);

#ifdef __cplusplus
}
#endif
