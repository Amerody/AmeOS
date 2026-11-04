#include "kernel/exceptions.h"

#include "arch/aarch64/gic.h"
#include "kernel/kprintf.h"

#include <stdint.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

#define READ_SYSREG(name)                              \
	({                                                 \
		uint64_t _v;                                   \
		__asm__ volatile("mrs %0, " #name : "=r"(_v)); \
		_v;                                            \
	})

#define WRITE_SYSREG(name, value) __asm__ volatile("msr " #name ", %0" ::"r"((uint64_t)(value)))

#define ESR_EC(esr) ((uint32_t)(((esr) >> 26) & 0x3Fu))
#define ESR_ISS(esr) ((uint32_t)((esr) & 0x1FFFFFFu))

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
	switch(ec)
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

/// FAR_EL1 only holds a meaningful address for these exception classes
static int far_is_valid(uint32_t ec)
{
	return ec == EC_IABORT_LOWER || ec == EC_IABORT_CURRENT || ec == EC_PC_ALIGNMENT || ec == EC_DABORT_LOWER || ec == EC_DABORT_CURRENT;
}

/// Print everything useful about the exception and stop. Used for every exception we can't recover from.
__attribute__((noreturn)) static void exception_panic(const char* kind, struct trap_frame* tf)
{
	uint64_t esr = READ_SYSREG(esr_el1);
	uint32_t ec = ESR_EC(esr);

	kprintf("\n*** %s EXCEPTION ***\n", kind);
	kprintf("EC=%x (%s) ISS=%x ESR=%lx\n", ec, ec_name(ec), ESR_ISS(esr), esr);
	kprintf("ELR=%lx\n", tf->elr);

	if(far_is_valid(ec))
		kprintf("FAR=%lx\n", READ_SYSREG(far_el1));

	for(;;)
		__asm__ volatile("wfe");
}

// ---------------------------------------------------------------------------
// IRQ dispatch table
// ---------------------------------------------------------------------------

static irq_fn_t s_irq_table[GIC_MAX_NUM_INTERRUPTS];

/// Register the function called when interrupt @p intid fires. Pass NULL to unregister.
gic_status_t irq_register(uint32_t intid, irq_fn_t fn)
{
	if(intid >= GIC_MAX_NUM_INTERRUPTS)
		return GIC_ERROR_INVALID_INTID;

	s_irq_table[intid] = fn;
	return GIC_SUCCESS;
}
// ---------------------------------------------------------------------------
// The four handlers called from the vector table
// ---------------------------------------------------------------------------

void sync_handler(struct trap_frame* tf)
{
	uint64_t esr = READ_SYSREG(esr_el1);
	uint32_t ec = ESR_EC(esr);

	if(ec == EC_SVC64)
	{
		uint32_t syscall_nr = (uint32_t)(esr & 0xFFFFu); // the immediate of "svc #imm"
		(void)syscall_nr;
		// syscall_dispatch(syscall_nr, tf);
		return; // ELR already points to the instruction after the SVC, so just return
	}

	exception_panic("SYNC", tf);
}

void irq_handler(struct trap_frame* tf)
{
	// 1. Acknowledge: the GIC tells us which interrupt it is, and marks it active
	uint32_t iar = gic_acknowledge();
	uint32_t intid = GIC_IAR_INTID(iar);

	// Nothing was pending (it was taken by another core, or withdrawn): do NOT write EOIR
	if(intid == GIC_INTID_SPURIOUS)
		return;

	// 2. Dispatch
	if(intid < GIC_MAX_NUM_INTERRUPTS && s_irq_table[intid] != 0)
		s_irq_table[intid](intid, tf);
	else
		kprintf("Unhandled IRQ %u\n", intid);

	// 3. End of interrupt. Must come after the device handler has deasserted a level-sensitive
	//    source, otherwise the interrupt fires again immediately. Also do it BEFORE any
	//    context switch you might do from here.
	gic_end_of_interrupt(iar);
}

void fiq_handler(struct trap_frame* tf)
{
	// Everything is Group 1 (IRQ), so a FIQ means the GIC/armstub setup isn't what we expect
	exception_panic("FIQ", tf);
}

void serror_handler(struct trap_frame* tf)
{
	// SError is asynchronous: ELR doesn't point at the faulting instruction and we can't safely resume
	exception_panic("SERROR", tf);
}

// ---------------------------------------------------------------------------
// Example user of the IRQ path: virtual timer tick (PPI 27)
// ---------------------------------------------------------------------------

#define IRQ_VTIMER 27u

static uint64_t s_timer_interval;

static void timer_irq(uint32_t intid, struct trap_frame* tf)
{
	(void)intid;
	(void)tf;

	// Re-arm first: the timer line is level-sensitive, and writing TVAL clears ISTATUS so it deasserts
	WRITE_SYSREG(cntv_tval_el0, s_timer_interval);

	kprintf("[TICK]\n");
}

void timer_init(uint32_t hz)
{
	s_timer_interval = READ_SYSREG(cntfrq_el0) / hz;

	irq_register(IRQ_VTIMER, timer_irq);
	gic_enable_irq(IRQ_VTIMER); // PPI: banked, so every core that wants ticks must do this itself

	WRITE_SYSREG(cntv_tval_el0, s_timer_interval);
	WRITE_SYSREG(cntv_ctl_el0, 1); // ENABLE=1, IMASK=0
	__asm__ volatile("isb");
}

// Boot order, after the vector table is installed (VBAR_EL1):
//
//   gic_init();                                   // boot core only
//   gic_cpu_init();                               // every core
//   timer_init(1);                                // 1 tick per second
//   __asm__ volatile("msr daifclr, #2" ::: "memory"); // finally unmask IRQs
