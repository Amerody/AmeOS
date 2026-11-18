#include <hal/hal_uart.h>

#include <kernel/errno.h>

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

/**
 * @brief Configure and enable a UART as 8 data bits, no parity, 1 stop bit.
 *
 * Calling it again reconfigures the port (for example to change the baud rate).
 * The implementation picks the closest achievable baud rate.
 *
 * @param port  Platform UART number
 * @param baud  Requested baud rate, e.g. 115200.
 * @return 0 on success, -ENODEV if the platform has no such port, -EINVAL if
 *         the baud rate is outside what the hardware can produce.
 */
int hal_uart_init(unsigned port, uint32_t baud)
{
	switch (port)
	{
	case 0:
	case 2:
	case 3:
	case 4:
	case 5:
		uart_pl011_enable(port); // TODO: implement
		break;
	case 1:
		uart_mini_enable(baud);
		break;
	default:
		return -ENODEV;
	}

	return SUCCESS;
}

/**
 * @brief Send one byte, waiting (polling) until the hardware accepts it.
 *
 * Sends the byte exactly as given: no newline translation (that belongs to the
 * console layer). Must work with CPU IRQs masked and without any kernel
 * service, so it is safe to call from interrupt handlers, exception handlers
 * and the panic path. It must not depend on locks, the allocator or the timer.
 *
 * Valid after a successful hal_uart_init() on the same port. Output to an
 * uninitialised or non-existent port is undefined.
 *
 * @param port  Platform UART number.
 * @param c     Byte to send.
 */
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

/**
 * @brief Recieve one byte.
 *
 * Gets the byte from the UART. There is no guarantee there is a byte.
 *
 * Valid after a successful hal_uart_init() on the same port. Output to an
 * uninitialised or non-existent port is undefined.
 *
 * @param port  Platform UART number.
 * @param c     Byte to recieve.
 *
 * @return @p true if a byte was recieved. @p false otherwise
 */
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

/** @} */