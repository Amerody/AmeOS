// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#include "kernel/panic.h"

#include "kernel/kprintf.h"

#include <arch/arch.h>

#include <stdarg.h>

/**
 * @file panic.c
 * @brief panic() implementation (see panic.h)
 * @ingroup print
 */

void panic(const char* fmt, ...)
{
	va_list ap;
	kprintf("\n*** KERNEL PANIC: ");
	va_start(ap, fmt);
	kvprintf(fmt, ap);
	va_end(ap);
	kprintf("\n");
	arch_halt();
}
