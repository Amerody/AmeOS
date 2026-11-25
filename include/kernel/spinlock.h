// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file spinlock.h
 * @brief Spinlock: a busy-waiting lock for mutual exclusion between CPUs, with
 *        variants that also mask IRQs on the calling CPU.
 * @ingroup kernel
 *
 * A CPU that finds the lock taken spins until it is free. Taking the lock is an
 * acquire and releasing it is a release (built on the compiler's @c __atomic
 * builtins), so everything written while holding the lock is visible to the next
 * CPU that takes it.
 *
 * A zero-initialised struct spinlock is unlocked, so a lock in static storage
 * needs no init call:
 *
 * @code
 * static struct spinlock s_lock; // unlocked
 *
 * arch_irq_flags_t f = spin_lock_irqsave(&s_lock);
 * // ... critical section ...
 * spin_unlock_irqrestore(&s_lock, f);
 * @endcode
 *
 * Rules for every lock:
 * - If an interrupt handler can take a lock, every place that takes it must use
 *   spin_lock_irqsave(). Otherwise the handler can interrupt the holder on the
 *   same CPU and spin forever, because the holder can't run until the handler
 *   returns.
 * - Locks aren't recursive: taking a lock the CPU already holds spins forever.
 * - Hold a lock briefly, and don't block or sleep while holding it.
 * - When taking several locks, always take them in the same order.
 *
 * Only CPU 0 runs kernel code for now, so a lock is never contended: on a single
 * core, the IRQ masking of the @c _irqsave variants is what protects the data.
 */

#pragma once

#include "arch/irq.h"

/**
 * @brief A spinlock. Zero-initialised means unlocked; use it only through the
 *        spin_*() functions.
 */
struct spinlock
{
	bool locked; ///< True while a CPU holds the lock. Accessed only with @c __atomic builtins
};

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	* @brief Take @p l, spinning until it is free.
	*
	* Acquire semantics: memory accesses after this call can't be moved before it,
	* by the compiler or the CPU. Spins with no timeout.
	*
	* Doesn't mask IRQs. Use it only for locks that no interrupt handler takes;
	* otherwise use spin_lock_irqsave().
	*
	* @param l  Lock to take; must not be NULL. Not recursive: the calling CPU must
	*           not already hold it.
	*/
	void spin_lock(struct spinlock* l);

	/**
	* @brief Release @p l.
	*
	* Release semantics: memory accesses before this call can't be moved after it,
	* by the compiler or the CPU. Doesn't change the IRQ mask.
	*
	* @param l  Lock to release; must be held by the calling CPU (not checked).
	*/
	void spin_unlock(struct spinlock* l);

	/**
	* @brief Mask IRQs on this CPU, then take @p l.
	*
	* Use this for every lock that an interrupt handler can also take (see the file
	* description). Nesting-safe: it may be called with IRQs already masked, because
	* the previous mask state is saved with arch_irq_save(). Always release with
	* spin_unlock_irqrestore() on the same CPU, passing the value returned here.
	*
	* @param l  Lock to take; must not be NULL. Not recursive, like spin_lock().
	*
	* @return IRQ mask state from before the call, to hand to spin_unlock_irqrestore().
	*/
	arch_irq_flags_t spin_lock_irqsave(struct spinlock* l);

	/**
	* @brief Release @p l, then restore the IRQ mask state saved by
	*        spin_lock_irqsave().
	*
	* IRQs are unmasked only if they were unmasked before the matching
	* spin_lock_irqsave() call.
	*
	* @param l      Lock to release; must be held by the calling CPU (not checked).
	* @param flags  Value returned by the matching spin_lock_irqsave().
	*/
	void spin_unlock_irqrestore(struct spinlock* l, arch_irq_flags_t flags);

#ifdef __cplusplus
}
#endif
