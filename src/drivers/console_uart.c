#include "drivers/console/console_uart.h"

#include "hal/hal_uart.h"
#include "kernel/console.h"
#include "kernel/errno.h"

#define UART_BAUD_RATE 115200

static unsigned s_port = 0xFF;

/**
 * @brief Small wrapper to use hal_uart_putc()
 *
 * No check if @p s_port is invalid, since it's guaranteed to be at this point
 *
 * @param c Character to print
 */
static void console_uart_putc(char c)
{
	hal_uart_putc(s_port, c);
}

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
int console_uart_init(unsigned port)
{
	if (s_port != 0xFF)
		return -EBUSY;

	s_port = port;

	static struct console uart_console;
	uart_console.name = "uart";
	uart_console.putc = console_uart_putc;
	console_register(&uart_console);

	return hal_uart_init(s_port, UART_BAUD_RATE);
}