#pragma once

static inline void enable_irq()
{
	__asm__ volatile("msr daifclr, #2"); // Clear IRQ mask
}

static inline void disable_irq()
{
	__asm__ volatile("msr daifset, #2"); // Set IRQ mask
}