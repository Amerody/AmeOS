#include "kernel/console.h"
#include "kernel/delay.h"
#include "kernel/kprintf.h"

#include <arch/arch.h>
#include <arch/irq.h>
#include <drivers/console/console_fb.h>
#include <drivers/console/console_uart.h>
#include <gfx/color.h>
#include <hal/hal.h>

#include <stddef.h>
#include <stdint.h>

#include <drivers/console/console_uart.h>
// uint64_t PERIPHERAL_BASE = 0xFE000000UL; // Fallback value

#define TIMER_POLL_HZ 1000

static uint32_t s_tick = 0;
static void on_tick()
{
	if (++s_tick >= 1000)
	{
		// kprintf("[TICK]\n");
		s_tick = 0;
	};
}

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

static void init()
{
	arch_init();     // exception vectors installed, IRQs masked
	platform_init(); // interrupt controller up, memory map valid
	console_init();
	console_uart_init(1);
	console_fb_init();
	console_set_scale(1);

	step(0, "Architecture %s initialized", arch_name());
	step(0, "Platform %s initialized", platform_name());

	step(hal_timer_init(), "Timer initialized");
	hal_timer_set_tick(TIMER_POLL_HZ, on_tick);
	arch_irq_enable();
	step(1, "Test error");
}

/**
 * @brief Kernel Main Entry Point
 *
 * @param dtb_ptr Physical memory address pointing to the Device Tree Blob (DTB).
 *                Passed automatically in X0 by QEMU or the physical RPi4 bootloader.
 *
 * Why we save this:
 * 1. Hardware Discovery: The DTB contains the blueprint of the SoC. It identifies
 *    the exact locations of memory maps, peripheral addresses, and CPU cores.
 * 2. Dynamic Memory: It explicitly details how much RAM is physically present
 *    (crucial for distinguishing between the 2GB, 4GB, or 8GB versions of the Pi 4).
 * 3. Future Proofing: If the peripheral base addresses change in future board revisions
 *    or QEMU targets, parsing this pointer prevents the kernel from instantly crashing.
 */
void kmain(uint64_t dtb_ptr)
{
	// 	  (Optional but recommended) Parse the DTB structure here
	//    Find the "soc" node or "ranges" property to extract the base.
	//
	//    if (parsed_soc_base != 0) {
	//        PERIPHERAL_BASE = parsed_soc_base;
	//    }

	init();

	step(1, "Test error");

	kprintf("Welcome to AmeOS version %d.%d.%d!\n", KERNEL_VERSION_MAJOR, KERNEL_VERSION_MINOR, KERNEL_VERSION_PATCH);

	while (1)
	{
		char c = 0;
		if (console_getc(&c))
			kprintf("%c\n", c);
		// This is so kernel_main() doesn't return
	}
}