#pragma once

#include <stdint.h>

#include "arch/aarch64/gic.h"

struct trap_frame
{
	uint64_t x[31];
	uint64_t elr;
	uint64_t spsr;
};

typedef void (*irq_fn_t)(uint32_t intid, struct trap_frame* tf);

/// Register the function called when interrupt @p intid fires. Pass NULL to unregister.
gic_status_t irq_register(uint32_t intid, irq_fn_t fn);

// ---------------------------------------------------------------------------
// The four handlers called from the vector table
// ---------------------------------------------------------------------------

void sync_handler(struct trap_frame* tf);

void irq_handler(struct trap_frame* tf);

void fiq_handler(struct trap_frame* tf);

void serror_handler(struct trap_frame* tf);

// ---------------------------------------------------------------------------
// Example user of the IRQ path: virtual timer tick (PPI 27)
// ---------------------------------------------------------------------------

void timer_init(uint32_t hz);
