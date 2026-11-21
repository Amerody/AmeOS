// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#pragma once

/**
 * @file uart_mini.h
 * @brief UART1 (mini UART) driver header
 * @ingroup uart
 */

#include <stdint.h>

/**
 * @brief Enables the UART1 (mini UART)
 *
 * Configures it as 8N1 at @p baud and enables its receive interrupt. Received
 * bytes are buffered from then on and read with uart_mini_getc(). Also calls
 * aux_init(), which registers the shared AUX interrupt line the first time.
 *
 * Call with the interrupt controller up (after platform_init()). Calling it
 * again reconfigures the port and discards buffered bytes.
 *
 * @param baud Requested baud rate, e.g. 115200.
 *
 * @retval 0 Success
 * @retval -ERROR_INVALID_ARG The interrupt controller isn't initialised (from aux_init())
 * @retval -ERROR_BUSY The AUX interrupt line is taken by something else
 *                (from aux_init())
 */
int uart_mini_enable(uint32_t baud);

/**
 * @brief Writes a character to the UART1 (mini UART)
 *
 * Busy-waits until the transmitter can accept a byte. Uses no kernel service, so it
 * can be called from interrupt handlers and the panic path.
 *
 * @bug Check-then-act race: the LSR "can accept a byte" check and the MU_IO_REG write
 *      are not done with IRQs masked. If an interrupt handler prints in between and
 *      fills the 8-byte transmit FIFO again, this writes into a full FIFO and the byte
 *      is lost. Latent: no interrupt handler prints today.
 *
 * @param c Character to write
 */
void uart_mini_putc(char c);

/**
 * @brief Takes one received byte from the UART1 (mini UART), if there is one
 *
 * Never blocks. Bytes come from the receive buffer the RX interrupt fills,
 * oldest first. When the buffer was full, newer bytes were dropped.
 *
 * Only one context may read (the buffer has a single consumer): don't call it
 * from interrupt handlers.
 *
 * @param[out] c Where the byte is stored; untouched if there is none.
 *
 * @retval true A byte was stored in @p c
 * @retval false No byte was available
 */
bool uart_mini_getc(char* c);
