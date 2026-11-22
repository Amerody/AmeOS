// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#include "drivers/console/console_uart.h"

#include <hal/hal_uart.h>
#include <kernel/console.h>
#include <kernel/error.h>

#include <stddef.h>

/**
 * @file console_uart.c
 * @brief UART console sink: connects the console registry to hal_uart (see console_uart.h)
 * @ingroup console
 */

/// @brief Baud rate the console UART is set up with
#define UART_BAUD_RATE 115200

/// @brief ANSI sequence @c "ESC[2J ESC[H": erase the whole screen, then move the cursor home
constexpr uint8_t CLEAR_SEQ[] = { 0x1Bu, 0x5Bu, 0x32u, 0x4Au, 0x1Bu, 0x5Bu, 0x48u };
/// @brief Length of #CLEAR_SEQ, in bytes
constexpr size_t CLEAR_SEQ_SIZE = sizeof(CLEAR_SEQ);

/// @brief Port of the UART console; 0xFF until console_uart_init() is called
static unsigned s_port = 0xFF;

/**
 * @brief Console @c putc of the UART sink: sends one byte with hal_uart_putc()
 *
 * @c '\\n' is sent as @c "\r\n", because hal_uart_putc() leaves newline translation to
 * the console layer.
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

/**
 * @brief Console @c clear of the UART sink: sends #CLEAR_SEQ, so an ANSI terminal
 *        erases its screen and moves its cursor home.
 *
 * No check that @ref s_port is valid: the console only calls this after
 * console_uart_init() set it.
 */
static void console_uart_clear()
{
	for (size_t i = 0; i < CLEAR_SEQ_SIZE; i++)
	{
		hal_uart_putc(s_port, CLEAR_SEQ[i]);
	}
}

/**
 * @brief Console @c flush of the UART sink: waits with hal_uart_flush() until every byte
 *        sent so far has left the UART.
 *
 * Busy-waits with no timeout. No check that @c s_port is valid: the console only calls
 * this after console_uart_init() set it.
 */
static void console_uart_flush()
{
	hal_uart_flush(s_port);
}

int console_uart_init(unsigned port)
{
	if (s_port != 0xFF)
		return -ERROR_BUSY;

	s_port = port;

	static struct console uart_console;
	uart_console.name  = "uart";
	uart_console.putc  = console_uart_putc;
	uart_console.getc  = console_uart_getc;
	uart_console.clear = console_uart_clear;
	uart_console.flush = console_uart_flush;
	console_register(&uart_console);

	return hal_uart_init(s_port, UART_BAUD_RATE);
}
