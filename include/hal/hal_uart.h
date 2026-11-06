/**
 * @file hal_uart.h
 * @brief Serial-port contract.
 *
 * Implemented by src/platform/<PLATFORM>/ (the mini UART on rpi4). The
 * implementation hides everything hardware-specific: enabling the peripheral
 * block, pin muxing, clocks and baud divisors are its private business, and
 * callers only ask for a port at a baud rate.
 *
 * Ports are numbered by the platform. Port 0 is always the primary (console)
 * UART. Other numbers exist only if the platform has more UARTs.
 *
 * See platform.h for the shared conventions (errors, contexts, init order).
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

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
	int hal_uart_init(unsigned port, uint32_t baud);

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
	void hal_uart_putc(unsigned port, char c);

#ifdef __cplusplus
}
#endif