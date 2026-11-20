#include "drivers/console/console_uart.h"

#include <hal/hal_uart.h>
#include <kernel/console.h>
#include <kernel/errno.h>

/**
 * @file console_uart.c
 * @brief UART console sink: connects the console registry to hal_uart (see console_uart.h)
 * @ingroup console
 */

/// @brief Baud rate the console UART is set up with
#define UART_BAUD_RATE 115200

/// @brief Port of the UART console; 0xFF until console_uart_init() is called
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
	if (c == '\n')
		hal_uart_putc(s_port, '\r');
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
