#pragma once

/**
 * @addtogroup uart
 * @{
 */

/**
 * @file uart_mini.h
 *
 * @brief UART1 (mini UART) driver header
 */

/**
 * @brief Enables the UART1 (mini UART)
 */
void uart_mini_enable();

/**
 * @brief Writes a character to the UART1 (mini UART)
 *
 * @param c Character to write
 */
void uart_mini_putc(char c);

/** @} */
