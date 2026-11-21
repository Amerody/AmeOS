// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#include "uart_mini.h"

/**
 * @file uart_mini.c
 * @brief UART1 (mini UART) driver implementation
 * @ingroup uart
 */

#include "auxiliary/aux_regs.h"
#include "auxiliary/auxiliary.h"
#include "gpio/gpio.h"
#include <kernel/errno.h>
#include <lib/ringbuf.h>

#include <stddef.h>
#include <stdint.h>

#define PIN_START 14U ///< Start pin to configure
#define PIN_END 17U   ///< End pin to configure

#define AUX_UART_CLOCK 500000000                                  ///< 500 MHz clock
#define AUX_MU_BAUD(baud) ((AUX_UART_CLOCK / ((baud) * 8U)) - 1U) ///< Helper to calculate the baudrate

#define UART_MINI_RX_BUF_SIZE 256u ///< Size of the receive ring buffer in bytes. Must be a power of 2

/// @brief Bytes received by the RX interrupt, waiting for uart_mini_getc()
static struct ringbuf s_rx_buffer;

/// @brief Storage behind @ref s_rx_buffer
static uint8_t s_rx_storage[UART_MINI_RX_BUF_SIZE];

/**
 * @brief Checks whether the transmitter can accept a byte
 *
 * @retval true The UART1 can accept a byte to send
 * @retval false The UART1 can't accept a byte yet
 */
static bool uart_mini_is_write_ready()
{
	return (bool) mmio_read_field(&AUX->MU_LSR_REG, 0x1, 5); // Transmitter can accept a byte
}

/**
 * @brief Checks whether the receive FIFO holds at least one byte
 *
 * Reads the "data ready" bit of AUX_MU_LSR_REG (BCM2711 ARM Peripherals,
 * chapter 2, mini UART registers).
 *
 * @retval true The receive FIFO holds at least one byte
 * @retval false The receive FIFO is empty
 */
static bool uart_mini_is_data_ready()
{
	return (bool) mmio_read_field(&AUX->MU_LSR_REG, 0x1, 0); // Data ready
}

/**
 * @brief Mini UART interrupt handler, set with aux_set_callback()
 *
 * Only the receive interrupt is enabled (see uart_mini_enable()), so every
 * call means the receive FIFO holds data. Moves all of it into
 * @ref s_rx_buffer. Reading the FIFO until it is empty is what deasserts the
 * receive interrupt. Bytes that don't fit in the ring buffer are dropped.
 *
 * AUX_MU_IO_REG is read with a 32-bit access like every other AUX register;
 * only its low byte holds data.
 *
 * Runs in interrupt context with CPU IRQs masked.
 *
 * @param arg Unused (NULL)
 */
static void uart_mini_irq_handler(void* arg)
{
	(void) arg;

	while (uart_mini_is_data_ready())
	{
		uint8_t c = (uint8_t) (mmio_read(&AUX->MU_IO_REG) & 0xFF);
		(void) ringbuf_put(&s_rx_buffer, c); // Buffer full: the byte is dropped
	}
}

int uart_mini_enable(uint32_t baud)
{
	// @todo Make it so used pins are choosable (between possibilities)

	// Set the necessary pins to ALT5
	for (gpio_t pin = PIN_START; pin <= PIN_END; pin++)
	{
		gpio_set_resistor(pin, GPIO_PUP_PDN_SEL_PUP);
		gpio_select_function(pin, GPIO_FUNCTION_ALT5);
	}

	int rc = aux_init(); // Shared AUX interrupt line, registered only once
	if (rc != SUCCESS)
		return rc;

	aux_enable(AUX_PERIPHERAL_MU); // Enable the MU for register access

	mmio_write_field(&AUX->MU_LCR_REG, 0, 0b1, 7);  // DLAB bit clear
	mmio_write_field(&AUX->MU_CNTL_REG, 0, 0x3, 0); // TX/RX off while configuring

	mmio_write_field(&AUX->MU_IER_REG, 0, 0x3, 0); // Disable interrupts

	// With the interrupts off, the handler can't run while the buffer is reset
	ringbuf_init(&s_rx_buffer, s_rx_storage, UART_MINI_RX_BUF_SIZE);
	aux_set_callback(AUX_PERIPHERAL_MU, uart_mini_irq_handler, NULL);

	mmio_write_field(&AUX->MU_LCR_REG, 1, 0x1, 0); // Enable 8-bit mode
	mmio_write(&AUX->MU_IIR_REG, 0b110);           // Clear the receive and transmit FIFO

	mmio_write_field(&AUX->MU_BAUD_REG, AUX_MU_BAUD(baud), 0xFFFF, 0); // Write the Baudrate

	// Receive interrupt (bit 0) only. The transmit-empty interrupt (bit 1) stays
	// off: TX is polled, and an empty TX FIFO would keep the line asserted
	mmio_write_field(&AUX->MU_IER_REG, 1, 0x1, 0);
	mmio_write_field(&AUX->MU_CNTL_REG, 0b11, 0x3, 0); // Enable TX/RX

	return SUCCESS;
}

void uart_mini_putc(char c)
{
	while (!uart_mini_is_write_ready())
		;
	mmio_write(&AUX->MU_IO_REG, (uint32_t) c);
}

bool uart_mini_getc(char* c)
{
	return ringbuf_get(&s_rx_buffer, (uint8_t*) c);
}
