// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#pragma once

/**
 * @file gpio.h
 * @brief Public GPIO Driver header
 * @ingroup gpio
 *
 * Context: every function does a non-atomic read-modify-write of the register that
 * holds the pin's field (mmio_write_field()). Calls that touch pins sharing a register
 * from different contexts (an interrupt handler and normal code) can lose a change.
 */

/// @brief Type to reference a pin
typedef unsigned int gpio_t;

/// @brief GPIO Status codes
typedef enum
{
	GPIO_SUCCESS = 0, ///< Operation Success

	GPIO_ERROR_INVALID_PIN   = -1, ///< An invalid pin has been provided
	GPIO_ERROR_INVALID_VALUE = -2, ///< An invalid value has been provided
} gpio_status_t;

/// @brief GPIO pull-up/down resistor values
typedef enum
{
	GPIO_PUP_PDN_SEL_NONE = 0, ///< De-select the resistor (neither pull-up o pull-down)
	GPIO_PUP_PDN_SEL_PUP  = 1, ///< Select the pull-up resistor
	GPIO_PUP_PDN_SEL_PDN  = 2  ///< Select the pull-down resistor
} gpio_resistor_t;

/// @brief GPIO Function select modes
typedef enum
{
	GPIO_FUNCTION_INPUT  = 0b000, ///< GPIO Pin as input
	GPIO_FUNCTION_OUTPUT = 0b001, ///< GPIO Pin as output
	GPIO_FUNCTION_ALT0   = 0b100, ///< GPIO Pin as alternate function 0
	GPIO_FUNCTION_ALT1   = 0b101, ///< GPIO Pin as alternate function 1
	GPIO_FUNCTION_ALT2   = 0b110, ///< GPIO Pin as alternate function 2
	GPIO_FUNCTION_ALT3   = 0b111, ///< GPIO Pin as alternate function 3
	GPIO_FUNCTION_ALT4   = 0b011, ///< GPIO Pin as alternate function 4
	GPIO_FUNCTION_ALT5   = 0b010, ///< GPIO Pin as alternate function 5
} gpio_function_t;

/**
 * @brief Sets the given GPIO pin
 *
 * @param pin Pin to set
 * 
 * @return Status of the operation
 * @retval GPIO_SUCCESS if the operation was successful
 * @retval GPIO_ERROR_INVALID_PIN @p pin is out of range
 */
gpio_status_t gpio_set(gpio_t pin);

/**
 * @brief Clears the given GPIO pin
 *
 * @param pin Pin to clear
 * 
 * @return Status of the operation
 * @retval GPIO_SUCCESS The operation was successful
 * @retval GPIO_ERROR_INVALID_PIN @p pin is out of range
 */
gpio_status_t gpio_clr(gpio_t pin);

/**
 * @brief Sets the resistor value to use for the given GPIO pin
 * @see gpio_resistor_t
 *
 * @param pin Pin to set the value for
 * @param value Value to set
 * @return Status of the operation
 * @retval GPIO_SUCCESS The operation was successful
 * @retval GPIO_ERROR_INVALID_PIN @p pin is out of range
 */
gpio_status_t gpio_set_resistor(gpio_t pin, gpio_resistor_t value);

/**
 * @brief Sets the function of a given GPIO pin
 * @see gpio_function_t
 *
 * @param pin Pin to set the function for
 * @param func Function to set
 * 
 * @return Status of the operation
 * @retval GPIO_SUCCESS The operation was successful
 * @retval GPIO_ERROR_INVALID_PIN @p pin is out of range
 */
gpio_status_t gpio_select_function(gpio_t pin, gpio_function_t func);
