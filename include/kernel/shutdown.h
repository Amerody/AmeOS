// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file shutdown.h
 * @brief Kernel shutdown path: halts, powers off or reboots the system after the
 *        kernel's own clean-up.
 * @ingroup kernel
 *
 * The one place the kernel stops itself in an orderly way. The shell's @c shutdown
 * and @c reboot commands call kernel_shutdown(); the steps that must happen first
 * (masking interrupts, flushing the console, and later stopping other cores or
 * syncing filesystems) are added there, not in each caller. The final step is the
 * platform's (platform_reboot(), platform_shutdown()) or the CPU's (arch_halt()).
 */

#pragma once

/** @brief How kernel_shutdown() stops the system. */
enum shutdown_action
{
	SHUTDOWN_HALT,     ///< Stop the CPU with arch_halt(); the board stays powered
	SHUTDOWN_POWEROFF, ///< Power the board off with platform_shutdown()
	SHUTDOWN_REBOOT,   ///< Reset the board with platform_reboot()
};

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief Stop the system: clean up, then halt, power off or reboot.
	 *
	 * In order:
	 *  1. masks CPU IRQs with arch_irq_disable(), so no handler runs during the
	 *     sequence;
	 *  2. prints a message for @p action ("Halting system...", "Shutting down..."
	 *     or "Rebooting...") and calls console_flush(), so the message is on every
	 *     sink before the board stops;
	 *  3. for #SHUTDOWN_POWEROFF and #SHUTDOWN_REBOOT, waits 1 s with delay_ms();
	 *  4. calls arch_halt(), platform_shutdown() or platform_reboot().
	 *
	 * A value that isn't one of #shutdown_action calls arch_halt() without a
	 * message.
	 *
	 * Never returns. Meant for thread context (the shell): it prints with kprintf(),
	 * which isn't safe from interrupt handlers (see console_putc()). It stops only
	 * the calling core: the other cores are still parked by boot.S, so there is
	 * nothing else to stop yet.
	 *
	 * @pre The console is initialised (console_init()), so the message and the
	 *      flush have somewhere to go. Without sinks both do nothing.
	 *
	 * @param action What to do once the clean-up is done
	 */
	[[noreturn]] void kernel_shutdown(enum shutdown_action action);

#ifdef __cplusplus
}
#endif