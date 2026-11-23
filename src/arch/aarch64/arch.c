// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#include "arch/arch.h"

#include "hal/hal_timer.h"

#include <stddef.h>

/**
 * @file arch.c
 * @brief aarch64 implementation of the arch.h contract (see arch.h)
 * @ingroup arch
 */

/// @brief Architecture name returned by arch_name(); KERNEL_ARCH_NAME is set by CMake
static const char* s_arch_name = KERNEL_ARCH_NAME;

/**
 * @brief Installs the exception vector table: writes the address of @c vector_table
 *        to VBAR_EL1.
 *
 * Implemented in vectors.S. Must run at EL1. Clobbers x0 only.
 */
extern void install_vector_table();

void arch_init()
{
	install_vector_table();                           // sets VBAR_EL1, then isb
	__asm__ volatile("msr daifclr, #4" ::: "memory"); // unmask SError
}

const char* arch_name()
{
	return s_arch_name;
}

void arch_cpu_idle()
{
	__asm__ volatile("wfi");
}

void arch_halt()
{
	// stop the timer
	hal_timer_set_tick(0, NULL); // the callback is ignored

	while (1)
		arch_cpu_idle();
}
