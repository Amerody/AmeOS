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
 * Supported conversions:
 * | Conversion | Argument             | Output                                      |
 * |------------|----------------------|---------------------------------------------|
 * | @c %%c     | @c int               | the character                               |
 * | @c %%s     | @c const @c char*    | the string; @c "(null)" for NULL            |
 * | @c %%d     | @c int               | signed decimal                              |
 * | @c %%u     | @c unsigned          | unsigned decimal                            |
 * | @c %%x     | @c unsigned          | lowercase hexadecimal, no prefix            |
 * | @c %%p     | @c void*             | @c 0x followed by lowercase hexadecimal     |
 * | @c \%\%     | none                 | a single @c %                               |
 *
 * Length modifiers: any number of @c l or @c z before @c d, @c u or @c x makes the
 * argument 64-bit (@c int64_t / @c uint64_t), so @c %%ld, @c %%lld and @c %%zu all work.
 * Other modifiers (@c h, @c hh, @c j, @c t) are not supported.
 *
 * Not supported: flags, field width and precision (e.g. @c %08x), @c %%X, @c %%o
 * and floating point. An unknown conversion is printed as is, @c % included, but
 * without its length modifiers (@c %%lq prints @c %%q); a lone @c % at the end of
 * the format string is dropped, also when length modifiers follow it.
 *
 * Context: same as console_putc(), so safe from interrupt and exception handlers
 * and the panic path. There is no locking, so output from different contexts can
 * interleave.
 */

#include <stdarg.h>

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief Prints a formatted string to the console.
	 *
	 * See the file description for the supported conversions.
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
