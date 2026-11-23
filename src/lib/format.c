// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#include "lib/format.h"

#include "lib/math.h"

/**
 * @file format.c
 * @brief The format-string parser and the buffer output behind snprintf() (see format.h)
 * @ingroup lib
 */

/**
 * @brief Sends a string to an output.
 *
 * @param out Output that receives the characters
 * @param s NUL-terminated string; NULL prints @c "(null)"
 */
static void print_str(struct fmt_out* out, const char* s)
{
	if (!s)
		s = "(null)";
	while (*s)
		out->putc(out, *s++);
}

/**
 * @brief Sends an unsigned number to an output in the given base, without prefix or
 *        padding.
 *
 * @param out Output that receives the digits
 * @param value Number to print
 * @param base Base, from 2 to 16
 * @param upper true to use @c A-F for digits above 9, false for @c a-f
 */
static void print_uint(struct fmt_out* out, uint64_t value, unsigned base, bool upper)
{
	const char* digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
	char        buf[64]; // enough for 64 bits in base 2
	size_t      i = 0;

	do
	{ // do-while so 0 prints "0"
		buf[i++] = digits[value % base];
		value /= base;
	}
	while (value);

	while (i) // digits came out reversed
		out->putc(out, buf[--i]);
}

/**
 * @brief Sends a signed number to an output in decimal, with a leading @c - if negative.
 *
 * @param out Output that receives the characters
 * @param value Number to print; every value works, including @c INT64_MIN
 */
static void print_int(struct fmt_out* out, int64_t value)
{
	if (value < 0)
	{
		out->putc(out, '-');
		print_uint(out, 0 - (uint64_t) value, 10, false); // also correct for INT64_MIN
	}
	else
		print_uint(out, (uint64_t) value, 10, false);
}

void fmt_vformat(struct fmt_out* out, const char* fmt, va_list ap)
{
	for (; *fmt; fmt++)
	{
		if (*fmt != '%')
		{
			out->putc(out, *fmt);
			continue;
		}

		fmt++;
		bool wide = false; // 'l' / 'z' -> 64-bit argument
		while (*fmt == 'l' || *fmt == 'z')
		{
			wide = true;
			fmt++;
		}

		switch (*fmt)
		{
		case 'c':
			out->putc(out, (char) va_arg(ap, int));
			break;
		case 's':
			print_str(out, va_arg(ap, const char*));
			break;
		case 'd':
			print_int(out, wide ? va_arg(ap, int64_t) : va_arg(ap, int));
			break;
		case 'u':
			print_uint(out, wide ? va_arg(ap, uint64_t) : va_arg(ap, unsigned), 10, false);
			break;
		case 'x':
			print_uint(out, wide ? va_arg(ap, uint64_t) : va_arg(ap, unsigned), 16, false);
			break;
		case 'p':
			print_str(out, "0x");
			print_uint(out, (uintptr_t) va_arg(ap, void*), 16, false);
			break;
		case '%':
			out->putc(out, '%');
			break;
		case '\0':
			return; // format string ended after a lone '%'
		default:
			out->putc(out, '%');
			out->putc(out, *fmt);
			break;
		}
	}
}

/**
 * @brief Buffer output used by vsnprintf(): stores @p c if it still fits, and counts it
 *        either way.
 *
 * A character is stored only while there is room for it and the terminating NUL
 * (@c len + 1 < @c size). @c len is always incremented, so after formatting it holds the
 * length of the full text, which is what vsnprintf() returns.
 *
 * @param out Buffer output (@c buf, @c size and @c len are used)
 * @param c Character to store
 */
static void buffer_out(struct fmt_out* out, char c)
{
	if (out->len + 1 < out->size)
		out->buf[out->len] = c;
	out->len++;
}

size_t vsnprintf(char* buf, size_t size, const char* fmt, va_list ap)
{
	struct fmt_out out;
	out.putc = buffer_out;
	out.buf  = buf;
	out.size = size;
	out.len  = 0;

	fmt_vformat(&out, fmt, ap);

	if (size > 0)
		buf[out.len < size ? out.len : size - 1] = '\0';

	return out.len;
}

size_t snprintf(char* buf, size_t size, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	size_t s = vsnprintf(buf, size, fmt, ap);
	va_end(ap);
	return s;
}
