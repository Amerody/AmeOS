// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#include "kernel/delay.h"
#include <hal/hal_timer.h>

/**
 * @file delay.c
 * @brief delay_ns() implementation: spins until hal_timer_now_ns() passes the deadline
 *        (see delay.h)
 * @ingroup kernel
 */

void delay_ns(uint64_t ns)
{
	uint64_t end = hal_timer_now_ns() + ns;
	while (hal_timer_now_ns() < end)
		;
}