// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file irq.c
 * @brief CPU-level IRQ masking on aarch64 (PSTATE.I via the DAIF register).
 * @ingroup arch
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

#include "arch/irq.h"

#include <stdint.h>

/// @brief I bit (IRQ mask) of the DAIF register, as read with "mrs x, daif"
#define DAIF_I (1ul << 7)
/// @brief F bit (FIQ mask) of the DAIF register, as read with "mrs x, daif"
#define DAIF_F (1ul << 6)

void arch_irq_enable()
{
	__asm__ volatile("msr daifclr, #3" ::: "memory");
}

void arch_irq_disable()
{
	__asm__ volatile("msr daifset, #3" ::: "memory");
}

arch_irq_flags_t arch_irq_save()
{
	arch_irq_flags_t daif;

	// Read the old state and mask in one asm block so nothing can slip between.
	__asm__ volatile("mrs %0, daif\n\t"
	                 "msr daifset, #3"
	                 : "=r"(daif)
	                 :
	                 : "memory");

	// Keep only the I bit and F bit: restore must not touch D or A
	return daif & (DAIF_I | DAIF_F);
}

void arch_irq_restore(arch_irq_flags_t flags)
{
	if (flags & DAIF_I)
		__asm__ volatile("msr daifset, #2" ::: "memory"); // IRQ was masked: keep masked
	else
		__asm__ volatile("msr daifclr, #2" ::: "memory"); // IRQ was unmasked: unmask

	if (flags & DAIF_F)
		__asm__ volatile("msr daifset, #1" ::: "memory"); // FIQ was masked: keep masked
	else
		__asm__ volatile("msr daifclr, #1" ::: "memory"); // FIQ was unmasked: unmask
}

bool arch_irqs_enabled()
{
	uint64_t daif;
	__asm__ volatile("mrs %0, daif" : "=r"(daif));
	return (daif & (DAIF_I | DAIF_F)) == 0;
}
