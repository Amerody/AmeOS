// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#pragma once

/**
 * @file auxiliary.h
 * @brief Public AUX Peripherals Driver header
 * @ingroup aux
 *
 * Owns the registers the AUX peripherals share (AUX_IRQ, AUX_ENABLES) and the
 * shared AUX interrupt line. Each peripheral's own registers belong to its
 * driver (see decision 0004).
 */

#include <stdint.h>

/// @brief AUX Peripherals Status codes
typedef enum
{
	AUX_SUCCESS = 0, ///< Operation Success

} aux_status_t;

/**
 * @brief AUX Peripherals
 *
 * Each value is the peripheral's bit position in both AUX_IRQ and
 * AUX_ENABLES (BCM2711 ARM Peripherals, chapter 2, AUX register summary).
 * Don't reorder.
 */
typedef enum
{
	AUX_PERIPHERAL_MU = 0,  ///< Mini UART. Uses 1 Byte of data
	AUX_PERIPHERAL_SP1 = 1, ///< SP1. Uses 2 Bytes of data
	AUX_PERIPHERAL_SP2 = 2  ///< SP2. Uses 2 Bytes of data
} aux_peripheral_t;

/**
 * @brief Interrupt handler for one AUX peripheral, set with aux_set_callback().
 *
 * Runs in interrupt context with CPU IRQs masked. It must be short and must
 * not block. AUX_IRQ is read-only, so the handler has to quiet its own device
 * (for the mini UART: read the receive FIFO until it is empty), otherwise the
 * level-triggered AUX line fires again straight away. The end of interrupt at
 * the GIC is done by hal_irq_dispatch().
 *
 * @param arg The pointer given to aux_set_callback().
 */
typedef void (*aux_callback_t)(void* arg);

/**
 * @brief Registers and enables the shared AUX interrupt line.
 *
 * Registers one handler with hal_irq on the AUX line and enables it at the
 * interrupt controller. That handler reads AUX_IRQ and calls the callback of
 * every peripheral whose bit is set. No peripheral raises interrupts until its
 * driver enables them, so the line stays quiet until then.
 *
 * Safe to call more than once: later calls do nothing and return 0, so every
 * AUX driver can call it from its own init.
 *
 * Call with the interrupt controller up (after platform_init()).
 *
 * @retval 0 Success, or already initialised
 * @retval -EINVAL The interrupt controller is not initialised yet
 *                 (from hal_irq_register())
 * @retval -EBUSY Something else already registered a handler on the AUX line
 *                (from hal_irq_register())
 */
int aux_init();

/**
 * @brief Sets the interrupt handler of one AUX peripheral.
 *
 * Set it before enabling the peripheral's interrupts. If a peripheral raises
 * an interrupt with no handler set, the AUX handler tries to silence it (for
 * the mini UART it flushes the receive FIFO, losing those bytes). If that
 * doesn't clear it, it disables the whole AUX line, which silences every AUX
 * peripheral until hal_irq_enable() is called again.
 *
 * Safe to call while the AUX line is enabled: the handler and its argument are
 * swapped with CPU IRQs masked.
 *
 * @param peripheral Peripheral to set the handler for. Out-of-range values are
 *                   ignored.
 * @param callback   Handler to call, or NULL to remove it.
 * @param arg        Passed back to @p callback unchanged; may be NULL.
 */
void aux_set_callback(aux_peripheral_t peripheral, aux_callback_t callback, void* arg);

/**
 * @brief Enables an AUX peripheral
 *
 * Must be called before accessing any of the peripheral's registers.
 * Enabling an already enabled peripheral is harmless, and the other
 * peripherals' enable bits are left untouched.
 *
 * @param peripheral Peripheral to enable
 */
void aux_enable(aux_peripheral_t peripheral);

/**
 * @brief Disables an AUX peripheral
 *
 * While disabled, the peripheral's registers are not accessible
 * (writes are ignored), so any configuration is lost.
 *
 * @param peripheral Peripheral to disable
 */
void aux_disable(aux_peripheral_t peripheral);

/**
 * @brief Checks whether an AUX peripheral is enabled
 *
 * @param peripheral Peripheral to check
 * @return true if enabled, false otherwise
 */
bool aux_is_enabled(aux_peripheral_t peripheral);
