#include "gic.h"
#include "soc.h"
#include <common/mmio.h>

#include <stdint.h>

/// @brief All the GICD Registers
/// @warning @p RESERVED_* are not to be used
typedef struct
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
} GICD_Registers;

#define GICD_BASE (PERIPHERAL_BASE + 0x1841000) ///< GICD base address
#define GICD ((GICD_Registers*) GICD_BASE)      ///< GICD access

/// @brief All the GICC Registers
/// @warning @p RESERVED_* are not to be used
typedef struct
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

	uint32_t RESERVED_02C_0CC[CALC_OFFSET(0x02C, 0x0CC)]; ///< 168 Bytes gap between AHPPIR and APR0

	volatile uint32_t APR0; ///< Active Priority Register

	uint32_t RESERVED_0D4_0DC[CALC_OFFSET(0x0D4, 0x0DC)]; ///< 12 Bytes gap between APR0 and NSAPR0

	volatile uint32_t NSAPR0; ///< Non-Secure Active Priority Register

	uint32_t RESERVED_0E4_0F8[CALC_OFFSET(0x0E4, 0x0F8)]; ///< 24 Bytes gap between NSAPR0 and IIDR

	volatile uint32_t IIDR; ///< CPU Interface Identification Register

	uint32_t RESERVED_100_FFC[CALC_OFFSET(0x100, 0xFFC)]; ///< 3840 Bytes gap between IIDR and DIR

	volatile uint32_t DIR; ///< Deactivate Interrupt Register
} GICC_Registers;

#define GICC_BASE (PERIPHERAL_BASE + 0x1842000) ///< GICC base address
#define GICC ((GICC_Registers*) GICC_BASE)      ///< GICC access

#define GIC_INTID_SGI_BASE 0u  ///< First SGI (0-15)
#define GIC_INTID_PPI_BASE 16u ///< First PPI (16-31)
#define GIC_INTID_SPI_BASE 32u ///< First SPI (shared, 32 and above)

#define GIC_DEFAULT_PRIORITY 0xA0u
#define GIC_DEFAULT_TARGET 0x01

#define GIC_MAX_PRIORITY_LEVEL 16u ///< Priority levels implemented. Since we boot in Non-secure mode, it's 16 for the GIC-400

#define GIC_DEFAULT_CORE_PRIORITY_MASK 0xFFu ///< Default core priority mask (all interrupts)

#define GICD_SGIR_SGIINTID_SHIFT 0u    // bits [3:0]   SGI ID
#define GICD_SGIR_TARGETLIST_SHIFT 16u // bits [23:16] CPUTargetList (one bit per core)
#define GICD_SGIR_FILTER_SHIFT 24u     // bits [25:24] TargetListFilter

#define GICD_SGIR_FILTER_LIST 0x0u         // use CPUTargetList
#define GICD_SGIR_FILTER_ALL_BUT_SELF 0x1u // every core except the requester
#define GICD_SGIR_FILTER_SELF 0x2u         // only the requester (0x3 is reserved)

static uint32_t s_num_intids;

#define GIC_CHECK_INTID(id)                 \
	do                                      \
	{                                       \
		if ((id) >= s_num_intids)           \
			return GIC_ERROR_INVALID_INTID; \
	}                                       \
	while (0)

#define GIC_CHECK_CPU_MASK(cpu_mask)           \
	do                                         \
	{                                          \
		if ((((cpu_mask) & 0xF0u) >> 4) != 0)  \
			return GIC_ERROR_INVALID_CPU_MASK; \
	}                                          \
	while (0)

/**
 * @brief Initialize the GIC distributor.
 *
 * Must be called once, by the boot core, before any other GIC function.
 * Each core, including the boot core, must then call gic_cpu_init().
 *
 * The firmware or armstub may have left the GIC in an unknown state, so this
 * does not assume a clean slate:
 *  1. Disables the distributor (GICD_CTLR = 0).
 *  2. Reads GICD_TYPER.ITLinesNumber and stores the maximum interrupt ID
 *     (32 * (ITLinesNumber + 1)), used by every function below to validate
 *     @p intid.
 *  3. Disables and clears the pending state of all SPIs (GICD_ICENABLERn,
 *     GICD_ICPENDRn).
 *  4. Gives all SPIs a default priority, a default target (core 0) and
 *     level-sensitive configuration.
 *  5. Enables the distributor (GICD_CTLR).
 *
 * @note The kernel runs in the Non-secure state at EL1, so the security group
 *       of each interrupt (GICD_IGROUPRn) is not configurable from here. It is
 *       expected to have been set by the armstub (Group 1, signalled as IRQ).
 */
