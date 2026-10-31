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
	UART_SUCCESS = 0,

	UART_ERROR_INVALID_UART = -1,
	UART_ERROR_NULL_STRING = -2,
} uart_status_t;

/**
 * @brief Initializes the given UART
 *
 * @param uart_id ID of the UART (0..5)
 * @return Status of the operation
 * @retval UART_SUCCESS The operation was successfull
 * @retval UART_ERROR_NULL_STRING @p uart_id is out of range (0..5)
 */
uart_status_t uart_init(uart_t uart_id);

/**
 * @brief Prints a string to the UART1
 *
 * @param uart_id UART to send the string to
 * @param buffer String to print.
 *
 * @return Status of the operation
 * @retval UART_SUCCESS The operation was successfull
 * @retval UART_ERROR_INVALID_UART @p uart_id is out of range (0..5)
 * @retval UART_ERROR_INVALID_STRING @p buffer is NULL
 */
uart_status_t uart_puts(uart_t uart_id, const char* buffer);

/** @} */
