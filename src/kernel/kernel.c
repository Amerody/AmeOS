#include "kernel/console.h"
#include "kernel/kprintf.h"
<<<<<<< HEAD
#include "kernel/panic.h"

#include "arch/aarch64/timer.h"

#include <stddef.h>
#include <stdint.h>

static inline unsigned int get_current_el(void)
{
	unsigned long el;
	__asm__ volatile("mrs %0, CurrentEL" : "=r"(el));
	return (el >> 2) & 3;
}
=======

#include "arch/aarch64/gic.h"
#include "kernel/exceptions.h"
>>>>>>> homeserver/Dev

#include <stddef.h>
#include <stdint.h>

// uint64_t PERIPHERAL_BASE = 0xFE000000UL; // Fallback value

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
void kernel_main(uint64_t dtb_ptr)
{
<<<<<<< HEAD
	console_init();
	timer_init();

	kprintf("Hello World!\n");
	kprintf("Welcome to AmeOS version %d.%d.%d!\n", 0, 1, 0);
	kprintf("Current EL: %d\n", get_current_el());
	// while(1)
	// 	; // This is so kernel_main() doesn't return

	uint64_t count = 0;
	while(1)
	{
		kprintf("Tick %ld\n", count++);

		timer_sleep(1000); // 1 second delay
=======
	// 	  (Optional but recommended) Parse the DTB structure here
	//    Find the "soc" node or "ranges" property to extract the base.
	//
	//    if (parsed_soc_base != 0) {
	//        PERIPHERAL_BASE = parsed_soc_base;
	//    }
	console_init();

	gic_init();                                       // boot core only
	gic_cpu_init();                                   // every core
	timer_init(1);                                    // 1 tick per second
	__asm__ volatile("msr daifclr, #2" ::: "memory"); // finally unmask IRQs

	// timer_init();
	// timer_interrupt_init(1);
	// timer_interrupt_enable();

	kprintf("Hello World!\n");
	kprintf("Welcome to AmeOS version %d.%d.%d!\n", 0, 1, 0);
	while(1)
	{
		; // This is so kernel_main() doesn't return
>>>>>>> homeserver/Dev
	}
}