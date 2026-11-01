#pragma once

/**
 * @addtogroup console
 * @{
 */

/**
 * @file console.h
 *
 * @brief Kernel console header
 */

/// @brief Console Status codes
typedef enum
{
	CONSOLE_SUCCESS = 0, ///< Operation Success

	CONSOLE_ERROR_NULL_STRING = -1, ///< An NULL pointer has been provided for a string

	CONSOLE_ERROR_UNKNOWN = -0xFF ///< Unknown error has ocurred
} console_status_t;

/**
 * @brief Initializes the console
 *
 * @retval true The console has been initialized
 * @retval false The console was already initialized
 */
bool console_init();

/**
 * @brief Prints a character to the console
 *
 * @param c Character to print
 * @return Status of the operation
 * @retval CONSOLE_SUCCESS The operation was successful
 * @retval CONSOLE_ERROR_UNKNOWN An unknown error has ocurred
 */
console_status_t console_putc(char c);

/**
 * @brief Prints a string to the console
 *
 * @param buffer String to print
 * @return Status of the operation
 * @retval CONSOLE_SUCCESS The operation was successful
 * @retval CONSOLE_ERROR_NULL_STRING @p buffer is NULL
 * @retval CONSOLE_ERROR_UNKNOWN An unknown error has ocurred
 */
console_status_t console_puts(const char* buffer);

/** @} */
