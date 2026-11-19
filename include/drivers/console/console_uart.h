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
	 * Bytes are sent unchanged: there is no newline translation.
	 *
	 * Only one UART console is supported: the port is kept in a private static, so
	 * a second call returns -EBUSY.
	 *
	 * Call after console_init().
	 *
	 * @param port Platform UART number; 1 is the console UART.
	 *
	 * @retval 0 The UART is set up and registered
	 * @retval -EBUSY A UART console was already initialised, even if that earlier
	 *                call failed
	 * @return Otherwise the negative error from hal_uart_init() (-ENODEV, -EINVAL, or
	 *         another error if the receive interrupt can't be set up)
	 *
	 * @bug @c '\\n' is not sent as @c "\r\n", although hal_uart_putc() leaves newline
	 *      translation to the console layer. A serial terminal that doesn't add the
	 *      carriage return itself moves down a line without going back to column 0.
	 * @bug The result of console_register() is ignored, so -ENOSPC is never returned:
	 *      when the registry is full, the UART is set up but gets no console output.
	 * @bug The sink is registered before hal_uart_init(). If hal_uart_init() fails, the
	 *      sink stays registered on a port that isn't set up, and the port stays
	 *      claimed, so a retry returns -EBUSY.
	 */
	int console_uart_init(unsigned port);

#ifdef __cplusplus
}
#endif