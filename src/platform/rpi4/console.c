#include "kernel/console.h"

/**
 * @addtogroup console
 * @{
 */

/**
 * @file console.h
 *
 * @brief Kernel console header
 */

#include "platform/rpi4/uart.h"

#define UART1 1 ///< mini UART

static bool console_initialized = false;

bool console_init()
{
	if(console_initialized)
		return false;

	uart_init(UART1);
	console_initialized = true;
	return true;
}

console_status_t console_putc(char c)
{
	if(uart_putc(UART1, c) != UART_SUCCESS)
		return CONSOLE_ERROR_UNKNOWN;

	return CONSOLE_SUCCESS;
}

console_status_t console_puts(const char* buffer)
{
	if(!buffer)
		return CONSOLE_ERROR_NULL_STRING;

	while(*buffer)
	{
		if(*buffer == '\n')
			uart_putc(UART1, '\r');
		uart_putc(UART1, *buffer++);
	}

	return CONSOLE_SUCCESS;
}

/** @} */
