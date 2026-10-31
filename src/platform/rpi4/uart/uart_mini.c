#include "uart_mini.h"

#include "../aux_regs.h"
#include "platform/rpi4/auxiliary.h"
#include "platform/rpi4/gpio.h"

#define PIN_START 14U
#define PIN_END 17U

bool uart_is_write_ready()
{
	return (bool)mmio_read_field(&AUX->AUX_MU_LSR_REG, 0x1, 6);
}

void uart_mini_putc(char c)
{
	while(!uart_is_write_ready())
		;
	mmio_write_field(&AUX->AUX_MU_IO_REG, (uint8_t)c, 0xFF, 0);
}

/// @todo Make it so used pins are choosable (between posiblities)
void uart_mini_enable()
{
	// Set the necessary pins to ALT5
	for(gpio_t pin = PIN_START; pin <= PIN_END; pin++)
	{
		gpio_set_resistor(pin, GPIO_PUP_PDN_SEL_PDN);
		gpio_select_function(pin, GPIO_FUNCTION_ALT5);
	}

	aux_enable(AUX_PERIPHERAL_MU);
	mmio_write_field(&AUX->AUX_MU_LCR_REG, 1, 0x1, 0); // Enable 8-bit mode
	mmio_write_field(&AUX->AUX_MU_IIR_REG, 3, 0x3, 1); // Clear the recieve and transmit FIFO
}

void uart_mini_puts(const char* buffer)
{
	while(*buffer)
	{
#if defined(_WIN32)
		if(*buffer == '\n')
			uart_mini_putc('\r');
#endif
		uart_mini_putc(*buffer++);
	}
}