// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#pragma once

/**
 * @file panic.h
 * @brief Kernel panic: report an unrecoverable error and stop.
 * @ingroup print
 */

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief Prints a panic message to the console and stops this CPU. Never returns.
	 *
	 * Prints @c "*** KERNEL PANIC: " on a new line, then the formatted message and a newline,
	 * then stops this CPU with arch_halt(), which masks interrupts first.
	 *
	 * Callable from any context, including interrupt and exception handlers: the output goes
	 * through kvprintf(), which takes no locks.
	 *
	 * @param fmt Format string; see kprintf.h for the supported conversions
	 * @param ... Arguments for the conversions in @p fmt
	 */
	[[noreturn]] void panic(const char* fmt, ...);

#ifdef __cplusplus
}
#endif