/**
 * @file arch.h
 * @brief CPU-architecture contract: bring-up, idle and halt.
 * @ingroup arch
 *
 * Implemented by src/arch/\<ARCH\>/. Core and portable drivers may include this
 * header; they must never include anything from src/arch/ directly.
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief One-time CPU setup on the boot CPU.
	 *
	 * Called once from kmain(), before platform_init().
	 *
	 * @pre boot code has already left the firmware's exception level for
	 * the kernel's (EL1 on aarch64), set up the boot stack and cleared .bss.
	 *
	 * @post the exception vector table is installed, so any fault taken
	 * from now on reaches the arch exception handlers instead of a wild jump.
	 *
	 * Interrupts stay masked. kmain() unmasks them with arch_irq_enable() only
	 * after the interrupt controller and timer have been initialised.
	 */
	void arch_init();

	/**
	 * @brief Short architecture name for the boot banner, e.g. "aarch64".
	 * @return A static string; never NULL.
	 */
	const char* arch_name();

	/**
	 * @brief Wait until something wakes the CPU (wfi on aarch64).
	 *
	 * May return spuriously, so callers must loop and re-check their condition.
	 * Used by the idle loop.
	 *
	 * On aarch64, a pending interrupt wakes the CPU even while IRQs are masked, so
	 * the sequence "mask IRQs; check for work; arch_cpu_idle(); unmask IRQs" has
	 * no lost-wakeup race.
	 */
	void arch_cpu_idle();

	/**
	 * @brief Stop this CPU permanently. Never returns.
	 *
	 * Masks interrupts and parks the CPU in a low-power loop. Used by panic() and
	 * other unrecoverable paths. Safe to call from any context, including
	 * exception handlers.
	 */
	void arch_halt() __attribute__((noreturn));

#ifdef __cplusplus
}
#endif
