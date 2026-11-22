// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#include "kernel/shutdown.h"

#include "arch/irq.h"
#include "kernel/console.h"
#include "kernel/delay.h"
#include "kernel/kprintf.h"

#include <arch/arch.h>
#include <hal/platform.h>

/**
 * @file shutdown.c
 * @brief Kernel shutdown path: clean-up before halting, powering off or rebooting
 *        (see shutdown.h)
 * @ingroup kernel
 */

void kernel_shutdown(enum shutdown_action action)
{
	// do stuff...
	arch_irq_disable();

	switch (action)
	{
	case SHUTDOWN_HALT:
		kprintf("Halting system...\n");
		console_flush();
		arch_halt();
		// doesn't return from arch_halt()

	case SHUTDOWN_POWEROFF:
		kprintf("Shutting down...\n");
		console_flush();
		delay_ms(1000); // manual delay to check
		platform_shutdown();
		// doesn't return from shutdown

	case SHUTDOWN_REBOOT:
		kprintf("Rebooting...\n");
		console_flush();
		delay_ms(1000); // manual delay to check
		platform_reboot();
		// doesn't return from reboot
	default:
		arch_halt();
	}
}
