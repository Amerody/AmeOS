// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#include "kernel/kprintf.h"

#include "kernel/console.h"

#include <arch/irq.h>
#include <lib/format.h>
#include <lib/math.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @file kprintf.c
 * @brief kprintf() / kvprintf() implementation: runs the lib formatter (fmt_vformat())
 *        with an output that writes each character to console_putc()
 * @ingroup print
 */

/**
 * @brief Console output for fmt_vformat(): sends @p c to console_putc().
 *
 * Uses none of the buffer members of @p o.
 *
 * @param o Output (unused)
 * @param c Character to print
 */
static void console_out(struct fmt_out* o, char c)
{
	(void) o;
	console_putc(c);
}

void kvprintf(const char* fmt, va_list ap)
{
	arch_irq_flags_t f = arch_irq_save();
	struct fmt_out   out;
	out.putc = console_out;
	fmt_vformat(&out, fmt, ap);
	arch_irq_restore(f);
}

__attribute__((format(printf, 1, 2))) void kprintf(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	kvprintf(fmt, ap);
	va_end(ap);
}
