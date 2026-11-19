#include "kernel/kprintf.h"
#include "kernel/console.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @file kprintf.c
 * @brief kprintf() / kvprintf() implementation: a format-string parser that writes each
 *        character to console_putc()
 * @ingroup print
 */

/**
 * @brief Writes a string to the console.
 *
 * @param s NUL-terminated string; NULL prints @c "(null)"
 */
static void print_str(const char* s)
{
	if (!s)
		s = "(null)";
	while (*s)
		console_putc(*s++);
}

/**
 * @brief Writes an unsigned number to the console in the given base, without prefix or
 *        padding.
 *
 * @param value Number to print
 * @param base Base, from 2 to 16
 * @param upper true to use @c A-F for digits above 9, false for @c a-f
 */
static void print_uint(uint64_t value, unsigned base, bool upper)
{
	const char* digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
	char buf[64]; // enough for 64 bits in base 2
	size_t i = 0;

	do
	{ // do-while so 0 prints "0"
		buf[i++] = digits[value % base];
		value /= base;
	}
	while (value);

	while (i) // digits came out reversed
		console_putc(buf[--i]);
}

/**
 * @brief Writes a signed number to the console in decimal, with a leading @c - if negative.
 *
 * @param value Number to print; every value works, including @c INT64_MIN
 */
static void print_int(int64_t value)
{
	if (value < 0)
	{
		console_putc('-');
		print_uint(0 - (uint64_t) value, 10, false); // also correct for INT64_MIN
	}
	else
		print_uint((uint64_t) value, 10, false);
}

void kvprintf(const char* fmt, va_list ap)
{
	for (; *fmt; fmt++)
	{
		if (*fmt != '%')
		{
			console_putc(*fmt);
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
			console_putc((char) va_arg(ap, int));
			break;
		case 's':
			print_str(va_arg(ap, const char*));
			break;
		case 'd':
			print_int(wide ? va_arg(ap, int64_t) : va_arg(ap, int));
			break;
		case 'u':
			print_uint(wide ? va_arg(ap, uint64_t) : va_arg(ap, unsigned), 10, false);
			break;
		case 'x':
			print_uint(wide ? va_arg(ap, uint64_t) : va_arg(ap, unsigned), 16, false);
			break;
		case 'p':
			print_str("0x");
			print_uint((uintptr_t) va_arg(ap, void*), 16, false);
			break;
		case '%':
			console_putc('%');
			break;
		case '\0':
			return; // format string ended after a lone '%'
		default:
			console_putc('%');
			console_putc(*fmt);
			break;
		}
	}
}

__attribute__((format(printf, 1, 2))) void kprintf(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	kvprintf(fmt, ap);
	va_end(ap);
}