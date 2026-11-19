/**
 * @file context_types.h
 * @brief aarch64 layouts of the saved execution states. Layout only, no logic.
 * @ingroup arch
 *
 * The assembly in context.S and vectors.S hard-codes these offsets. If you
 * reorder or add a field, update the assembly (and the static asserts below)
 * in the same change.
 *
 * @todo context.S doesn't exist yet (struct cpu_context is not used by any code), and
 *       there are no static asserts on the layouts yet. Today only vectors.S depends
 *       on the struct trap_frame offsets.
 */
#pragma once

#include <stdint.h>

/**
 * @brief State saved by arch_context_switch(): the AAPCS64 callee-saved registers plus
 *        the stack pointer. 13 words.
 */
struct cpu_context
{
	/// @brief Callee-saved general-purpose registers x19 to x28
	uint64_t x19, x20, x21, x22, x23, x24, x25, x26, x27, x28;
	uint64_t fp; ///< x29 (frame pointer)
	uint64_t lr; ///< x30: where the switch resumes
	uint64_t sp; ///< Stack pointer
};

/**
 * @var cpu_context::x20
 * Callee-saved register x20
 * @var cpu_context::x21
 * Callee-saved register x21
 * @var cpu_context::x22
 * Callee-saved register x22
 * @var cpu_context::x23
 * Callee-saved register x23
 * @var cpu_context::x24
 * Callee-saved register x24
 * @var cpu_context::x25
 * Callee-saved register x25
 * @var cpu_context::x26
 * Callee-saved register x26
 * @var cpu_context::x27
 * Callee-saved register x27
 * @var cpu_context::x28
 * Callee-saved register x28
 */

/**
 * @brief State saved by the exception vectors on entry. 34 words (272 bytes), which
 *        keeps the kernel stack 16-byte aligned after the frame is pushed.
 *
 * vectors.S restores every field from the frame before @c eret, so a handler that
 * changes a field (for example @c elr or a register in @c x) changes the state it
 * returns to.
 */
struct trap_frame
{
	uint64_t x[31]; ///< x0..x30
	uint64_t sp;    ///< SP_EL0 (the user stack when coming from EL0)
	uint64_t elr;   ///< ELR_EL1: address to return to
	uint64_t spsr;  ///< SPSR_EL1: PSTATE to restore on return
};