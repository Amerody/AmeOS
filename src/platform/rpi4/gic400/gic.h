/**
 * @file gic.h
 * @brief Public interface of the GICv2 (GIC-400) interrupt controller driver.
 * @ingroup gic
 *
 * Register names mentioned in the documentation follow the ARM GICv2
 * specification (ARM IHI 0048B) and the GIC-400 TRM.
 *
 * Interrupt ID ranges:
 *  - 0-15:    SGIs (Software Generated Interrupts), per core
 *  - 16-31:   PPIs (Private Peripheral Interrupts), per core
 *  - 32-1019: SPIs (Shared Peripheral Interrupts), routable to any core
 *  - 1020-1023: special values (1023 = spurious)
 */

#pragma once

#include <stdint.h>

/// @brief GIC Status codes
typedef enum
{
	GIC_SUCCESS = 0, ///< Operation Success

	GIC_ERROR_INVALID_INTID = -1,    ///< An invalid interrupt id has been provided
	GIC_ERROR_INTID_NOT_SPI = -2,    ///< An interrupt id NOT corresponding to an SPI has been provided where it shouldn't be
	GIC_ERROR_INTID_NOT_SGI = -3,    ///< An interrupt id NOT corresponding to an SGI has been provided where it shouldn't be
	GIC_ERROR_INTID_IS_SGI = -4,     ///< An interrupt id corresponding to an SGI has been provided where it shouldn't be
	GIC_ERROR_INVALID_CPU_MASK = -5, ///< An invalid CPU mask has been provided

	GIC_ERROR_INVALID_ARG = -999, ///< An invalid argument has been provided (shouldn't happen)

} gic_status_t;

/// @brief Target selection for gic_send_sgi() (GICD_SGIR)
typedef enum
{
	GIC_SGI_TARGET_LIST,         ///< Send to the cores in the cpu_mask
	GIC_SGI_TARGET_ALL_BUT_SELF, ///< Send to every core except the calling one (cpu_mask ignored)
	GIC_SGI_TARGET_SELF,         ///< Send only to the calling core (cpu_mask ignored)
} gic_sgi_target_t;

/// @brief Extracts the interrupt ID (bits [9:0]) from a raw GICC_IAR value
#define GIC_IAR_INTID(iar) ((iar) & 0x3FFu)

/// @brief Extracts the source CPU ID (bits [12:10]) from a raw GICC_IAR value. Only meaningful for SGIs
#define GIC_IAR_CPUID(iar) (((iar) >> 10) & 0x7u)

/// @brief Most interrupt IDs the driver handles: its register arrays cover IDs 0-511
#define GIC_MAX_NUM_INTERRUPTS 512u
#define GIC_INTID_SPURIOUS 1023u ///< Returned by GICC_IAR when nothing is pending

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
void gic_init();

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
void gic_cpu_init();

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
gic_status_t gic_enable_irq(uint32_t intid);

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
gic_status_t gic_disable_irq(uint32_t intid);

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
int gic_is_enabled(uint32_t intid);

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
uint32_t gic_acknowledge();

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
gic_status_t gic_end_of_interrupt(uint32_t iar);

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
gic_status_t gic_set_priority(uint32_t intid, uint8_t prio);

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
gic_status_t gic_set_target(uint32_t intid, uint8_t cpu_mask);

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
gic_status_t gic_set_trigger(uint32_t intid, int edge);

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
int gic_is_pending(uint32_t intid);

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
int gic_is_active(uint32_t intid);

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
gic_status_t gic_set_pending(uint32_t intid);

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
gic_status_t gic_clear_pending(uint32_t intid);

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
gic_status_t gic_clear_sgi_pending(uint32_t intid, uint8_t src_cpu_mask);

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
gic_status_t gic_send_sgi(uint32_t intid, gic_sgi_target_t target, uint8_t cpu_mask);

/**
 * @brief Number of interrupt IDs this GIC implements. 0 before gic_init().
 *
 * @return 32 * (GICD_TYPER.ITLinesNumber + 1), capped at GIC_MAX_NUM_INTERRUPTS.
 *         Valid IDs are 0 to this value minus 1.
 */
uint32_t gic_num_intids();