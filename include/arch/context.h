// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file context.h
 * @brief Execution-context contract: kernel context switch and trap frames.
 * @ingroup arch
 *
 * Two different saved states exist, for two different situations:
 *
 *  - struct cpu_context: the minimal state saved when kernel code voluntarily
 *    switches from one thread to another (callee-saved registers and the stack
 *    pointer). Used by the scheduler.
 *  - struct trap_frame: the full register state saved by the exception vectors
 *    when the CPU takes an exception or interrupt. Used by fault handlers, the
 *    panic dump and, later, syscalls and user mode.
 *
 * Core never looks inside either struct. It only embeds them (for example one
 * cpu_context per thread) and passes them to the functions below.
 *
 * The struct layouts come from <arch/context_types.h>, which is provided by the
 * selected arch (src/arch/\<ARCH\>/ include/arch/context_types.h) and added to the
 * include path by CMake. It is the one place where an arch's data layout is
 * visible to the rest of the kernel; it contains layout only, no logic.
 *
 * Implemented by src/arch/\<ARCH\>/.
 */
#pragma once

#include <stdint.h>

#include "context_types.h" /* struct cpu_context, struct trap_frame */

#ifdef __cplusplus
extern "C"
{
#endif

	/* ------------------------------------------------------------------------ */
	/* Kernel-to-kernel context switch                                          */
	/* ------------------------------------------------------------------------ */

	/**
	 * @brief Prepare a context so that the first switch into it starts a thread.
	 *
	 * The first arch_context_switch() into @p ctx begins executing @p entry(@p arg)
	 * on the given stack, with IRQs masked.
	 *
	 * @p entry must not return. Core's thread wrapper is expected to end the thread
	 * (for example through a thread_exit()) rather than fall off the end.
	 *
	 * @param ctx        Context to initialise (memory owned by the caller).
	 * @param stack_top  One past the highest address of the thread's stack. Must be
	 *                   16-byte aligned (aarch64 requirement).
	 * @param entry      Function the thread starts in.
	 * @param arg        Argument passed to @p entry.
	 *
	 * @todo Not implemented yet: no arch provides arch_context_init().
	 */
	void arch_context_init(struct cpu_context* ctx, void* stack_top, void (*entry)(void* arg), void* arg);

	/**
	 * @brief Save the current context into @p from and resume @p to.
	 *
	 * To the caller this looks like an ordinary function call that returns only
	 * when some later switch targets @p from again. In between, other threads run.
	 *
	 * Requirements:
	 *  - Local IRQs must be masked by the caller. Switching with IRQs on would let
	 *    a timer tick re-enter the scheduler mid-switch.
	 *  - @p from and @p to must be different, valid contexts.
	 *  - A freshly initialised context (arch_context_init) starts with IRQs masked.
	 *    Its entry function, or core's wrapper around it, unmasks them when ready.
	 *
	 * Only callee-saved state is switched; the caller-saved registers are already
	 * dead across a normal function call.
	 *
	 * @param from Where to save the running context.
	 * @param to   Context to resume.
	 *
	 * @todo Not implemented yet: no arch provides arch_context_switch().
	 */
	void arch_context_switch(struct cpu_context* from, struct cpu_context* to);

	/* ------------------------------------------------------------------------ */
	/* Trap frames (exception entry state)                                      */
	/* ------------------------------------------------------------------------ */

	/**
	 * @brief Program counter at which the exception was taken.
	 *
	 * For a fault this is the faulting instruction; for an interrupt, the
	 * instruction that would have run next.
	 *
	 * @param tf Frame saved by the exception vectors.
	 * @return The saved program counter (ELR_EL1 on aarch64)
	 */
	uintptr_t arch_trap_frame_pc(const struct trap_frame* tf);

	/**
	 * @brief Print the register state held in a trap frame via kprintf().
	 *
	 * For fatal-exception and panic paths. Arch code may call kprintf()/panic()
	 * (leaf kernel facilities) but no other kernel subsystem.
	 *
	 * @param tf Frame saved by the exception vectors.
	 */
	void arch_trap_frame_dump(const struct trap_frame* tf);

#ifdef __cplusplus
}
#endif