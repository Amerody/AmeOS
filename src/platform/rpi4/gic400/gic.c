// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#include "gic.h"

#include "soc.h"

#include <common/mmio.h>

#include <stdint.h>

/**
 * @file gic.c
 * @brief GICv2 (GIC-400) interrupt controller driver: register layouts and the
 *        functions declared in gic.h
 * @ingroup gic
 */

/// @brief All the GICD Registers
/// @warning @p RESERVED_* are not to be used
struct GICD_Registers
{
	volatile uint32_t CTLR;  ///< Distributor Control Register
	volatile uint32_t TYPER; ///< Interrupt Controller Type Register
	volatile uint32_t IIDR;  ///< Distributor Implementer Identification Register

	uint32_t RESERVED_00C_07C[CALC_OFFSET(0x00C, 0x07C)]; ///< 116 Bytes gap between IIDR and IGROUPR

	volatile uint32_t IGROUPR[CALC_OFFSET(0x080, 0x0BC)]; ///< Interrupt Group Registers

	uint32_t RESERVED_0C0_0FC[CALC_OFFSET(0x0C0, 0x0FC)]; ///< 64 Bytes gap between IGROUPR and ISENABLER

	volatile uint32_t ISENABLER[CALC_OFFSET(0x100, 0x13C)]; ///< Interrupt Set-Enable Registers

	uint32_t RESERVED_140_17C[CALC_OFFSET(0x140, 0x17C)]; ///< 64 Bytes gap between ISENABLER and ICENABLER

	volatile uint32_t ICENABLER[CALC_OFFSET(0x180, 0x1BC)]; ///< Interrupt Clear-Enable Registers

	uint32_t RESERVED_1C0_1FC[CALC_OFFSET(0x1C0, 0x1FC)]; ///< 64 Bytes gap between ICENABLER and ISPENDR

	volatile uint32_t ISPENDR[CALC_OFFSET(0x200, 0x23C)]; ///< Interrupt Set-Pending Registers

	uint32_t RESERVED_240_27C[CALC_OFFSET(0x240, 0x27C)]; ///< 64 Bytes gap between ISPENDR and ICPENDR

	volatile uint32_t ICPENDR[CALC_OFFSET(0x280, 0x2BC)]; ///< Interrupt Clear-Pending Registers

	uint32_t RESERVED_2C0_2FC[CALC_OFFSET(0x2C0, 0x2FC)]; ///< 64 Bytes gap between ICPENDR and ISACTIVER

	volatile uint32_t ISACTIVER[CALC_OFFSET(0x300, 0x33C)]; ///< Interrupt Set-Active Registers

	uint32_t RESERVED_340_37C[CALC_OFFSET(0x340, 0x37C)]; ///< 64 Bytes gap between ISACTIVER and ICACTIVER

	volatile uint32_t ICACTIVER[CALC_OFFSET(0x380, 0x3BC)]; ///< Interrupt Clear-Active Registers

	uint32_t RESERVED_3C0_3FC[CALC_OFFSET(0x3C0, 0x3FC)]; ///< 64 Bytes gap between ICACTIVER and IPRIORITYR

	volatile uint8_t IPRIORITYR[CALC_OFFSET(0x400, 0x5FC) * 4]; ///< Interrupt Priority Registers

	uint32_t RESERVED_600_7FC[CALC_OFFSET(0x600, 0x7FC)]; ///< 512 Bytes gap between IPRIORITYR and ITARGETSR

	volatile uint8_t ITARGETSR[CALC_OFFSET(0x800, 0x9FC) * 4]; ///< Interrupt Processor Targets Registers

	uint32_t RESERVED_A00_BFC[CALC_OFFSET(0xA00, 0xBFC)]; ///< 512 Bytes gap between ITARGETSR and ICFGR

	volatile uint32_t ICFGR[CALC_OFFSET(0xC00, 0xC7C)]; ///< Interrupt Configuration Registers

	uint32_t RESERVED_C80_CFC[CALC_OFFSET(0xC80, 0xCFC)]; ///< 128 Bytes gap between ICFGR and PPISR

