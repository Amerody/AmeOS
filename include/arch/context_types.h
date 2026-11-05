/**
 * @file context_types.h
 * @brief aarch64 layouts of the saved execution states. Layout only, no logic.
 *
 * The assembly in context.S and vectors.S hard-codes these offsets. If you
 * reorder or add a field, update the assembly (and the static asserts below)
 * in the same change.
 */
#pragma once

#include <stdint.h>

/**
 * State saved by arch_context_switch(): the AAPCS64 callee-saved registers plus
 * the stack pointer. 13 words.
 */
struct cpu_context
{
	uint64_t x19, x20, x21, x22, x23, x24, x25, x26, x27, x28;
	uint64_t fp; /* x29 */
	uint64_t lr; /* x30: where the switch resumes */
	uint64_t sp;
};

/**
 * State saved by the exception vectors on entry. 34 words (272 bytes), which
 * keeps the kernel stack 16-byte aligned after the frame is pushed.
 */
struct trap_frame
{
	uint64_t x[31];  /* x0..x30 */
	uint64_t sp;     /* SP_EL0 (the user stack when coming from EL0) */
	uint64_t pc;     /* ELR_EL1: address to return to */
	uint64_t pstate; /* SPSR_EL1: PSTATE to restore on return */
};

_Static_assert(sizeof(struct trap_frame) % 16 == 0, "trap_frame must keep SP 16-byte aligned");