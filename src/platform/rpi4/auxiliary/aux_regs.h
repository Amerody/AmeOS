// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#pragma once

/**
 * @file aux_regs.h
 * @brief Definition of the AUX Peripherals registers
 * @ingroup aux
 */

#include "common/mmio.h"
#include "soc.h"
#include <stdint.h>

/// @brief All the AUX Peripherals Registers
/// @warning @p RESERVED_* are not to be used
struct AUX_Registers
{
	volatile uint32_t IRQ;     ///< Auxiliary Interrupt status
	volatile uint32_t ENABLES; ///< Auxiliary enables

	uint32_t RESERVED_08_3C[CALC_OFFSET(0x08, 0x3C)]; ///< 56 Bytes gap between ENABLES and MU_IO_REG

	volatile uint32_t MU_IO_REG;  ///< Mini UART I/O Data
	volatile uint32_t MU_IER_REG; ///< Mini UART Interrupt Enable
	volatile uint32_t MU_IIR_REG; ///< Mini UART Interrupt Identify

	volatile uint32_t MU_LCR_REG; ///< Mini UART Line Control
	volatile uint32_t MU_MCR_REG; ///< Mini UART Modem Control

	volatile uint32_t MU_LSR_REG; ///< Mini UART Line Status
	volatile uint32_t MU_MSR_REG; ///< Mini UART Modem Status

	volatile uint32_t MU_SCRATCH; ///< Mini UART Scratch

	volatile uint32_t MU_CNTL_REG; ///< Mini UART Extra Control
	volatile uint32_t MU_STAT_REG; ///< Mini UART Extra Status

	volatile uint32_t MU_BAUD_REG; ///< Mini UART Baudrate

	uint32_t RESERVED_6C_7C[CALC_OFFSET(0x6C, 0x7C)]; ///< 20 Bytes gap between MU_BAUD_REG and SPI1_CNTL0_REG

	volatile uint32_t SPI1_CNTL0_REG; ///< SPI 1 Control register 0
	volatile uint32_t SPI1_CNTL1_REG; ///< SPI 1 Control register 1

	volatile uint32_t SPI1_STAT_REG; ///< SPI 1 Status
	volatile uint32_t SPI1_PEEK_REG; ///< SPI 1 Peek

	uint32_t RESERVED_90_9C[CALC_OFFSET(0x90, 0x9C)]; ///< 16 Bytes gap between SPI1_PEEK_REG and SPI1_IO_REGa

	volatile uint32_t SPI1_IO_REGa; ///< SPI 1 Data
	volatile uint32_t SPI1_IO_REGb; ///< SPI 1 Data
	volatile uint32_t SPI1_IO_REGc; ///< SPI 1 Data
	volatile uint32_t SPI1_IO_REGd; ///< SPI 1 Data

	volatile uint32_t SPI1_TXHOLD_REGa; ///< SPI 1 Extended Data
	volatile uint32_t SPI1_TXHOLD_REGb; ///< SPI 1 Extended Data
	volatile uint32_t SPI1_TXHOLD_REGc; ///< SPI 1 Extended Data
	volatile uint32_t SPI1_TXHOLD_REGd; ///< SPI 1 Extended Data

	volatile uint32_t SPI2_CNTL0_REG; ///< SPI 2 Control register 0
	volatile uint32_t SPI2_CNTL1_REG; ///< SPI 2 Control register 1

	volatile uint32_t SPI2_STAT_REG; ///< SPI 2 Status
	volatile uint32_t SPI2_PEEK_REG; ///< SPI 2 Peek

	uint32_t RESERVED_D0_DC[CALC_OFFSET(0xD0, 0xDC)]; ///< 16 Bytes gap between SPI2_PEEK_REG and SPI2_IO_REGa

	volatile uint32_t SPI2_IO_REGa; ///< SPI 2 Data
	volatile uint32_t SPI2_IO_REGb; ///< SPI 2 Data
	volatile uint32_t SPI2_IO_REGc; ///< SPI 2 Data
	volatile uint32_t SPI2_IO_REGd; ///< SPI 2 Data

	volatile uint32_t SPI2_TXHOLD_REGa; ///< SPI 2 Extended Data
	volatile uint32_t SPI2_TXHOLD_REGb; ///< SPI 2 Extended Data
	volatile uint32_t SPI2_TXHOLD_REGc; ///< SPI 2 Extended Data
	volatile uint32_t SPI2_TXHOLD_REGd; ///< SPI 2 Extended Data
};

#define AUX_BASE (PERIPHERAL_BASE + 0x215000) ///< AUX Peripherals base address
#define AUX ((struct AUX_Registers*) AUX_BASE)       ///< AUX Peripherals access
