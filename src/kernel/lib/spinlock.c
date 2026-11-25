// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#include "kernel/spinlock.h"

#include "arch/irq.h"

/**
 * @file spinlock.c
 * @brief Spinlock implementation (see spinlock.h)
 * @ingroup kernel
 *
 * @todo When multicore, stop waiting cores from pulling the lock's cache line
 *       exclusive on every iteration: the exchange in spin_lock() writes 1 even
 *       when the lock is already taken. Spin on a relaxed __atomic_load_n() until
 *       the lock reads 0 and only then retry the exchange (test-and-test-and-set).
 *       Then sleep in WFE between reads (SEVL first, via an arch_* function)
 *       instead of busy-reading; the STLR in spin_unlock() clears the exclusive
 *       monitor, which wakes the waiters. YIELD doesn't help here: it's only a hint
 *       for multithreaded cores.
 */
void spin_lock(struct spinlock* l)
{
	while (__atomic_exchange_n(&l->locked, 1, __ATOMIC_ACQUIRE))
	{
		// Wait until the previous value was 0 (unlocked)
	}
}

void spin_unlock(struct spinlock* l)
{
	__atomic_store_n(&l->locked, 0, __ATOMIC_RELEASE);
}

// Spinlock with irqs masked
arch_irq_flags_t spin_lock_irqsave(struct spinlock* l)
{
	// mask irqs first
	arch_irq_flags_t f = arch_irq_save();
	spin_lock(l); // take the lock
	return f;
}

void spin_unlock_irqrestore(struct spinlock* l, arch_irq_flags_t flags)
{
	spin_unlock(l);          // release the lock
	arch_irq_restore(flags); // unmask irqs
}
