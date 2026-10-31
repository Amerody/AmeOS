#pragma once

/**
 * @addtogroup aux
 * @{
 */

/**
 * @file auxiliary.h
 *
 * @brief Public AUX Peripherals Driver header
 */

#include <stdint.h>

/// @brief AUX Peripherals Status codes
typedef enum
{
	AUX_SUCCESS = 0, ///< Operation Success

} aux_status_t;

/// @brief AUX Peripherals
typedef enum
{
	AUX_PERIPHERAL_MU,  ///< Mini UART. Uses 1 Byte of data
	AUX_PERIPHERAL_SP1, ///< SP1. Uses 2 Bytes of data
	AUX_PERIPHERAL_SP2  ///< SP2. Uses 2 Bytes of data
} aux_peripheral_t;

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

/** @} */