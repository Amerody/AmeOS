// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#include <hal/hal_uart.h>
#include <kernel/error.h>

/**
 * @file uart.c
 * @brief Implementation of the UART Driver
 * @ingroup uart
 */

#include "uart_mini.h"
#include "uart_pl011.h"

// On rpi4, the "receive interrupt can't be set up" errors are -ERROR_INVALID_ARG / -ERROR_BUSY from aux_init().
int hal_uart_init(unsigned port, uint32_t baud)
{
	switch (port)
	{
	case 0:
	case 2:
	case 3:
	case 4:
	case 5:
		return uart_pl011_enable(port); // TODO: implement
		break;
	case 1:
		return uart_mini_enable(baud);
	default:
		return -ERROR_NO_DEVICE;
	}

	return SUCCESS;
}

void hal_uart_putc(unsigned port, char c)
{
	switch (port)
	{
	case 0:
	case 2:
	case 3:
	case 4:
	case 5:
		uart_pl011_putc(port, c); // Todo: implement
		break;
	case 1:
		uart_mini_putc(c);
		break;
	default:
		return;
	}
}

bool hal_uart_getc(unsigned port, char* c)
{
	switch (port)
	{
	case 0:
	case 2:
	case 3:
	case 4:
	case 5:
		return uart_pl011_getc(port, c); // Todo: implement
	case 1:
		return uart_mini_getc(c);
	default:
		return false;
	}
}

void hal_uart_flush(unsigned port)
{
	switch (port)
	{
	case 0:
	case 2:
	case 3:
	case 4:
	case 5:
		uart_pl011_flush(port); // Todo: implement
		break;
	case 1:
		uart_mini_flush();
		break;
	default:
		break;
	}
}
