// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#pragma once

/**
 * @file kprintf.h
 * @brief Kernel printf: formatted output to the console.
 * @ingroup print
 *
 * A small printf() subset with no buffering: every character goes straight to
 * console_putc(), so output appears on every registered sink as it is formatted.
 *
 * The format string is parsed by fmt_vformat() from the kernel library, so the
 * supported conversions are the ones listed in format.h. To format into a buffer
 * instead of the console, use snprintf() from format.h.
 *
 * Context: same as console_putc(), so safe from interrupt and exception handlers
 * and the panic path. There is no locking, so output from different contexts can
 * interleave.
 */

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief Prints a formatted string to the console.
	 *
	 * See format.h for the supported conversions.
	 *
	 * @param fmt Format string
	 * @param ... Arguments for the conversions in @p fmt
	 */
	__attribute__((format(printf, 1, 2))) void kprintf(const char* fmt, ...);

	/**
	 * @brief kprintf() with the arguments given as a @c va_list.
	 *
	 * For functions that take their own variadic arguments and forward them, such as
	 * panic().
	 *
	 * @param fmt Format string, as for kprintf()
	 * @param ap Arguments for the conversions in @p fmt. Started with @c va_start() by the
	 *           caller, which also calls @c va_end() afterwards
	 */
	void kvprintf(const char* fmt, va_list ap);

#ifdef __cplusplus
}
#endif