	volatile uint32_t PPISR;                            ///< Private Peripheral Interrupt Status Register
	volatile uint32_t SPISR[CALC_OFFSET(0xD04, 0xD3C)]; ///< Shared Peripheral Interrupt Status Registers

	uint32_t RESERVED_D40_EFC[CALC_OFFSET(0xD40, 0xEFC)]; ///< 448 Bytes gap between SPISR and SGIR

	volatile uint32_t SGIR; ///< Software Generated Interrupt Register

	uint32_t RESERVED_F04_F0C[CALC_OFFSET(0xF04, 0xF0C)]; ///< 12 Bytes gap between SGIR and CPENDSGIR

	volatile uint32_t CPENDSGIR[CALC_OFFSET(0xF10, 0xF1C)]; ///< SGI Clear-Pending Registers
	volatile uint32_t SPENDSGIR[CALC_OFFSET(0xF20, 0xF2C)]; ///< SGI Set-Pending Registers

	uint32_t RESERVED_F30_FCC[CALC_OFFSET(0xF30, 0xFCC)]; ///< 160 Bytes gap between SPENDSGIR and PIDR4

	volatile uint32_t PIDR4; ///< Peripheral ID 4 Register
	volatile uint32_t PIDR5; ///< Peripheral ID 5 Register
	volatile uint32_t PIDR6; ///< Peripheral ID 6 Register
	volatile uint32_t PIDR7; ///< Peripheral ID 7 Register

	volatile uint32_t PIDR0; ///< Peripheral ID 0 Register
	volatile uint32_t PIDR1; ///< Peripheral ID 1 Register
	volatile uint32_t PIDR2; ///< Peripheral ID 2 Register
	volatile uint32_t PIDR3; ///< Peripheral ID 3 Register

	volatile uint32_t CIDR0; ///< Component ID 0 Register
	volatile uint32_t CIDR1; ///< Component ID 1 Register
	volatile uint32_t CIDR2; ///< Component ID 2 Register
	volatile uint32_t CIDR3; ///< Component ID 3 Register
};

#define GICD_BASE (PERIPHERAL_BASE + 0x1841000)        ///< GICD base address
#define GICD      ((struct GICD_Registers*) GICD_BASE) ///< GICD access

/// @brief All the GICC Registers
/// @warning @p RESERVED_* are not to be used
struct GICC_Registers
{
	volatile uint32_t CTLR;   ///< CPU Interface Control Register
	volatile uint32_t PMR;    ///< Interrupt Priority Mask Register
	volatile uint32_t BPR;    ///< Binary Point Register
	volatile uint32_t IAR;    ///< Interrupt Acknowledge Register
	volatile uint32_t EOIR;   ///< End of Interrupt Register
	volatile uint32_t RPR;    ///< Running Priority Register
	volatile uint32_t HPPIR;  ///< Highest Priority Pending Interrupt Register
	volatile uint32_t ABPR;   ///< Aliased Binary Point Register
	volatile uint32_t AIAR;   ///< Aliased Interrupt Acknowledge Register
	volatile uint32_t AEOIR;  ///< Aliased End of Interrupt Register
	volatile uint32_t AHPPIR; ///< Aliased Highest Priority Pending Interrupt Register

	uint32_t RESERVED_02C_0CC[CALC_OFFSET(0x02C, 0x0CC)]; ///< 164 Bytes gap between AHPPIR and APR0

	volatile uint32_t APR0; ///< Active Priority Register

	uint32_t RESERVED_0D4_0DC[CALC_OFFSET(0x0D4, 0x0DC)]; ///< 12 Bytes gap between APR0 and NSAPR0

	volatile uint32_t NSAPR0; ///< Non-Secure Active Priority Register

	uint32_t RESERVED_0E4_0F8[CALC_OFFSET(0x0E4, 0x0F8)]; ///< 24 Bytes gap between NSAPR0 and IIDR

	volatile uint32_t IIDR; ///< CPU Interface Identification Register

	uint32_t RESERVED_100_FFC[CALC_OFFSET(0x100, 0xFFC)]; ///< 3840 Bytes gap between IIDR and DIR

	volatile uint32_t DIR; ///< Deactivate Interrupt Register
};

#define GICC_BASE                      (PERIPHERAL_BASE + 0x1842000)        ///< GICC base address
#define GICC                           ((struct GICC_Registers*) GICC_BASE) ///< GICC access

