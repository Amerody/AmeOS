// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#include "auxiliary.h"

#include "aux_regs.h"

#include <arch/irq.h>
#include <common/mmio.h>
#include <hal/hal_irq.h>
#include <kernel/error.h>

/**
 * @file auxiliary.c
 * @brief Implementation of the AUX Peripherals Driver
 * @ingroup aux
 */

#include <stddef.h>
#include <stdint.h>

/**
 * @brief GIC interrupt ID of the shared AUX interrupt line
 *
 * AUX is VideoCore peripheral IRQ 29, and the VideoCore peripheral IRQs start
 * at GIC SPI ID 96 (BCM2711 ARM Peripherals, chapter 6), so 96 + 29.
 */
#define AUX_IRQ_INTID            125u

#define AUX_PERIPHERAL_COUNT     3u ///< Number of AUX peripherals (MU, SPI1, SPI2)

#define AUX_MU_IIR_CLEAR_RX_FIFO (1u << 1) ///< AUX_MU_IIR_REG write: clear the receive FIFO

static_assert(AUX_PERIPHERAL_MU == 0 && AUX_PERIPHERAL_SP1 == 1 && AUX_PERIPHERAL_SP2 == 2, "aux_peripheral_t values must be the AUX_IRQ / AUX_ENABLES bit positions");

/// @brief Interrupt handler registered for one AUX peripheral
struct aux_handler_entry
{
	aux_callback_t callback; ///< Handler to call, NULL if none
	void*          arg;      ///< Passed back to @p callback
};

/// @brief Registered handlers, indexed by aux_peripheral_t. Also read from the IRQ path
static struct aux_handler_entry s_handlers[AUX_PERIPHERAL_COUNT];

/// @brief Whether aux_init() already registered the AUX line
static bool s_is_initialized = false;

/**
 * @brief Tries to silence a peripheral that raised an interrupt with no handler
 *
 * AUX_IRQ is read-only: an AUX interrupt only goes away once the peripheral
 * that raised it is serviced. For the mini UART, flushing the receive FIFO
 * deasserts the receive interrupt (the bytes are lost). Anything still pending
 * after that can't be cleared from here (the mini UART transmit-empty
 * interrupt stays asserted while its FIFO is empty; SPI1/SPI2 aren't handled
 * yet), so the whole AUX line is disabled to stop an interrupt storm.
 *
 * Runs in interrupt context.
 *
 * @param peripheral Peripheral whose AUX_IRQ bit is set
 */
static void silence_unhandled(aux_peripheral_t peripheral)
{
	if (peripheral == AUX_PERIPHERAL_MU)
		mmio_write(&AUX->MU_IIR_REG, AUX_MU_IIR_CLEAR_RX_FIFO);

	if (mmio_read_field(&AUX->IRQ, 0x1, (uint32_t) peripheral))
		hal_irq_disable(AUX_IRQ_INTID);
}

/**
 * @brief Handler of the shared AUX interrupt line, registered with hal_irq
 *
 * Reads AUX_IRQ once and calls the handler of every peripheral whose bit is
 * set (more than one can be pending at a time). Each handler quiets its own
 * device; peripherals without a handler go to silence_unhandled().
 *
 * Runs in interrupt context with CPU IRQs masked.
 *
 * @param arg Unused (NULL)
 */
static void aux_irq_handler(void* arg)
{
	(void) arg;

	uint32_t pending = mmio_read(&AUX->IRQ);

	for (uint32_t i = 0; i < AUX_PERIPHERAL_COUNT; i++)
	{
		if ((pending & (1u << i)) == 0)
			continue;

		struct aux_handler_entry* entry = &s_handlers[i];
		if (entry->callback != NULL)
			entry->callback(entry->arg);
		else
			silence_unhandled((aux_peripheral_t) i);
	}
}

int aux_init()
{
	if (s_is_initialized)
		return SUCCESS;

	int rc = hal_irq_register(AUX_IRQ_INTID, aux_irq_handler, NULL);
	if (rc != SUCCESS)
		return rc;

	hal_irq_enable(AUX_IRQ_INTID);
	s_is_initialized = true;

	return SUCCESS;
}

void aux_set_callback(aux_peripheral_t peripheral, aux_callback_t callback, void* arg)
{
	if ((uint32_t) peripheral >= AUX_PERIPHERAL_COUNT)
		return;

	arch_irq_flags_t flags          = arch_irq_save(); // The table is also read from the IRQ path
	s_handlers[peripheral].callback = callback;
	s_handlers[peripheral].arg      = arg;
	arch_irq_restore(flags);
}

void aux_enable(aux_peripheral_t peripheral)
{
	mmio_write_field(&AUX->ENABLES, 1, 0x1, (uint32_t) peripheral);
}

void aux_disable(aux_peripheral_t peripheral)
{
	mmio_write_field(&AUX->ENABLES, 0, 0x1, (uint32_t) peripheral);
}

bool aux_is_enabled(aux_peripheral_t peripheral)
{
	return (bool) mmio_read_field(&AUX->ENABLES, 0x1, (uint32_t) peripheral);
}
