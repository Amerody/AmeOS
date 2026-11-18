#include "drivers/console/console_uart.h"

#include <hal/hal_uart.h>
#include <kernel/console.h>
#include <kernel/errno.h>

#define UART_BAUD_RATE 115200

static unsigned s_port = 0xFF;

/**
 * @brief Console @c putc of the UART sink: sends one byte with hal_uart_putc()
 *
 * No check that @ref s_port is valid: the console only calls this after
 * console_uart_init() set it.
 *
 * @param c Byte to send
 */
static void console_uart_putc(char c)
{
	hal_uart_putc(s_port, c);
}

/**
 * @brief Console @c getc of the UART sink: takes one received byte with
 *        hal_uart_getc(). Never blocks.
 *
 * No check that @ref s_port is valid: the console only calls this after
 * console_uart_init() set it.
 *
 * @param[out] c Where the byte is stored; untouched if there is none.
 *
 * @retval true A byte was stored in @p c
 * @retval false No byte was available
 */
static bool console_uart_getc(char* c)
{
	return hal_uart_getc(s_port, c);
}

/**
 * @brief Initialise a UART and register it as a console sink.
 *
 * Calls hal_uart_init(`port`, `baud`) (8N1) and then `console_register()` with
 * a sink named "uart" that sends bytes through hal_uart_putc() and reads
 * received bytes through hal_uart_getc().
 *
 * Newline handling is done here: every '\n' is sent as "\r\n" so serial
 * terminals start a new line at column 0. A lone '\r' is passed through
 * unchanged.
 *
 * Only one UART console is supported: the port is kept in a private static, so
 * a second call returns -EBUSY.
 *
 * @param port Platform UART number; 1 is the console UART.
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
	uart_console.getc = console_uart_getc;
	console_register(&uart_console);

	return hal_uart_init(s_port, UART_BAUD_RATE);
}
