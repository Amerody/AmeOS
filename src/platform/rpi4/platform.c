#include "hal/platform.h"

#include <arch/irq.h>
#include <hal/hal_irq.h>

#include <stddef.h>

/**
 * @file platform.c
 * @brief Raspberry Pi 4 implementation of the platform.h contract (see platform.h)
 * @ingroup drivers
 */

/// @brief Platform name returned by platform_name(); KERNEL_PLATFORM_NAME is set by CMake
static const char* s_platform_name = KERNEL_PLATFORM_NAME;

void platform_init()
{
	// Enable the GIC
	// gic_init();
	// gic_cpu_init();

	hal_irq_init();

	// Platform mem regions data not parsed yet
}

const char* platform_name()
{
	return s_platform_name;
}

size_t platform_mem_regions(struct mem_region* out, size_t max)
{
	return 0;
}