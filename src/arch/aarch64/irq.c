/**
 * @file irq.c
 * @brief CPU-level IRQ masking on aarch64 (PSTATE.I via the DAIF register).
 *
 * Only the I bit (IRQ) is touched. Debug, SError and FIQ masks are left alone.
 * The contract and its documentation live in include/arch/irq.h.
 *
 * DAIF bits as seen by "mrs x, daif": D=9, A=8, I=7, F=6.
 * The "msr daifset/daifclr, #imm" immediate uses a different encoding:
 * D=8, A=4, I=2, F=1, so #2 selects the I bit.
 *
 * Every asm statement has a "memory" clobber, which is what makes these
 * functions compiler barriers: the compiler cannot move memory accesses from
 * inside a critical section to outside it.
 */

#include <arch/irq.h>

#include <stdint.h>

#define DAIF_I (1ul << 7)

void arch_irq_enable()
{
	__asm__ volatile("msr daifclr, #2" ::: "memory");
}

void arch_irq_disable()
{
	__asm__ volatile("msr daifset, #2" ::: "memory");
}

arch_irq_flags_t arch_irq_save()
{
	arch_irq_flags_t daif;

	/* Read the old state and mask in one asm block so nothing can slip between. */
	__asm__ volatile("mrs %0, daif\n\t"
	                 "msr daifset, #2"
	                 : "=r"(daif)
	                 :
	                 : "memory");

	/* Keep only the I bit: restore must not touch D, A or F. */
	return daif & DAIF_I;
}

void arch_irq_restore(arch_irq_flags_t flags)
{
	if (flags & DAIF_I)
		__asm__ volatile("msr daifset, #2" ::: "memory"); /* were masked: stay masked */
	else
		__asm__ volatile("msr daifclr, #2" ::: "memory"); /* were unmasked: unmask */
}

bool arch_irqs_enabled()
{
	uint64_t daif;
	__asm__ volatile("mrs %0, daif" : "=r"(daif));
	return (daif & DAIF_I) == 0;
}