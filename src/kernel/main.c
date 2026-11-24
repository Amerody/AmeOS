// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#include "kernel/console.h"
#include "kernel/kprintf.h"
#include "kernel/shell.h"

#include <arch/arch.h>
#include <arch/irq.h>
#include <drivers/console/console_fb.h>
#include <drivers/console/console_uart.h>
#include <gfx/color.h>
#include <hal/hal.h>
#include <kernel/error.h>

#include <stddef.h>
#include <stdint.h>

/**
 * @file main.c
 * @brief Kernel entry point: the boot-time init sequence and the main input loop that
 *        feeds the shell.
 * @ingroup kernel
 */

/// @brief Rate of the periodic timer tick set up in init(), in Hz
#define TIMER_POLL_HZ 1000

/// @brief Ticks counted by on_tick() since the counter last wrapped to 0
static uint32_t s_tick = 0;

/**
 * @brief Timer tick callback, registered with hal_timer_set_tick() in init().
 *
 * Runs in interrupt context, #TIMER_POLL_HZ times per second. Counts ticks in #s_tick
 * and resets the counter every 1000 ticks; the periodic action there is commented out,
 * so it has no visible effect today.
 *
 * The wrap is a literal 1000, not #TIMER_POLL_HZ, so it is once per second only while
 * the two are equal.
 */
static void on_tick()
{
	if (++s_tick >= 1000)
	{
		// kprintf("[TICK]\n");
		s_tick = 0;
	};
}

/**
 * @brief Prints one boot status line: @c "[ OK ] " in green or @c "[FAIL] " in red, then
 *        the formatted message, then @c " (error <rc>)" on failure, and a newline.
 *
 * The colour only shows on the framebuffer sink; console_set_color() has no effect on
 * the UART.
 *
 * @param rc Result of the step: 0 for success, anything else is a failure code
 * @param fmt Format string, as for kprintf()
 * @param ... Arguments for the conversions in @p fmt
 */
static void step(int rc, const char* fmt, ...) __attribute__((format(printf, 2, 3)));

static void step(int rc, const char* fmt, ...)
{
	uint32_t color = (rc == 0) ? COLOR_GREEN : COLOR_RED;
	console_set_color(color);
	kprintf("%s", rc == 0 ? "[ OK ] " : "[FAIL] ");
	console_set_color(COLOR_WHITE);

	va_list ap;
	va_start(ap, fmt);
	kvprintf(fmt, ap);
	va_end(ap);

	if (rc != 0)
		kprintf(" (error %d)", rc);
	kprintf("\n");
}

/**
 * @brief Brings the system up, in order: architecture, platform, console (UART1 and
 *        framebuffer sinks), timer and tick, then unmasks CPU IRQs.
 *
 * The console exists only after platform_init(), so arch_init() and platform_init()
 * can't print. arch_init() returns nothing, and the result of platform_init() isn't
 * checked (it halts on fatal errors), so their status lines are always printed as OK.
 * The board description from platform_get_info() is printed after them.
 * console_uart_init(), console_fb_init() and hal_timer_init() report their real result;
 * the return value of hal_timer_set_tick() is not checked.
 * 
 * @param firmware_arg Argument left from the firmware
 */
static void init(uintptr_t firmware_arg)
{
	arch_init();                 // exception vectors installed, IRQs masked
	platform_init(firmware_arg); // interrupt controller up, memory map valid
	console_init();
	step(console_uart_init(1), "Console UART");
	step(console_fb_init(), "Console FB");
	console_set_scale(1);

	step(0, "Architecture %s initialized", arch_name());

	// Once we got a console we can write to, write the possible platform errors
	const char* platform_errors = platform_get_init_errors();
	if (platform_errors)
	{
		console_set_color(COLOR_RED);
		kprintf("Errors when initializing platform\n%s", platform_errors);
		console_set_color(COLOR_WHITE);
		step(1, "Platform %s failed to initialize", platform_name());
	}
	else
	{
		step(0, "Platform %s initialized", platform_name());
	}

	kprintf("Platform info: %s\n", platform_get_info());

	step(hal_timer_init(), "Timer initialized");
	hal_timer_set_tick(TIMER_POLL_HZ, on_tick);
	arch_irq_enable();
}

/**
 * @brief Kernel Main Entry Point
 *
 * Called from boot.S on CPU 0 at EL1. Runs init(), prints the welcome line, starts the
 * shell and then loops forever: with IRQs masked it asks console_getc() for a byte; if
 * there is one, it unmasks IRQs and passes it to shell_feed(), otherwise it waits in
 * arch_cpu_idle() before unmasking IRQs (the lost-wakeup-free sequence described
 * there). Never returns.
 *
 * @param firmware_arg Argument left from the firmware
 */
void kmain(uintptr_t firmware_arg)
{
	init(firmware_arg);
	kprintf("Welcome to AmeOS version %d.%d.%d!\n", KERNEL_VERSION_MAJOR, KERNEL_VERSION_MINOR, KERNEL_VERSION_PATCH);

	// initialize the shell after boot has completed successfully
	shell_init();

	while (1) // This is so kernel_main() doesn't return
	{
		arch_irq_disable();
		char c = 0;
		if (console_getc(&c))
		{
			arch_irq_enable();
			shell_feed(c);
		}
		else
		{
			// just wait
			arch_cpu_idle();
			arch_irq_enable();
		}
	}
}
