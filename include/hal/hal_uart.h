// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file hal_uart.h
 * @brief Serial-port contract.
 *
 * Implemented by src/platform/<PLATFORM>/ (the mini UART on rpi4). The
 * implementation hides everything hardware-specific: enabling the peripheral
 * block, pin muxing, clocks and baud divisors are its private business, and
 * callers only ask for a port at a baud rate.
 *
 * Ports are numbered by the platform, using its hardware UART numbers (on rpi4
 * port N is UARTN, so the mini UART is port 1). Which port is the console is
 * chosen by the caller (see decision 0004).
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
	 *         the baud rate is outside what the hardware can produce, or another
	 *         negative error if the UART's receive interrupt can't be set up.
	 *
	 * @bug rpi4: ports 0 and 2-5 return 0 although the PL011 driver is only a
	 *      placeholder, so a caller (e.g. console_uart_init()) believes the port works
	 *      and all its output is silently dropped. -ENODEV would match the contract.
	 * @bug rpi4: -EINVAL is never returned for a bad @p baud (only from aux_init()). The divisor
	 *      500 MHz / (8 * baud) - 1 is not range-checked: @p baud 0 divides by zero,
	 *      rates below ~954 baud need more than the 16 bits of AUX_MU_BAUD_REG and
	 *      are silently truncated by mmio_write_field(), and very high rates
	 *      underflow the "- 1" or overflow 8 * baud.
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
	bool hal_uart_getc(unsigned port, char* c);

#ifdef __cplusplus
}
#endif