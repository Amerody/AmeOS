// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#include "uart_pl011.h"

/**
 * @file uart_pl011.c
 * @brief PL011 UART driver (UART0, UART2-5). Placeholder: nothing is implemented yet.
 * @ingroup uart
 */

void uart_pl011_enable(unsigned int id)
{
	(void) id;
	// Placeholder
}

void uart_pl011_putc(unsigned int port, char c)
{
	(void) port;
	(void) c;
	// Placeholder
}

bool uart_pl011_getc(unsigned int port, char* c)
{
	(void) port;
	(void) c;
	// Placeholder
	return false;
}

void uart_pl011_flush(unsigned int port)
{
	(void) port;
	return;
}
