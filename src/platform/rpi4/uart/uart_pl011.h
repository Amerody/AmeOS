// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#pragma once

/**
 * @file uart_pl011.h
 * @brief PL011 UART driver (UART0, UART2-5) header. Placeholder: nothing is implemented
 *        yet.
 * @ingroup uart
 */

/**
 * @brief Enables a PL011 UART. Placeholder: does nothing yet.
 *
 * @param id Platform UART number (0, 2, 3, 4 or 5)
 */
void uart_pl011_enable(unsigned int id);

/**
 * @brief Writes a character to a PL011 UART. Placeholder: does nothing yet.
 *
 * @param port Platform UART number (0, 2, 3, 4 or 5)
 * @param c Character to write
 */
void uart_pl011_putc(unsigned int port, char c);

/**
 * @brief Takes one received byte from a PL011 UART. Placeholder: never has one yet.
 *
 * @param port Platform UART number (0, 2, 3, 4 or 5)
 * @param[out] c Where the byte would be stored; untouched
 *
 * @retval false Always: no byte available
 */
bool uart_pl011_getc(unsigned int port, char* c);

/**
 * @brief Waits until a PL011 UART has sent every byte written so far. Placeholder:
 *        returns at once.
 *
 * @param port Platform UART number (0, 2, 3, 4 or 5)
 */
void uart_pl011_flush(unsigned int port);