#define GIC_INTID_SGI_BASE             0u  ///< First SGI (0-15)
#define GIC_INTID_PPI_BASE             16u ///< First PPI (16-31)
#define GIC_INTID_SPI_BASE             32u ///< First SPI (shared, 32 and above)

/// @brief Priority given to every interrupt by gic_init() and gic_cpu_init()
#define GIC_DEFAULT_PRIORITY           0xA0u
/// @brief Target CPU mask given to every SPI by gic_init(): core 0
#define GIC_DEFAULT_TARGET             0x01

#define GIC_MAX_PRIORITY_LEVEL         16u ///< Priority levels implemented. Since we boot in Non-secure mode, it's 16 for the GIC-400

#define GIC_DEFAULT_CORE_PRIORITY_MASK 0xFFu ///< Default core priority mask (all interrupts)

#define GICD_SGIR_SGIINTID_SHIFT       0u  ///< GICD_SGIR bits [3:0]: SGI ID
#define GICD_SGIR_TARGETLIST_SHIFT     16u ///< GICD_SGIR bits [23:16]: CPUTargetList (one bit per core)
#define GICD_SGIR_FILTER_SHIFT         24u ///< GICD_SGIR bits [25:24]: TargetListFilter

#define GICD_SGIR_FILTER_LIST          0x0u ///< TargetListFilter: use CPUTargetList
#define GICD_SGIR_FILTER_ALL_BUT_SELF  0x1u ///< TargetListFilter: every core except the requester
#define GICD_SGIR_FILTER_SELF          0x2u ///< TargetListFilter: only the requester (0x3: reserved)

/// @brief Number of interrupt IDs implemented, set by gic_init(); see gic_num_intids()
static uint32_t s_num_intids;

/// @brief Returns GIC_ERROR_INVALID_INTID from the calling function if @p id is out of range
#define GIC_CHECK_INTID(id)                 \
	do                                      \
	{                                       \
		if ((id) >= s_num_intids)           \
			return GIC_ERROR_INVALID_INTID; \
	}                                       \
	while (0)

/**
 * @brief Returns GIC_ERROR_INVALID_CPU_MASK from the calling function if any of the
 *        upper 4 bits of @p cpu_mask is set (only cores 0-3 exist)
 */
#define GIC_CHECK_CPU_MASK(cpu_mask)           \
	do                                         \
	{                                          \
		if ((((cpu_mask) & 0xF0u) >> 4) != 0)  \
			return GIC_ERROR_INVALID_CPU_MASK; \
	}                                          \
	while (0)

void gic_init()
{
	// Disable the GICD
	mmio_write(&GICD->CTLR, 0);

	// Read maximum interrupt ID
	uint32_t it_lines_number = mmio_read_field(&GICD->TYPER, 0b11111, 0); // Since NUM_SPIS = 192, it should be 192/32 = 6
	s_num_intids             = (32 * (it_lines_number + 1));              // NUM_SPIS = 192, so it should be (((192/32)+1)*32) = 224
	if (s_num_intids > GIC_MAX_NUM_INTERRUPTS)                            // Array size limit
		s_num_intids = GIC_MAX_NUM_INTERRUPTS;

	uint32_t num_words = (s_num_intids + 31) / 32;

	// Disable all SPIs and clear their active and pending state (word 0 is banked, see gic_cpu_init())
	for (uint32_t i = 1; i < num_words; i++)
	{
		mmio_write(&GICD->ICENABLER[i], 0xFFFFFFFF);
		mmio_write(&GICD->ICACTIVER[i], 0xFFFFFFFF);
		mmio_write(&GICD->ICPENDR[i], 0xFFFFFFFF);
	}

	// Give all SPIs a default priority, a default target (core 0) and level-sensitive configuration
	for (uint32_t id = GIC_INTID_SPI_BASE; id < s_num_intids; id++)
	{
		mmio_write8(&GICD->IPRIORITYR[id], GIC_DEFAULT_PRIORITY);
		mmio_write8(&GICD->ITARGETSR[id], GIC_DEFAULT_TARGET);
	}

	// ICFGR: 2 bits per interrupt, 16 interrupts per word. SPIs start at word 2.
	for (uint32_t w = GIC_INTID_SPI_BASE / 16; w < (s_num_intids + 15) / 16; w++)
	{
		mmio_write(&GICD->ICFGR[w], 0); // 0 = level-sensitive for every interrupt in the word
	}

	// Enable the GICD
	mmio_write(&GICD->CTLR, 1); // Since the GICD is disabled, a non-atomic operation is fine

	// Avoid cache issues with multi-core
	__asm__ volatile("dc cvac, %0\n\tdsb sy" ::"r"(&s_num_intids) : "memory");
}

