#include "platform/rpi4/gpio.h"

/**
 * @addtogroup gpio
 * @{
 */

/**
 * @file gpio.c
 *
 * @brief Implementation of the GPIO Driver
 */

#include "platform/rpi4/mmio.h"

#include <stddef.h>
#include <stdint.h>

/// @brief All the GPIO Registers
/// @warning @p RESERVED_* are not to be used
typedef struct
{
	volatile uint32_t GPFSEL0; ///< GPIO Function Select 0
	volatile uint32_t GPFSEL1; ///< GPIO Function Select 1
	volatile uint32_t GPFSEL2; ///< GPIO Function Select 2
	volatile uint32_t GPFSEL3; ///< GPIO Function Select 3
	volatile uint32_t GPFSEL4; ///< GPIO Function Select 4
	volatile uint32_t GPFSEL5; ///< GPIO Function Select 5

	uint32_t RESERVED_18[1]; ///< 4 Bytes gap between GPFSEL5 and GPSET0

	volatile uint32_t GPSET0; ///< GPIO Pin Output Set 0
	volatile uint32_t GPSET1; ///< GPIO Pin Output Set 1

	uint32_t RESERVED_24[1]; ///< 4 Bytes gap between GPSET1 and GPCLR0

	volatile uint32_t GPCLR0; ///< GPIO Pin Output Clear 0
	volatile uint32_t GPCLR1; ///< GPIO Pin Output Clear 1

	uint32_t RESERVED_30[1]; ///< 4 Bytes gap between GPCLR1 and GPLEV0

	volatile uint32_t GPLEV0; ///< GPIO Pin Level 0
	volatile uint32_t GPLEV1; ///< GPIO Pin Level 1

	uint32_t RESERVED_3C[1]; ///< 4 Bytes gap between GPLEV1 and GPEDS0

	volatile uint32_t GPEDS0; ///< GPIO Pin Event Detect Status 0
	volatile uint32_t GPEDS1; ///< GPIO Pin Event Detect Status 1

	uint32_t RESERVED_48[1]; ///< 4 Bytes gap between GPEDS1 and GPREN0

	volatile uint32_t GPREN0; ///< GPIO Pin Rising Edge Detect Enable 0
	volatile uint32_t GPREN1; ///< GPIO Pin Rising Edge Detect Enable 1

	uint32_t RESERVED_54[1]; ///< 4 Bytes gap between GPREN1 and GPFEN0

	volatile uint32_t GPFEN0; ///< GPIO Pin Falling Edge Detect Enable 0
	volatile uint32_t GPFEN1; ///< GPIO Pin Falling Edge Detect Enable 1

	uint32_t RESERVED_60[1]; ///< 4 Bytes gap between GPFEN1 and GPHEN0

	volatile uint32_t GPHEN0; ///< GPIO Pin High Detect Enable 0
	volatile uint32_t GPHEN1; ///< GPIO Pin High Detect Enable 1

	uint32_t RESERVED_6C[1]; ///< 4 Bytes gap between GPHEN1 and GPLEN0

	volatile uint32_t GPLEN0; ///< GPIO Pin Low Detect Enable 0
	volatile uint32_t GPLEN1; ///< GPIO Pin Low Detect Enable 1

	uint32_t RESERVED_78[1]; ///< 4 Bytes gap between GPLEN1 and GPAREN0

	volatile uint32_t GPAREN0; ///< GPIO Pin Async. Rising Edge Detect 0
	volatile uint32_t GPAREN1; ///< GPIO Pin Async. Rising Edge Detect 1

	uint32_t RESERVED_84[1]; ///< 4 Bytes gap between GPAREN1 and GPAFEN0

	volatile uint32_t GPAFEN0; ///< GPIO Pin Async. Falling Edge Detect 0
	volatile uint32_t GPAFEN1; ///< GPIO Pin Async. Falling Edge Detect 1 (0x8c)

	uint32_t RESERVED_90_E0[CALC_OFFSET(0x90, 0xE0)]; ///< 84 Bytes gap between GPAFEN1 and GPIO_PUP_PDN_CNTRL_REG0

	volatile uint32_t GPIO_PUP_PDN_CNTRL_REG0; ///< GPIO Pull-up / Pull-down Register 0
	volatile uint32_t GPIO_PUP_PDN_CNTRL_REG1; ///< GPIO Pull-up / Pull-down Register 1
	volatile uint32_t GPIO_PUP_PDN_CNTRL_REG2; ///< GPIO Pull-up / Pull-down Register 2
	volatile uint32_t GPIO_PUP_PDN_CNTRL_REG3; ///< GPIO Pull-up / Pull-down Register 3
} GPIO_Registers;

#define GPIO_BASE (PERIPHERAL_BASE + 0x200000) ///< GPIO base address
#define GPIO ((GPIO_Registers*)GPIO_BASE)      ///< GPIO access

#define GPIO_MAX_PIN 58

/// @brief GPIO field sizes in bits
typedef enum
{
	GPIO_SET_FIELD_SIZE = 1,
	GPIO_CLR_FIELD_SIZE = 1,
	GPIO_PULL_FIELD_SIZE = 2,
	GPIO_FSEL_FIELF_SIZE = 3
} gpio_field_size_t;

/**
 * @brief Generalized call to a gpio operation
 *
 * @param pin Pin to use
 * @param value Value to write
 * @param base_reg Pointer to the base register of this register block (e.g., &(GPIO->GPFSEL0))
 * @param field_size Size of the field to write. Important for calculations on which register to use
 * @return Status of the operation
 * @retval GPIO_SUCCESS The operation was successful
 * @retval GPIO_ERROR_INVALID_PIN @p pin is out of range
 * @retval GPIO_ERROR_INVALID_VALUE @p value is invalid for its field
 */
gpio_status_t gpio_call(gpio_t pin, uint32_t value, mmio_register32_t base_reg, gpio_field_size_t field_size)
{
	uint32_t field_mask = (1U << field_size) - 1U;

	if(pin > GPIO_MAX_PIN)
		return GPIO_ERROR_INVALID_PIN;
	if(value > field_mask)
		return GPIO_ERROR_INVALID_VALUE;

	unsigned int num_fields = 32U / field_size;

	uint32_t reg_index = pin / num_fields;
	mmio_register32_t target_reg = base_reg + reg_index;
	unsigned int shift = (pin % num_fields) * field_size;

	mmio_write_field(target_reg, value, field_mask, shift);

	return GPIO_SUCCESS;
}

gpio_status_t gpio_set(gpio_t pin) { return gpio_call(pin, 1, &GPIO->GPSET0, GPIO_SET_FIELD_SIZE); }
gpio_status_t gpio_clr(gpio_t pin) { return gpio_call(pin, 1, &GPIO->GPCLR0, GPIO_CLR_FIELD_SIZE); }
gpio_status_t gpio_set_resistor(gpio_t pin, gpio_resistor_t value) { return gpio_call(pin, value, &GPIO->GPIO_PUP_PDN_CNTRL_REG0, GPIO_PULL_FIELD_SIZE); }
gpio_status_t gpio_select_function(gpio_t pin, gpio_function_t func) { return gpio_call(pin, func, &GPIO->GPFSEL0, GPIO_FSEL_FIELF_SIZE); }

/** @} */
