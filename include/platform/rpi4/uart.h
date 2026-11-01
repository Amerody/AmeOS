#pragma once

/**
 * @addtogroup uart
 * @{
 */

/**
 * @file uart.h
 *
 * @brief Public UART Driver header
 */

/// @brief Type to reference an UART
typedef unsigned int uart_t;

/// @brief UART Status codes
typedef enum
{
	UART_SUCCESS = 0, ///< Operation Success

	UART_ERROR_INVALID_UART = -1, ///< An invalid UART has been provided
} uart_status_t;

/**
 * @brief Initializes the given UART
 *
 * @param uart_id ID of the UART (0..5)
 * @return Status of the operation
 * @retval UART_SUCCESS The operation was successful
 * @retval UART_ERROR_INVALID_UART @p uart_id is out of range (0..5)
 */
uart_status_t uart_init(uart_t uart_id);

/**
 * @brief Prints a character to the given UART
 *
 * @param uart_id UART to send the character to
 * @param c Character to print.
 *
 * @return Status of the operation
 * @retval UART_SUCCESS The operation was successfull
 * @retval UART_ERROR_INVALID_UART @p uart_id is out of range (0..5)
 */
uart_status_t uart_putc(uart_t uart_id, char c);

/** @} */
