#pragma once

/**
 * @addtogroup aux
 * @{
 */

/**
 * @file aux_regs.h
 *
 * @brief Definition of the AUX Peripherals registers
 */

#include "platform/rpi4/mmio.h"
#include <stdint.h>


/// @brief All the AUX Peripherals Registers
/// @warning @p RESERVED_N are not to be used
typedef struct
{
	volatile uint32_t AUX_IRQ;     ///< Auxiliary Interrupt status
	volatile uint32_t AUX_ENABLES; ///< Auxiliary enables

	uint32_t RESERVED_04_36[14]; ///< 56 Bytes gap between AUX_ENABLES and AUX_MU_IO_REG

	volatile uint32_t AUX_MU_IO_REG;  ///< Mini UART I/O Data
	volatile uint32_t AUX_MU_IER_REG; ///< Mini UART Interrupt Enable
	volatile uint32_t AUX_MU_IIR_REG; ///< Mini UART Interrupt Identify

	volatile uint32_t AUX_MU_LCR_REG; ///< Mini UART Line Control
	volatile uint32_t AUX_MU_MCR_REG; ///< Mini UART Modem Control

	volatile uint32_t AUX_MU_LSR_REG; ///< Mini UART Line Status
	volatile uint32_t AUX_MU_MSR_REG; ///< Mini UART Modem Status

	volatile uint32_t AUX_MU_SCRATCH; ///< Mini UART Scratch

	volatile uint32_t AUX_MU_CNTL_REG; ///< Mini UART Extra Control
	volatile uint32_t AUX_MU_STAT_REG; ///< Mini UART Extra Status

	volatile uint32_t AUX_MU_BAUD_REG; ///< Mini UART Baudrate

	uint32_t RESERVED_6C_7C[5]; ///< 20 Bytes gap between AUX_MU_BAUD_REG and AUX_SPI1_CNTL0_REG

	volatile uint32_t AUX_SPI1_CNTL0_REG; ///< SPI 1 Control register 0
	volatile uint32_t AUX_SPI1_CNTL1_REG; ///< SPI 1 Control register 1

	volatile uint32_t AUX_SPI1_STAT_REG; ///< SPI 1 Status
	volatile uint32_t AUX_SPI1_PEEK_REG; ///< SPI 1 Peek

	uint32_t RESERVED_90_9C[4]; ///< 4 Bytes gap between AUX_SPI1_PEEK_REG and AUX_SPI1_IO_REGa

	volatile uint32_t AUX_SPI1_IO_REGa; ///< SPI 1 Data
	volatile uint32_t AUX_SPI1_IO_REGb; ///< SPI 1 Data
	volatile uint32_t AUX_SPI1_IO_REGc; ///< SPI 1 Data
	volatile uint32_t AUX_SPI1_IO_REGd; ///< SPI 1 Data

	volatile uint32_t AUX_SPI1_TXHOLD_REGa; ///< SPI 1 Extended Data
	volatile uint32_t AUX_SPI1_TXHOLD_REGb; ///< SPI 1 Extended Data
	volatile uint32_t AUX_SPI1_TXHOLD_REGc; ///< SPI 1 Extended Data
	volatile uint32_t AUX_SPI1_TXHOLD_REGd; ///< SPI 1 Extended Data

	volatile uint32_t AUX_SPI2_CNTL0_REG; ///< SPI 2 Control register 0
	volatile uint32_t AUX_SPI2_CNTL1_REG; ///< SPI 2 Control register 1

	volatile uint32_t AUX_SPI2_STAT_REG; ///< SPI 2 Status
	volatile uint32_t AUX_SPI2_PEEK_REG; ///< SPI 2 Peek

	uint32_t RESERVED_D0_DC[4]; ///< 4 Bytes gap between AUX_SPI2_PEEK_REG and AUX_SPI2_IO_REGa

	volatile uint32_t AUX_SPI2_IO_REGa; ///< SPI 2 Data
	volatile uint32_t AUX_SPI2_IO_REGb; ///< SPI 2 Data
	volatile uint32_t AUX_SPI2_IO_REGc; ///< SPI 2 Data
	volatile uint32_t AUX_SPI2_IO_REGd; ///< SPI 2 Data

	volatile uint32_t AUX_SPI2_TXHOLD_REGa; ///< SPI 2 Extended Data
	volatile uint32_t AUX_SPI2_TXHOLD_REGb; ///< SPI 2 Extended Data
	volatile uint32_t AUX_SPI2_TXHOLD_REGc; ///< SPI 2 Extended Data
	volatile uint32_t AUX_SPI2_TXHOLD_REGd; ///< SPI 2 Extended Data
} AUX_Registers;

#define AUX_BASE (PERIPHERAL_BASE + 0x215000) ///< AUX Peripherals base address
#define AUX ((AUX_Registers*)AUX_BASE)        ///< AUX Peripherals access

/** @} */