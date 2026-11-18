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

#include <stdint.h>

/**
 * @brief Enables the UART1 (mini UART)
 *
 * @param baud Requested baud rate, e.g. 115200.
 *
 * @return 0 on success, -ENODEV if the platform has no such port, -EINVAL if
 *         the baud rate is outside what the hardware can produce.
 */
int uart_mini_enable(uint32_t baud);

/**
 * @brief Writes a character to the UART1 (mini UART)
 *
 * @param c Character to write
 */
void uart_mini_putc(char c);

/**
 * @brief Recieve one byte from the UART1 (mini UART)
 *
 * @param c Byte to recieve.
 *
 * @return @p true if a byte was recieved. @p false otherwise
 */
bool uart_mini_getc(char* c);

/** @} */
