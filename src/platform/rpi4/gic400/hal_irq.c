/**
 * @file hal_irq.c
 * @brief hal_irq implementation for the Raspberry Pi 4, on top of the GIC-400
 *        driver (gic.h).
 *
 * The contract and its documentation live in include/hal/hal_irq.h. IRQ numbers
 * are GIC interrupt IDs (0-15 SGIs, 16-31 PPIs, 32+ SPIs).
 *
 * This file only adds what the GIC driver does not have: the table of
 * registered handlers and the acknowledge / run / end-of-interrupt sequence.
 * The GIC itself is brought up by platform_init(), which must call
 *
 *     gic_init();       // distributor, once
 *     gic_cpu_init();   // this core's CPU interface
 *
 * before any function in this file is used.
 */

#include <arch/irq.h>
#include <hal/hal_irq.h>
#include <kernel/errno.h>

#include "gic.h"

#include <stddef.h>
#include <stdint.h>

#define GIC_INTID_SPECIAL_MIN 1020u /* 1020..1023 are never real sources */

struct irq_entry
{
	hal_irq_handler_t fn;
	void* arg;
};

/* Sized for the driver's maximum; gic_num_intids() is always <= this. */
static struct irq_entry s_handlers[GIC_MAX_NUM_INTERRUPTS];

void hal_irq_init()
{
	gic_init();
	gic_cpu_init();
}

int hal_irq_register(unsigned irq, hal_irq_handler_t handler, void* arg)
{
	/* gic_num_intids() is 0 until gic_init() has run, so this also rejects
	 * use before platform_init(). */
	if (handler == NULL || irq >= gic_num_intids())
		return -EINVAL;

	int rc = 0;
	arch_irq_flags_t f = arch_irq_save(); /* the table is also read from the IRQ path */
	if (s_handlers[irq].fn != NULL)
	{
		rc = -EBUSY;
	}
	else
	{
		s_handlers[irq].arg = arg;
		s_handlers[irq].fn = handler;
	}
	arch_irq_restore(f);
	return rc;
}

void hal_irq_enable(unsigned irq)
{
	/* The handler-table write must be visible before the source can fire. */
	__asm__ volatile("dsb sy" ::: "memory");

	(void) gic_enable_irq(irq); /* out-of-range numbers are ignored, per the contract */
}

void hal_irq_disable(unsigned irq)
{
	(void) gic_disable_irq(irq);
}

void hal_irq_dispatch(void)
{
	uint32_t iar = gic_acknowledge();
	uint32_t irq = GIC_IAR_INTID(iar);

	if (irq >= GIC_INTID_SPECIAL_MIN)
		return; /* spurious: must NOT be EOI'd */

	if (irq < gic_num_intids() && s_handlers[irq].fn != NULL)
	{
		s_handlers[irq].fn(s_handlers[irq].arg);
	}
	else
	{
		/* No handler: a level-triggered source would re-fire forever, so shut
		 * it off instead of letting it storm. */
		(void) gic_disable_irq(irq);
	}

	(void) gic_end_of_interrupt(iar); /* echo the raw IAR back (SGI source CPU) */
}