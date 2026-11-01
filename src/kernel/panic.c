#include "kernel/panic.h"

#include "kernel/kprintf.h"
#include <stdarg.h>

void panic(const char* fmt, ...)
{
	va_list ap;
	kprintf("\n*** KERNEL PANIC: ");
	va_start(ap, fmt);
	kvprintf(fmt, ap);
	va_end(ap);
	kprintf("\n");
	for(;;)
		__asm__ volatile("wfe");
}