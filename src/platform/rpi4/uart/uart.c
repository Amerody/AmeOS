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
 *         the baud rate is outside what the hardware can produce, or another
 *         negative error if the UART's receive interrupt can't be set up
 *         (on rpi4: -EINVAL / -EBUSY from aux_init()).
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
		return uart_mini_enable(baud);
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
 * @brief Take one received byte, if there is one. Never blocks.
 *
 * The implementation buffers received bytes from hal_uart_init() on (on
 * rpi4 the UART's receive interrupt fills a ring buffer), and this takes
 * the oldest one. When that buffer was full, newer bytes were dropped.
 *
 * Only one context may read a port (the buffer has a single consumer), so
 * don't call it from interrupt handlers.
 *
 * Valid after a successful hal_uart_init() on the same port. Reading an
 * uninitialised or non-existent port returns false.
 *
 * @param port   Platform UART number.
 * @param[out] c Where the byte is stored; untouched if there is none.
 *
 * @retval true A byte was stored in @p c
 * @retval false No byte was available
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