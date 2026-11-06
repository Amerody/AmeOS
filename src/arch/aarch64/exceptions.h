/**
 * @file exceptions.h
 * @brief Private to src/arch/aarch64/: the C entry points that vectors.S calls.
 *
 * Nothing outside the arch includes this. Each vector-table slot saves a
 * struct trap_frame on the kernel stack, passes its address in x0, and calls
 * one of these by name.
 */
#pragma once

#include "arch/context.h" /* struct trap_frame */

/** Synchronous exceptions: SVC (syscalls), aborts, alignment faults, BRK, ... */
void sync_handler(struct trap_frame* tf);

/** IRQ: hands over to the HAL, which does the controller-specific work. */
void irq_handler(struct trap_frame* tf);

/** FIQ: unused on this system, always fatal. */
void fiq_handler(struct trap_frame* tf);

/** SError (asynchronous abort): always fatal. */
void serror_handler(struct trap_frame* tf);