void gic_cpu_init()
{
	// Banked SGIs and PPIs (IDs 0-31): disable, and clear active and pending state of this core's copy
	mmio_write(&GICD->ICENABLER[0], 0xFFFFFFFF);
	mmio_write(&GICD->ICACTIVER[0], 0xFFFFFFFF);
	mmio_write(&GICD->ICPENDR[0], 0xFFFFFFFF); // PPIs only: the SGI bits of this register are ignored

	// SGIs have their own clear-pending registers (one byte per SGI, one bit per source core)
	for (uint32_t i = 0; i < 4; i++)
	{
		mmio_write(&GICD->CPENDSGIR[i], 0xFFFFFFFF);
	}

	// SGIs and PPIs (IDs 0-31): IPRIORITYR0-7 are banked, so this only sets this core's priorities
	for (uint32_t id = 0; id < GIC_INTID_SPI_BASE; id++)
	{
		mmio_write8(&GICD->IPRIORITYR[id], GIC_DEFAULT_PRIORITY);
	}

	// Since there are 16 priority levels implemented (non-secure mode boot) bit[3:0] are 0b0000
	mmio_write(&GICC->PMR, GIC_DEFAULT_CORE_PRIORITY_MASK);

	// Enable the GICC. The whole register is written so that EOImode stays 0 (see gic_end_of_interrupt())
	mmio_write(&GICC->CTLR, 1);
}

gic_status_t gic_enable_irq(uint32_t intid)
{
	GIC_CHECK_INTID(intid);

	uint32_t reg_idx      = intid / 32; // Which ISENABLERn to write
	uint32_t required_bit = intid % 32; // Which bit number (0..31) is the one to set
	uint32_t value        = (1u << required_bit);

	mmio_write(&GICD->ISENABLER[reg_idx], value);

	return GIC_SUCCESS;
}

gic_status_t gic_disable_irq(uint32_t intid)
{
	GIC_CHECK_INTID(intid);

	uint32_t reg_idx      = intid / 32; // Which ICENALERn to write
	uint32_t required_bit = intid % 32; // Which bit number (0..31) is the one to set
	uint32_t value        = (1u << required_bit);

	mmio_write(&GICD->ICENABLER[reg_idx], value);

	return GIC_SUCCESS;
}

int gic_is_enabled(uint32_t intid)
{
	if (intid >= s_num_intids)
		return 0;

	uint32_t reg_idx      = intid / 32; // Which ISENABLERn to read
	uint32_t required_bit = intid % 32; // Which bit number (0..31) is the one to set
	uint32_t mask         = (1u << required_bit);

	uint32_t current_value = mmio_read(&GICD->ISENABLER[reg_idx]);
	return ((current_value & mask) >> required_bit);
}

uint32_t gic_acknowledge()
{
	return mmio_read(&GICC->IAR);
}

gic_status_t gic_end_of_interrupt(uint32_t iar)
{
	GIC_CHECK_INTID(GIC_IAR_INTID(iar));

	mmio_write(&GICC->EOIR, iar);

	return GIC_SUCCESS;
}

gic_status_t gic_set_priority(uint32_t intid, uint8_t prio)
{
	GIC_CHECK_INTID(intid);

	mmio_write8(&GICD->IPRIORITYR[intid], prio);

	return GIC_SUCCESS;
}

gic_status_t gic_set_target(uint32_t intid, uint8_t cpu_mask)
{
	GIC_CHECK_INTID(intid);

	GIC_CHECK_CPU_MASK(cpu_mask);

	if (intid < GIC_INTID_SPI_BASE)
		return GIC_ERROR_INTID_NOT_SPI;

	mmio_write8(&GICD->ITARGETSR[intid], cpu_mask);

	return GIC_SUCCESS;
}

