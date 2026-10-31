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
		uart_enable(uart_id); // TODO: implement
		break;
	case 1:
		uart_mini_enable();
	default:
		return UART_ERROR_INVALID_UART;
	}

	return UART_SUCCESS;
}

uart_status_t uart_puts(uart_t uart_id, const char* buffer)
{
	if(!buffer)
		return UART_ERROR_NULL_STRING;

	switch(uart_id)
	{
	case 0:
	case 2:
	case 3:
	case 4:
	case 5:
		uart_enable(uart_id); // placeholder
		break;
	case 1:
		uart_mini_puts(buffer);
	default:
		return UART_ERROR_INVALID_UART;
	}

	return UART_SUCCESS;
}

/** @} */