#pragma once

#include "arch/irq.h"

struct spinlock
{
	bool locked;
};

void spin_lock(struct spinlock* l);

void spin_unlock(struct spinlock* l);

// Spinlock with irqs masked
arch_irq_flags_t spin_lock_irqsave(struct spinlock* l);
void             spin_unlock_irqrestore(struct spinlock* l, arch_irq_flags_t flags);
