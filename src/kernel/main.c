#include "kernel/console.h"
#include "kernel/kprintf.h"

#include "arch/arch.h"
#include "drivers/console/console_uart.h"
#include "hal/hal.h"

#include <stddef.h>
#include <stdint.h>

// uint64_t PERIPHERAL_BASE = 0xFE000000UL; // Fallback value

static void on_tick() { kprintf("[TICK]\n"); }

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

	arch_init();     // exception vectors installed, IRQs masked
	platform_init(); // interrupt controller up, memory map valid
	console_init();
	console_uart_init(1);
	kprintf("Architecture: %s\n", arch_name());
	kprintf("Platform: %s\n", platform_name());

	if (hal_timer_init() < 0)
		kprintf("Timer init failed\n");
	hal_timer_set_tick(1, on_tick);
	arch_irq_enable();

	kprintf("Welcome to AmeOS version %d.%d.%d!\n", 0, 1, 0);
	while (1)
	{
		; // This is so kernel_main() doesn't return
	}
}