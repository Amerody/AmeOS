/**
 * @file irq.h
 * @brief CPU-level interrupt masking (this CPU only).
 *
 * This is NOT the interrupt controller. An interrupt reaches the kernel only if
 * all three of these allow it:
 *   1. the peripheral raises it,
 *   2. the interrupt controller has the source enabled and routed
 *      (hal_irq_enable(), implemented by the platform),
 *   3. the CPU has IRQs unmasked (this header).
 *
 * On aarch64 this is the PSTATE.I bit (DAIF register). FIQ, SError and debug
 * exceptions are not affected by anything here.
 *
 * Implemented by src/arch/<ARCH>/.
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief Opaque saved interrupt-mask state, produced by arch_irq_save() and
	 *        consumed by arch_irq_restore(). Do not interpret its value.
	 */
	typedef unsigned long arch_irq_flags_t;

	/**
	 * @brief Unmask IRQs on this CPU.
	 *
	 * Not nesting-safe: use it only where IRQs are known to have been masked by
	 * you (for example once in kmain() at the end of boot). Elsewhere use
	 * arch_irq_save()/arch_irq_restore().
	 *
	 * Do not call from an IRQ handler: handlers run with IRQs masked and nested
	 * interrupts are not supported.
	 *
	 * Acts as a compiler barrier.
	 */
	void arch_irq_enable();

	/**
	 * @brief Mask IRQs on this CPU.
	 *
	 * @warning Not nesting-safe, see arch_irq_enable(). Acts as a compiler barrier.
	 */
	void arch_irq_disable();

	/**
	 * @brief Mask IRQs and return the previous mask state.
	 *
	 * @note Nesting-safe: this is the building block for critical sections and, later,
	 *       spinlocks. Always pair with arch_irq_restore() on the same CPU.
	 *
	 * @code
	 * arch_irq_flags_t f = arch_irq_save();
	 * // ... critical section, cannot be interrupted on this CPU ...
	 * arch_irq_restore(f);
	 * @endcode
	 *
	 * Acts as a compiler barrier.
	 *
	 * @return State to hand back to arch_irq_restore().
	 */
	arch_irq_flags_t arch_irq_save();

	/**
	 * @brief Restore the mask state captured by arch_irq_save().
	 *
	 * IRQs become unmasked only if they were unmasked when the matching save ran.
	 * Acts as a compiler barrier.
	 *
	 * @param flags Value returned by the matching arch_irq_save().
	 */
	void arch_irq_restore(arch_irq_flags_t flags);

	/**
	 * @brief Are IRQs currently unmasked on this CPU?
	 *
	 * Intended for assertions (for example "must be called with IRQs masked");
	 * do not use it to make locking decisions.
	 */
	bool arch_irqs_enabled();

#ifdef __cplusplus
}
#endif