#include "platform/rpi4/uart.h"

/**
 * @addtogroup uart
 * @{
 */

/**
 * @file uart.c
 *
 * @brief Implementation of the UART Driver
 */

#include "uart_mini.h"
#include "uart_pl011.h"

uart_status_t uart_init(uart_t uart_id)
{
	switch(uart_id)
	{
	case 0:
	case 2:
	case 3:
	case 4:
	case 5:
		uart_pl011_enable(uart_id); // TODO: implement
		break;
	case 1:
		uart_mini_enable();
	default:
		return UART_ERROR_INVALID_UART;
	}

	return UART_SUCCESS;
}

uart_status_t uart_putc(uart_t uart_id, char c)
{
	switch(uart_id)
	{
	case 0:
	case 2:
	case 3:
	case 4:
	case 5:
		uart_pl011_putc(uart_id);
		break;
	case 1:
		uart_mini_putc(c);
	default:
		return UART_ERROR_INVALID_UART;
	}

	return UART_SUCCESS;
}

/** @} */