#include "kernel/console.h"
#include "kernel/kprintf.h"
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

void kernel_main()
{
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
	}
}