// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file format.h
 * @brief printf-style formatting into any output: the format-string parser shared by
 *        kprintf() and snprintf().
 * @ingroup lib
 *
 * fmt_vformat() parses a format string and hands every output character to a
 * caller-provided output (struct fmt_out). The output decides where characters go:
 * snprintf() stores them in a buffer, kprintf() sends them to the console. This file
 * depends on nothing outside lib, so platform and driver code can format text
 * without depending on the kernel console.
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
 * Context: the parser touches no global state and never blocks, so it can be used from
 * any context, including interrupt handlers, as long as the output's @c putc can.
 */
#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Where fmt_vformat() sends its output, plus the state a buffer output needs.
 *
 * fmt_vformat() only calls @c putc; the other members belong to the output. An output
 * that doesn't write to a buffer (e.g. the console in kprintf.c) can leave them unset.
 */
struct fmt_out
{
	void (*putc)(struct fmt_out* o, char c); ///< Called once per output character, in order
	char*  buf;                              ///< Buffer only: where characters are stored
	size_t size;                             ///< Buffer only: capacity of @c buf in bytes, NUL included
	size_t len;                              ///< Buffer only: characters produced so far, including
	                                         ///< the ones that didn't fit in @c buf
};

/**
 * @brief Formats @p fmt with the arguments in @p ap and sends each character to @p out.
 *
 * See the file description for the supported conversions. No NUL is sent at the end:
 * ending the text (if needed) is the output's job.
 *
 * @param out Output that receives the characters; @c out->putc must be set
 * @param fmt Format string
 * @param ap  Arguments for the conversions in @p fmt. Started with @c va_start() by the
 *            caller, which also calls @c va_end() afterwards; it can't be used again
 *            after this returns
 */
void fmt_vformat(struct fmt_out* out, const char* fmt, va_list ap);

/**
 * @brief Formats into a buffer, like the C library's snprintf().
 *
 * Writes at most @p size - 1 characters followed by a NUL, so @p buf is always
 * NUL-terminated. Output that doesn't fit is dropped but still counted in the return
 * value, so a return value of @p size or more means the text was truncated.
 *
 * @param buf  Buffer to write to
 * @param size Capacity of @p buf in bytes, NUL included
 * @param fmt  Format string (see the file description)
 * @param ...  Arguments for the conversions in @p fmt
 *
 * @return Length of the full formatted text, without the NUL, even if it didn't fit
 *         or @p size is 0 (nothing is written then).
 */
__attribute__((format(printf, 3, 4))) size_t snprintf(char* buf, size_t size, const char* fmt, ...);

/**
 * @brief snprintf() with the arguments given as a @c va_list.
 *
 * For functions that take their own variadic arguments and forward them.
 *
 * @param buf  Buffer to write to
 * @param size Capacity of @p buf in bytes, NUL included; must not be 0 (see the bug note)
 * @param fmt  Format string (see the file description)
 * @param ap   Arguments for the conversions in @p fmt. Started with @c va_start() by the
 *             caller, which also calls @c va_end() afterwards
 *
 * @return Length of the full formatted text, without the NUL, even if it didn't fit or
 * @p size is 0 (nothing is written then).
 */
size_t vsnprintf(char* buf, size_t size, const char* fmt, va_list ap);