gic_status_t gic_set_trigger(uint32_t intid, int edge)
{
	GIC_CHECK_INTID(intid);

	if (intid < GIC_INTID_SPI_BASE)
		return GIC_ERROR_INTID_NOT_SPI;

	uint32_t reg_idx = intid / 16; // Which ICFGRn to write
	uint32_t shift   = (intid % 16) * 2 + 1;

	mmio_write_field(&GICD->ICFGR[reg_idx], (edge != 0), 0x1, shift);

	return GIC_SUCCESS;
}

int gic_is_pending(uint32_t intid)
{
	if (intid >= s_num_intids)
		return 0;

	uint32_t reg_idx      = intid / 32;
	uint32_t bit_required = intid % 32;

	return mmio_read_field(&GICD->ISPENDR[reg_idx], 1, bit_required);
}

int gic_is_active(uint32_t intid)
{
	if (intid >= s_num_intids)
		return 0;

	uint32_t reg_idx      = intid / 32;
	uint32_t bit_required = intid % 32;

	return mmio_read_field(&GICD->ISACTIVER[reg_idx], 1, bit_required);
}

gic_status_t gic_set_pending(uint32_t intid)
{
	GIC_CHECK_INTID(intid);

	if (intid < GIC_INTID_PPI_BASE)
		return GIC_ERROR_INTID_IS_SGI;

	uint32_t reg_idx = intid / 32;
	uint32_t bit     = intid % 32;

	mmio_write(&GICD->ISPENDR[reg_idx], 1u << bit);

	return GIC_SUCCESS;
}

gic_status_t gic_clear_pending(uint32_t intid)
{
	GIC_CHECK_INTID(intid);

	if (intid < GIC_INTID_PPI_BASE)
		return GIC_ERROR_INTID_IS_SGI;

	uint32_t reg_idx = intid / 32;
	uint32_t bit     = intid % 32;

	mmio_write(&GICD->ICPENDR[reg_idx], 1u << bit);

	return GIC_SUCCESS;
}

gic_status_t gic_clear_sgi_pending(uint32_t intid, uint8_t src_cpu_mask)
{
	if (intid >= GIC_INTID_PPI_BASE)
		return GIC_ERROR_INTID_NOT_SGI;

	GIC_CHECK_CPU_MASK(src_cpu_mask);

	uint32_t reg_idx = intid / 4;       // 4 SGIs (bytes) per CPENDSGIRn
	uint32_t shift   = (intid % 4) * 8; // Which byte of the word belongs to this SGI

	mmio_write(&GICD->CPENDSGIR[reg_idx], ((uint32_t) src_cpu_mask) << shift);

	return GIC_SUCCESS;
}

gic_status_t gic_send_sgi(uint32_t intid, gic_sgi_target_t target, uint8_t cpu_mask)
{
	if (intid >= GIC_INTID_PPI_BASE)
		return GIC_ERROR_INTID_NOT_SGI;

	uint32_t filter;
	uint32_t target_list = 0;

	switch (target)
	{
	case GIC_SGI_TARGET_LIST:
		GIC_CHECK_CPU_MASK(cpu_mask);
		filter      = GICD_SGIR_FILTER_LIST;
		target_list = cpu_mask;
		break;

	case GIC_SGI_TARGET_ALL_BUT_SELF:
		filter = GICD_SGIR_FILTER_ALL_BUT_SELF;
		break;

	case GIC_SGI_TARGET_SELF:
		filter = GICD_SGIR_FILTER_SELF;
		break;

	default:
		return GIC_ERROR_INVALID_ARG;
	}

	uint32_t value = (filter << GICD_SGIR_FILTER_SHIFT) | (target_list << GICD_SGIR_TARGETLIST_SHIFT) | (intid << GICD_SGIR_SGIINTID_SHIFT);

	__asm__ volatile("dsb sy" ::: "memory"); // make earlier stores visible before the SGI is sent
	mmio_write(&GICD->SGIR, value);

	return GIC_SUCCESS;
}

uint32_t gic_num_intids() { return s_num_intids; }
