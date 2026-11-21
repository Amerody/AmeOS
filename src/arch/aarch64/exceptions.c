// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file exceptions.c
 * @brief aarch64 exception handlers called from the vector table, plus the
 *        trap-frame helpers declared in include/arch/context.h.
 * @ingroup arch
 */

#include "exceptions.h"
#include "arch/arch.h"
#include "arch/context.h"

#include <hal/hal_irq.h>
#include <kernel/kprintf.h>

#include <stdint.h>

/// @brief Exception class (EC) field of an ESR_EL1 value, bits [31:26]
#define ESR_EC(esr) ((uint32_t) (((esr) >> 26) & 0x3Fu))
/// @brief Instruction-specific syndrome (ISS) field of an ESR_EL1 value, bits [24:0]
#define ESR_ISS(esr) ((uint32_t) ((esr) & 0x1FFFFFFu))

/// @brief Exception class: unknown reason
#define EC_UNKNOWN 0x00u
/// @brief Exception class: SVC instruction executed in AArch64 state
#define EC_SVC64 0x15u
/// @brief Exception class: instruction abort from a lower exception level
#define EC_IABORT_LOWER 0x20u
/// @brief Exception class: instruction abort taken without a change of exception level
#define EC_IABORT_CURRENT 0x21u
/// @brief Exception class: PC alignment fault
#define EC_PC_ALIGNMENT 0x22u
/// @brief Exception class: data abort from a lower exception level
#define EC_DABORT_LOWER 0x24u
/// @brief Exception class: data abort taken without a change of exception level
#define EC_DABORT_CURRENT 0x25u
/// @brief Exception class: SP alignment fault
#define EC_SP_ALIGNMENT 0x26u
/// @brief Exception class: BRK instruction executed in AArch64 state
#define EC_BRK64 0x3Cu

/**
 * @brief Reads ESR_EL1 (Exception Syndrome Register).
 *
 * Holds the cause of the last exception taken to EL1
 * (EC [31:26], IL [25], ISS [24:0]). Must run at EL1 or higher.
 *
 * @return Contents of ESR_EL1.
 */
static inline uint64_t read_esr_el1()
{
	uint64_t v;
	__asm__ volatile("mrs %0, esr_el1" : "=r"(v));
	return v;
}

/**
 * @brief Reads FAR_EL1 (Fault Address Register).
 *
 * Holds the faulting virtual address for aborts, PC alignment faults
 * and watchpoints taken to EL1. UNKNOWN for other exceptions.
 * Must run at EL1 or higher.
 *
 * @return Contents of FAR_EL1.
 */
static inline uint64_t read_far_el1()
{
	uint64_t v;
	__asm__ volatile("mrs %0, far_el1" : "=r"(v));
	return v;
}

/**
 * @brief Human-readable name of an exception class, for the panic output.
 *
 * @param ec Exception class, from ESR_EC()
 * @return A static string; "other" for classes without a name here
 */
static const char* ec_name(uint32_t ec)
{
	switch (ec)
	{
	case EC_UNKNOWN:
		return "unknown reason";
	case EC_SVC64:
		return "SVC (AArch64)";
	case EC_IABORT_LOWER:
		return "instruction abort (lower EL)";
	case EC_IABORT_CURRENT:
		return "instruction abort (same EL)";
	case EC_PC_ALIGNMENT:
		return "PC alignment fault";
	case EC_DABORT_LOWER:
		return "data abort (lower EL)";
	case EC_DABORT_CURRENT:
		return "data abort (same EL)";
	case EC_SP_ALIGNMENT:
		return "SP alignment fault";
	case EC_BRK64:
		return "BRK instruction";
	default:
		return "other";
	}
}

/**
 * @brief Tells whether FAR_EL1 holds a meaningful address for an exception class.
 *
 * @param ec Exception class, from ESR_EC()
 * @retval 1 Instruction abort, data abort or PC alignment fault: FAR_EL1 is meaningful
 * @retval 0 Any other class
 */
static int far_is_valid(uint32_t ec)
{
	return ec == EC_IABORT_LOWER || ec == EC_IABORT_CURRENT || ec == EC_PC_ALIGNMENT || ec == EC_DABORT_LOWER || ec == EC_DABORT_CURRENT;
}

/* ---- trap-frame helpers (include/arch/context.h) ----------------------- */

uintptr_t arch_trap_frame_pc(const struct trap_frame* tf)
{
	return (uintptr_t) tf->elr;
}

void arch_trap_frame_dump(const struct trap_frame* tf)
{
	for (unsigned i = 0; i < 30; i += 2)
		kprintf("x%u=%lx x%u=%lx\n", i, tf->x[i], i + 1, tf->x[i + 1]);
	kprintf("x30=%lx\n", tf->x[30]);
	kprintf("ELR=%lx SPSR=%lx SP_EL0=%lx\n", tf->elr, tf->spsr, tf->sp);
}

/**
 * @brief Prints everything useful about the exception and stops. Never returns.
 *
 * Used for every exception we cannot recover from. Prints the exception kind, the
 * exception class and syndrome from ESR_EL1, FAR_EL1 when it is meaningful (see
 * far_is_valid()) and the registers in @p tf, then calls arch_halt().
 *
 * @param kind Exception kind for the first line, e.g. "SYNC"
 * @param tf Frame saved by vectors.S
 */
__attribute__((noreturn)) static void exception_panic(const char* kind, struct trap_frame* tf)
{
	uint64_t esr = read_esr_el1();
	uint32_t ec = ESR_EC(esr);

	kprintf("\n*** %s EXCEPTION ***\n", kind);
	kprintf("EC=%x (%s) ISS=%x ESR=%lx\n", ec, ec_name(ec), ESR_ISS(esr), esr);

	if (far_is_valid(ec))
		kprintf("FAR=%lx\n", read_far_el1());

	arch_trap_frame_dump(tf);
	arch_halt();
}

/* ---- the four handlers called from the vector table ------------------- */

void sync_handler(struct trap_frame* tf)
{
	uint64_t esr = read_esr_el1();
	uint32_t ec = ESR_EC(esr);

	if (ec == EC_SVC64)
	{
		uint32_t syscall_nr = (uint32_t) (esr & 0xFFFFu); /* the immediate of "svc #imm" */
		(void) syscall_nr;
		/* Later: core declares the upcall (include/kernel/syscall.h) and the
		 * arch calls it here, e.g. syscall_dispatch(syscall_nr, tf). */
		return; /* ELR already points past the SVC, so just return */
	}

	exception_panic("SYNC", tf);
}

void irq_handler(struct trap_frame* tf)
{
	(void) tf;

	/* The HAL acknowledges the interrupt, runs the registered handler and
	 * signals end of interrupt. Nothing controller-specific lives here.
	 *
	 * Later, preemption goes right after this call: if core says a reschedule
	 * is needed, switch here, once the interrupt has been completed. */
	hal_irq_dispatch();
}

void fiq_handler(struct trap_frame* tf)
{
	/* Everything is Group 1 (IRQ), so a FIQ means the GIC/armstub setup is not
	 * what we expect. */
	exception_panic("FIQ", tf);
}

void serror_handler(struct trap_frame* tf)
{
	/* SError is asynchronous: ELR does not point at the faulting instruction
	 * and we cannot safely resume. */
	exception_panic("SERROR", tf);
}