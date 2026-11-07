/**
 * @file exceptions.c
 * @brief aarch64 exception handlers called from the vector table, plus the
 *        trap-frame helpers declared in include/arch/context.h.
 */

#include "exceptions.h"
#include "arch/arch.h"
#include "arch/context.h"

#include <hal/hal_irq.h>
#include <kernel/kprintf.h>

#include <stdint.h>

/* ---- helpers ----------------------------------------------------------- */

#define READ_SYSREG(name)                              \
	({                                                 \
		uint64_t _v;                                   \
		__asm__ volatile("mrs %0, " #name : "=r"(_v)); \
		_v;                                            \
	})

#define ESR_EC(esr) ((uint32_t) (((esr) >> 26) & 0x3Fu))
#define ESR_ISS(esr) ((uint32_t) ((esr) & 0x1FFFFFFu))

#define EC_UNKNOWN 0x00u
#define EC_SVC64 0x15u
#define EC_IABORT_LOWER 0x20u
#define EC_IABORT_CURRENT 0x21u
#define EC_PC_ALIGNMENT 0x22u
#define EC_DABORT_LOWER 0x24u
#define EC_DABORT_CURRENT 0x25u
#define EC_SP_ALIGNMENT 0x26u
#define EC_BRK64 0x3Cu

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

/* FAR_EL1 only holds a meaningful address for these exception classes. */
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

/* Print everything useful about the exception and stop. Used for every
 * exception we cannot recover from. */
__attribute__((noreturn)) static void exception_panic(const char* kind, struct trap_frame* tf)
{
	uint64_t esr = READ_SYSREG(esr_el1);
	uint32_t ec = ESR_EC(esr);

	kprintf("\n*** %s EXCEPTION ***\n", kind);
	kprintf("EC=%x (%s) ISS=%x ESR=%lx\n", ec, ec_name(ec), ESR_ISS(esr), esr);

	if (far_is_valid(ec))
		kprintf("FAR=%lx\n", READ_SYSREG(far_el1));

	arch_trap_frame_dump(tf);
	arch_halt();
}

/* ---- the four handlers called from the vector table ------------------- */

void sync_handler(struct trap_frame* tf)
{
	uint64_t esr = READ_SYSREG(esr_el1);
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