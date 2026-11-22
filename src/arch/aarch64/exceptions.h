// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file exceptions.h
 * @brief Private to src/arch/aarch64/: the C entry points that vectors.S calls.
 * @ingroup arch
 *
 * Nothing outside the arch includes this. Each vector-table slot saves a
 * struct trap_frame on the kernel stack, passes its address in x0, and calls
 * one of these by name.
 *
 * Only the "current EL with SP_ELx" slots (exceptions taken from EL1 itself) call
 * these handlers. Every other slot branches to a silent @c wfe loop in vectors.S.
 *
 * Each handler runs at EL1 with IRQs masked, on the stack the exception was taken
 * on. When it returns, vectors.S restores the state from @p tf and returns with
 * @c eret.
 */
#pragma once

#include "arch/context.h" /* struct trap_frame */

/**
 * @brief Synchronous exceptions: SVC (syscalls), aborts, alignment faults, BRK, ...
 *
 * An SVC returns straight away without doing anything (syscalls are not implemented
 * yet). Every other exception class is fatal: it prints the exception details and the
 * registers, then halts the CPU.
 *
 * @param tf Frame saved by vectors.S
 */
void sync_handler(struct trap_frame* tf);

/**
 * @brief IRQ: hands over to the HAL, which does the controller-specific work.
 *
 * Calls hal_irq_dispatch().
 *
 * @param tf Frame saved by vectors.S; unused for now
 */
void irq_handler(struct trap_frame* tf);

/**
 * @brief FIQ: unused on this system, always fatal.
 *
 * Prints the exception details and the registers, then halts the CPU.
 *
 * @bug Never runs: boot.S enters EL1 with D, A, I and F masked (SPSR 0x3c5) and only
 *      the I bit is ever cleared (arch_irq_enable()), so FIQs stay masked and this
 *      diagnostic cannot fire.
 *
 * @param tf Frame saved by vectors.S
 */
void fiq_handler(struct trap_frame* tf);

/**
 * @brief SError (asynchronous abort): always fatal.
 *
 * Prints the exception details and the registers, then halts the CPU.
 *
 * @bug Never runs: PSTATE.A stays masked from boot.S on (only I is ever cleared), so an
 *      asynchronous abort (e.g. a bad MMIO access on real hardware) stays pending
 *      silently instead of panicking. Unmasking A after arch_init() would enable it.
 *
 * @param tf Frame saved by vectors.S
 */
void serror_handler(struct trap_frame* tf);