void gic_init()
{
	// Disable the GICD
	mmio_write(&GICD->CTLR, 0);

	// Read maximum interrupt ID
	uint32_t it_lines_number = mmio_read_field(&GICD->TYPER, 0b11111, 0); // Since NUM_SPIS = 192, it should be 192/32 = 6
	s_num_intids = (32 * (it_lines_number + 1));                          // NUM_SPIS = 192, so it should be (((192/32)+1)*32) = 224
	if (s_num_intids > GIC_MAX_NUM_INTERRUPTS)                            // Clamp s_num_intids to 1020, so ITLinesNumber=31 doesn't accept intids 1020-1023
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

/**
 * @brief Initialize the calling core's CPU interface.
 *
 * Must be called on every core that is going to receive interrupts.
 *
 * The SGIs and PPIs (IDs 0-31) are banked per core: each core has its own copy
 * of their enable, pending, active, priority and configuration registers at the
 * same address. This function is therefore where each core:
 *  1. Disables its SGIs and PPIs and clears their active and pending state
 *     (the firmware may have left them in any state).
 *  2. Sets the default priority of its own SGIs and PPIs.
 *  3. Sets the priority mask (GICC_PMR) so interrupts are signalled to the core.
 *  4. Enables the CPU interface (GICC_CTLR).
 *
 * Enabling a PPI with gic_enable_irq() on core 0 does nothing for core 1.
 */
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

/**
 * @brief Enable forwarding of an interrupt.
 *
 * Writes to GICD_ISENABLERn. The register is write-1-to-set, so no
 * read-modify-write is needed. Other interrupts are not affected.
 *
 * For @p intid < 32 (SGIs and PPIs) the register is banked, so this only
 * affects the calling core.
 *
 * @param intid Interrupt ID to enable
 *
 * @return Status of the operation
 * @retval GIC_SUCCESS The operation completed successfully
 * @retval GIC_ERROR_INVALID_INTID @p intid is out of range
 */
gic_status_t gic_enable_irq(uint32_t intid)
{
	GIC_CHECK_INTID(intid);

	uint32_t reg_idx = intid / 32;      // Which ISENABLERn to write
	uint32_t required_bit = intid % 32; // Which bit number (0..31) is the one to set
	uint32_t value = (1u << required_bit);

	mmio_write(&GICD->ISENABLER[reg_idx], value);

	return GIC_SUCCESS;
}

/**
 * @brief Disable forwarding of an interrupt.
 *
 * Writes to GICD_ICENABLERn. The register is write-1-to-clear, so no
 * read-modify-write is needed. Other interrupts are not affected.
 *
 * For @p intid < 32 (SGIs and PPIs) the register is banked, so this only
 * affects the calling core.
 *
 * @param intid Interrupt ID to disable
 *
 * @return Status of the operation
 * @retval GIC_SUCCESS The operation completed successfully
 * @retval GIC_ERROR_INVALID_INTID @p intid is out of range
 */
gic_status_t gic_disable_irq(uint32_t intid)
{
	GIC_CHECK_INTID(intid);

	uint32_t reg_idx = intid / 32;      // Which ICENALERn to write
	uint32_t required_bit = intid % 32; // Which bit number (0..31) is the one to set
	uint32_t value = (1u << required_bit);

	mmio_write(&GICD->ICENABLER[reg_idx], value);

	return GIC_SUCCESS;
}

/**
 * @brief Check whether an interrupt is enabled.
 *
 * Reads back GICD_ISENABLERn. Useful to verify that an enable write was not
 * silently ignored (for example due to security group restrictions).
 *
 * For @p intid < 32 this reports the state for the calling core.
 *
 * @param intid Interrupt ID to query.
 *
 * @return Interrupt state
 * @retval 1 The interrupt is enabled
 * @retval 0 The interrupt is disabled or @p intid is out of range
 */
int gic_is_enabled(uint32_t intid)
{
	if (intid >= s_num_intids)
		return 0;

	uint32_t reg_idx = intid / 32;      // Which ISENABLERn to read
	uint32_t required_bit = intid % 32; // Which bit number (0..31) is the one to set
	uint32_t mask = (1u << required_bit);

	uint32_t current_value = mmio_read(&GICD->ISENABLER[reg_idx]);
	return ((current_value & mask) >> required_bit);
}

/**
 * @brief Acknowledge the highest priority pending interrupt.
 *
 * Reads GICC_IAR, which marks the interrupt as active. Must be called at the
 * start of the IRQ handler.
 *
 * The raw register value is returned, not just the ID. For SGIs, bits [12:10]
 * hold the source CPU ID, and the same value has to be written back to
 * GICC_EOIR. Use GIC_IAR_INTID() and GIC_IAR_CPUID() to decode it.
 *
 * @code
 * uint32_t iar   = gic_acknowledge();
 * uint32_t intid = GIC_IAR_INTID(iar);
 * if (intid == GIC_INTID_SPURIOUS) return;
 * dispatch(intid);
 * gic_end_of_interrupt(iar);
 * @endcode
 *
 * @return The raw GICC_IAR value. If @p GIC_IAR_INTID() of it is
 *         @p GIC_INTID_SPURIOUS no interrupt was pending, and the value must
 *         not be passed to @p gic_end_of_interrupt()
 */
uint32_t gic_acknowledge()
{
	return mmio_read(&GICC->IAR);
}

/**
 * @brief Signal the end of interrupt handling.
 *
 * Writes GICC_EOIR. With the default EOImode (0), this both drops the
 * running priority and deactivates the interrupt.
 *
 * @param iar The raw value previously returned by gic_acknowledge(), passed
 *            back unchanged (including the CPU ID field for SGIs).
 *
 * @return Status of the operation
 * @retval GIC_SUCCESS The operation completed successfully
 * @retval GIC_ERROR_INVALID_INTID @p iar holds an incorrect interrupt ID (corrupted or made-up value, GIC_INTID_SPURIOUS)
 */
gic_status_t gic_end_of_interrupt(uint32_t iar)
{
	GIC_CHECK_INTID(GIC_IAR_INTID(iar));

	mmio_write(&GICC->EOIR, iar);

	return GIC_SUCCESS;
}

/**
 * @brief Set the priority of an interrupt.
 *
 * Writes GICD_IPRIORITYRn with a single byte store, so it is atomic with
 * respect to the neighbouring interrupts. Lower values mean higher priority.
 * The GIC-400 only implements the upper bits of the 8-bit field; the
 * unimplemented lower bits read as zero.
 *
 * For @p intid < 32 the register is banked, so this only affects the calling core.
 *
 * @param intid Interrupt ID.
 * @param prio  Priority value (0 = highest).
 *
 * @return Status of the operation
 * @retval GIC_SUCCESS The operation completed successfully
 * @retval GIC_ERROR_INVALID_INTID @p intid is out of range
 */
gic_status_t gic_set_priority(uint32_t intid, uint8_t prio)
{
	GIC_CHECK_INTID(intid);

	mmio_write8(&GICD->IPRIORITYR[intid], prio);

	return GIC_SUCCESS;
}

/**
 * @brief Choose which cores an SPI is routed to.
 *
 * Writes GICD_ITARGETSRn with a single byte store. Only meaningful for SPIs
 * (ID 32 and above); the register is read-only for SGIs and PPIs, which are
 * fixed to their own core.
 *
 * @param intid    SPI interrupt ID.
 * @param cpu_mask Bit mask of target cores (bit n = core n).
 *
 * @return Status of the operation
 * @retval GIC_SUCCESS The operation completed successfully
 * @retval GIC_ERROR_INVALID_INTID @p intid is out of range
 * @retval GIC_ERROR_INTID_NOT_SPI `intid < 32`
 * @retval GIC_ERROR_INVALID_CPU_MASK @p cpu_mask is invalid (at least one of the upper 4 bits is set)
 */
gic_status_t gic_set_target(uint32_t intid, uint8_t cpu_mask)
{
	GIC_CHECK_INTID(intid);

	GIC_CHECK_CPU_MASK(cpu_mask);

	if (intid < GIC_INTID_SPI_BASE)
		return GIC_ERROR_INTID_NOT_SPI;

	mmio_write8(&GICD->ITARGETSR[intid], cpu_mask);

	return GIC_SUCCESS;
}

/**
 * @brief Configure an interrupt as level- or edge-sensitive.
 *
 * Writes GICD_ICFGRn (2 bits per interrupt). The configuration is read-only
 * for SGIs and implementation-defined for PPIs, so this is intended for SPIs
 * and returns an error if it's not used for SPIs.
 *
 * @warning NOT atomic. The register holds 16 interrupts per word, so this is
 *          a read-modify-write. Call it during initialization, or with
 *          interrupts masked and no other core touching the same word.
 *
 * @warning Changing the trigger of an enabled interrupt is **UNPREDICTABLE**.
 * 			Always disable the interrupt before changing its trigger.
 *
 * @param intid Interrupt ID.
 * @param edge  Non-zero for edge-triggered, 0 for level-sensitive.
 *
 * @return Status of the operation
 * @retval GIC_SUCCESS The operation completed successfully
 * @retval GIC_ERROR_INTID_NOT_SPI `intid < 32`
 * @retval GIC_ERROR_INVALID_INTID @p intid is out of range
 */
gic_status_t gic_set_trigger(uint32_t intid, int edge)
{
	GIC_CHECK_INTID(intid);

	if (intid < GIC_INTID_SPI_BASE)
		return GIC_ERROR_INTID_NOT_SPI;

	uint32_t reg_idx = intid / 16; // Which ICFGRn to write
	uint32_t shift = (intid % 16) * 2 + 1;

	mmio_write_field(&GICD->ICFGR[reg_idx], (edge != 0), 0x1, shift);

	return GIC_SUCCESS;
}

/**
 * @brief Check whether an interrupt is pending.
 *
 * Reads GICD_ISPENDRn. For @p intid < 32 this reports the state for the
 * calling core.
 *
 * @note For SGIs and PPIs a 1 means pending on this processor, and for
 * SPIs it means pending on at least one processor
 *
 * @param intid Interrupt ID to query.
 *
 * @return Interrupt state
 * @retval 1 The interrupt is pending
 * @retval 0 The interrupt is not pending or @p intid is out of range
 */
int gic_is_pending(uint32_t intid)
{
	if (intid >= s_num_intids)
		return 0;

	uint32_t reg_idx = intid / 32;
	uint32_t bit_required = intid % 32;

	return mmio_read_field(&GICD->ISPENDR[reg_idx], 1, bit_required);
}

/**
 * @brief Check whether an interrupt is active.
 *
 * Reads GICD_ISACTIVERn. An interrupt becomes active when it is acknowledged
 * (gic_acknowledge()) and stops being active when gic_end_of_interrupt() is
 * called. Useful to debug handlers that forget the EOI.
 *
 * For @p intid < 32 this reports the state for the calling core.
 *
 * @param intid Interrupt ID to query.
 *
 * @return Interrupt state
 * @retval 1 The interrupt is active (or active and pending)
 * @retval 0 The interrupt is not active or @p intid is out of range
 */
int gic_is_active(uint32_t intid)
{
	if (intid >= s_num_intids)
		return 0;

	uint32_t reg_idx = intid / 32;
	uint32_t bit_required = intid % 32;

	return mmio_read_field(&GICD->ISACTIVER[reg_idx], 1, bit_required);
}

/**
 * @brief Set the pending state of a PPI or SPI by software.
 *
 * Writes GICD_ISPENDRn (write-1-to-set), so no read-modify-write is needed.
 * For PPIs (@p intid 16-31) the register is banked, so this only affects the
 * calling core. The interrupt is only signalled to a core if it is also
 * enabled (gic_enable_irq()) and its priority is high enough.
 *
 * SGIs are not handled here, writes to GICD_ISPENDRn are ignored for them;
 * @see gic_send_sgi().
 *
 * @note Edge-triggered: the interrupt becomes pending, or active and pending
 *       if it was active. Has no effect if it is already pending.
 * @note Level-sensitive: a pending state set this way stays until it is cleared
 *       with gic_clear_pending(). If the source signal is asserted, the
 *       interrupt is pending regardless, and this write has no effect.
 *
 * @param intid Interrupt ID (16 or above).
 *
 * @return Status of the operation
 * @retval GIC_SUCCESS The operation completed successfully
 * @retval GIC_ERROR_INVALID_INTID @p intid is out of range
 * @retval GIC_ERROR_INTID_IS_SGI @p intid is an SGI (below 16)
 */
gic_status_t gic_set_pending(uint32_t intid)
{
	GIC_CHECK_INTID(intid);

	if (intid < GIC_INTID_PPI_BASE)
		return GIC_ERROR_INTID_IS_SGI;

	uint32_t reg_idx = intid / 32;
	uint32_t bit = intid % 32;

	mmio_write(&GICD->ISPENDR[reg_idx], 1u << bit);

	return GIC_SUCCESS;
}

/**
 * @brief Clear the pending state of a PPI or SPI.
 *
 * Writes GICD_ICPENDRn (write-1-to-clear), so no read-modify-write is needed.
 * For PPIs (@p intid 16-31) the register is banked, so this only affects the
 * calling core. SGIs are not handled here; @see gic_clear_sgi_pending().
 *
 * @note For a level-sensitive interrupt, this only clears a pending state that
 *       was set by @p gic_set_pending() (GICD_ISPENDRn). If the source is still
 *       asserting the signal, the interrupt stays pending: deassert it in the
 *       peripheral instead.
 *
 * @warning Clearing doesn’t deactivate: An active-and-pending interrupt goes
 * back to active, not inactive. The only thing that ends the active state is EOIR,
 * so this function is not a substitute for @p gic_end_of_interrupt()
 *
 * @param intid Interrupt ID (16 or above).
 *
 * @return Status of the operation
 * @retval GIC_SUCCESS The operation completed successfully
 * @retval GIC_ERROR_INVALID_INTID @p intid is out of range
 * @retval GIC_ERROR_INTID_IS_SGI @p intid is an SGI (below 16)
 */
gic_status_t gic_clear_pending(uint32_t intid)
{
	GIC_CHECK_INTID(intid);

	if (intid < GIC_INTID_PPI_BASE)
		return GIC_ERROR_INTID_IS_SGI;

	uint32_t reg_idx = intid / 32;
	uint32_t bit = intid % 32;

	mmio_write(&GICD->ICPENDR[reg_idx], 1u << bit);

	return GIC_SUCCESS;
}

/**
 * @brief Clear the pending state of an SGI.
 *
 * Writes GICD_CPENDSGIRn (write-1-to-clear). Each SGI has one byte in these
 * registers with one bit per source core, because the same SGI can be pending
 * on the calling core once per sender. Only the pending states coming from the
 * cores in @p src_cpu_mask are cleared; the write is a single word store with
 * zeros in the other bytes, so other SGIs are not affected and no
 * read-modify-write is needed.
 *
 * The register is banked, so this only affects the calling core.
 *
 * @warning Like gic_clear_pending(), this does not deactivate the interrupt.
 *          Only gic_end_of_interrupt() does.
 *
 * @param intid        SGI interrupt ID (0-15).
 * @param src_cpu_mask Bit mask of source cores whose pending SGI is cleared
 *                     (bit n = core n). Use 0xF to clear it for every core.
 *
 * @return Status of the operation
 * @retval GIC_SUCCESS The operation completed successfully
 * @retval GIC_ERROR_INTID_NOT_SGI @p intid is not an SGI (above 15)
 * @retval GIC_ERROR_INVALID_CPU_MASK @p src_cpu_mask is invalid (at least one of the upper 4 bits is set)
 */
gic_status_t gic_clear_sgi_pending(uint32_t intid, uint8_t src_cpu_mask)
{
	if (intid >= GIC_INTID_PPI_BASE)
		return GIC_ERROR_INTID_NOT_SGI;

	GIC_CHECK_CPU_MASK(src_cpu_mask);

	uint32_t reg_idx = intid / 4;     // 4 SGIs (bytes) per CPENDSGIRn
	uint32_t shift = (intid % 4) * 8; // Which byte of the word belongs to this SGI

	mmio_write(&GICD->CPENDSGIR[reg_idx], ((uint32_t) src_cpu_mask) << shift);

	return GIC_SUCCESS;
}

/**
 * @brief Send a Software Generated Interrupt.
 *
 * Writes GICD_SGIR with the requested TargetListFilter. A `dsb` is issued
 * before the write so that stores made earlier (for example the data the
 * target core is going to read) are visible to the target core when the SGI
 * arrives.
 *
 * The SGI is only delivered to a core that has enabled it (bit @p intid of its
 * banked GICD_ISENABLER0) and whose CPU interface is enabled. Both are done on
 * each core by gic_cpu_init() and gic_enable_irq().
 *
 * @param intid    SGI interrupt ID (0-15).
 * @param target   Which cores to send it to.
 * @param cpu_mask Bit mask of target cores (bit n = core n). Only used with
 *                 `GIC_SGI_TARGET_LIST`.
 *
 * @return Status of the operation
 * @retval GIC_SUCCESS The SGI was requested successfully
 * @retval GIC_ERROR_INTID_NOT_SGI @p intid is not an SGI (above 15)
 * @retval GIC_ERROR_INVALID_ARG @p target is not a valid @p gic_sgi_target_t
 * @retval GIC_ERROR_INVALID_CPU_MASK @p cpu_mask is invalid (at least one of the upper 4 bits is set)
 */
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
		filter = GICD_SGIR_FILTER_LIST;
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