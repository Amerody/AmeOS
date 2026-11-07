#include "uart_mini.h"

/**
 * @addtogroup uart
 * @{
 */

/**
 * @file uart_mini.c
 *
 * @brief UART1 (mini UART) driver implementation
 */

#include "auxiliary/aux_regs.h"
#include "auxiliary/auxiliary.h"
#include "gpio/gpio.h"
#include <kernel/errno.h>

#define PIN_START 14U ///< Start pin to configure
#define PIN_END 17U   ///< End pin to configure

#define AUX_UART_CLOCK 500000000                                  ///< 500 MHz clock
#define AUX_MU_BAUD(baud) ((AUX_UART_CLOCK / ((baud) * 8U)) - 1U) ///< Helper to calculate the baudrate

/**
 * @brief Checks if the UART1 is ready to recieve a byte
 *
 * @retval true The UART1 is ready to recieve a byte
 * @retval false The UART1 is not ready to recieve a byte
 */
static bool uart_mini_is_write_ready()
{
	return (bool) mmio_read_field(&AUX->MU_LSR_REG, 0x1, 5); // Transmitter can accept a byte
}

/**
 * @brief Enables the UART1 (mini UART)
 *
 * @param baud Requested baud rate, e.g. 115200.
 *
 * @return 0 on success, -EINVAL if
 *         the baud rate is outside what the hardware can produce.
 */
int uart_mini_enable(uint32_t baud)
{
	// @todo Make it so used pins are choosable (between possibilities)
	// Set the necessary pins to ALT5
	for (gpio_t pin = PIN_START; pin <= PIN_END; pin++)
	{
		gpio_set_resistor(pin, GPIO_PUP_PDN_SEL_PUP);
		gpio_select_function(pin, GPIO_FUNCTION_ALT5);
	}
	aux_enable(AUX_PERIPHERAL_MU); // Enable the MU for register access

	mmio_write_field(&AUX->MU_LCR_REG, 0, 0b1, 7);  // DLAB bit clear
	mmio_write_field(&AUX->MU_CNTL_REG, 0, 0x3, 0); // TX/RX off while configuring

	mmio_write_field(&AUX->MU_IER_REG, 0, 0x3, 0); // Disable interrupts
	mmio_write_field(&AUX->MU_LCR_REG, 1, 0x1, 0); // Enable 8-bit mode
	mmio_write(&AUX->MU_IIR_REG, 0b110);           // Clear the receive and transmit FIFO

	mmio_write_field(&AUX->MU_BAUD_REG, AUX_MU_BAUD(baud), 0xFFFF, 0); // Write the Baudrate

	mmio_write_field(&AUX->MU_CNTL_REG, 0b11, 0x3, 0); // Enable TX/RX

	return SUCCESS;
}

void uart_mini_putc(char c)
{
	while (!uart_mini_is_write_ready())
		;
	mmio_write(&AUX->MU_IO_REG, (uint32_t) c);
}

/** @} */
