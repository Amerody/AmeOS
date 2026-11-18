#include "auxiliary.h"
#include "aux_regs.h"
#include <common/mmio.h>
#include <hal/hal_irq.h>

/**
 * @addtogroup aux
 * @{
 */

/**
 * @file auxiliary.c
 *
 * @brief Implementation of the AUX Peripherals Driver
 */

#include <stddef.h>
#include <stdint.h>

#define AUX_NUM_PERIPHERALS 3u
#define AUX_INTERRUPT_NUM 29u

// interrupt masks
#define AUX_MU_IRQ_MASK 0b001
#define AUX_SPI1_IRQ_MASK 0b010
#define AUX_SPI2_IRQ_MASK 0b100

// AUX_MU_IIR Interrupt ID
#define AUX_MU_IIR_NO_IRQ 0b00
#define AUX_MU_IIR_TX_EMPTY 0b01
#define AUX_MU_IIR_RX_VALID_BYTE 0b10

// AUX_MU_IIR acknowledge bits
#define AUX_MU_IIR_CLEAR_RX_FIFO 0b010
#define AUX_MU_IIR_CLEAR_TX_FIFO 0b100

typedef struct
{
	aux_callback_t callback;
	void* args;
} aux_rsi_entry_t;

static aux_rsi_entry_t aux_cbs[AUX_NUM_PERIPHERALS];

/**
 * @brief Manages the selection and acknowledge of the MU interrupt
 *
 * @p entry should always be set before calling this function. @see aux_ISR;
 *
 * @param entry
 */
void aux_mu_irq(aux_rsi_entry_t* entry)
{
	aux_interrupt_t interrupt; // Type of interrupt
	// Check AUX_MU_IIR for what kind of interrupt it is
	uint32_t aux_mu_iir_reg = mmio_read_field(&AUX->MU_IIR_REG, 0b11, 1);
	switch (aux_mu_iir_reg)
	{
	case AUX_MU_IIR_TX_EMPTY:
		mmio_write(&AUX->MU_IIR_REG, AUX_MU_IIR_CLEAR_TX_FIFO);
		interrupt = AUX_MU_TRANSMIT;
		break;
	case AUX_MU_IIR_RX_VALID_BYTE:
		mmio_write(&AUX->MU_IIR_REG, AUX_MU_IIR_CLEAR_RX_FIFO);
		interrupt = AUX_MU_RECIEVE;
		break;
	default: // No interrupt or invalid (0b11)
		return;
	}

	entry->callback(interrupt, entry->args);
}

/**
 * @brief ISR for the AUX interrupt
 *
 * It dispatches the callbacks for each of the peripherals
 * depending on the AUX_IRQ register. It also aknowledges
 * the corresponding interrupt, so the registered ISR doesn't
 * have to.
 */
void aux_ISR(void*)
{
	aux_rsi_entry_t* entry; // entry from aux_cbs to use

	// first check AUX_IRQ register
	uint32_t aux_irq_reg = mmio_read(&AUX->IRQ);

	// we do it this way in case there are multiple interrupts waiting
	if (aux_irq_reg & AUX_MU_IRQ_MASK)
	{
		entry = &aux_cbs[(size_t) AUX_PERIPHERAL_MU];
		aux_mu_irq(entry);
	}

	if (aux_irq_reg & AUX_SPI1_IRQ_MASK)
	{
		entry = &aux_cbs[(size_t) AUX_PERIPHERAL_SP1];
		entry->callback(AUX_SPI1, entry->args);
		// TODO: Acknowledge the interrupt
		// for now SPI1 is not enabled
	}

	if (aux_irq_reg & AUX_SPI2_IRQ_MASK)
	{
		entry = &aux_cbs[(size_t) AUX_PERIPHERAL_SP2];
		entry->callback(AUX_SPI2, entry->args);
		// TODO: Acknowledge the interrupt
		// for now SPI2 is not enabled
	}
}

void aux_init()
{
	hal_irq_register(AUX_INTERRUPT_NUM, aux_ISR, NULL);
	for (size_t i = 0; i < AUX_NUM_PERIPHERALS; i++)
	{
		aux_cbs[i].callback = NULL;
		aux_cbs[i].args = NULL;
	}
}

void aux_set_callback(aux_peripheral_t peripheral, aux_callback_t callback, void* args)
{
	aux_cbs[(size_t) peripheral].callback = callback;
	aux_cbs[(size_t) peripheral].args = args;
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

/** @} */