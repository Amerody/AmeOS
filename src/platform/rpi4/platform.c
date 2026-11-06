#include "hal/platform.h"

#include "arch/irq.h"
#include "hal/hal_irq.h"

#include <stddef.h>

static const char* s_platform_name = KERNEL_PLATFORM_NAME;

/**
 * @brief Board-level bring-up that must happen before anything else uses
 *        the HAL.
 *
 * Called once from kmain(), after arch_init(). On return:
 *  - the interrupt controller is initialised, so hal_irq_register() /
 *    hal_irq_enable() work (CPU IRQs are still masked);
 *  - platform_mem_regions() returns valid data (including any parsing of
 *    firmware-provided information such as a device tree).
 *
 * Does not touch the UART (see hal_uart_init()) or the timer (see
 * hal_timer_init()), and does not unmask CPU interrupts.
 *
 * Nothing can be printed yet while this runs. If bring-up fails in a way that
 * cannot be recovered, the implementation calls arch_halt().
 */
void platform_init()
{
	// Enable the GIC
	// gic_init();
	// gic_cpu_init();

	hal_irq_init();

	// Platform mem regions data not parsed yet
}

/**
 * @brief Short platform name for the boot banner, e.g. "rpi4".
 * @return A static string; never NULL.
 */
const char* platform_name()
{
	return s_platform_name;
}

/**
 * @brief Describe the RAM the kernel may use.
 *
 * Regions are sorted by ascending address and do not overlap. They exclude
 * memory owned by firmware or the GPU, but include the kernel image itself:
 * core subtracts the image (using linker symbols) before handing pages to the
 * physical allocator.
 *
 * Follows the snprintf convention: the return value is the total number of
 * regions, which may exceed @p max. At most @p max entries are written.
 *
 * Valid after platform_init().
 *
 * @param out  Array to fill; may be NULL when @p max is 0 (to just count).
 * @param max  Capacity of @p out in entries.
 * @return     Total number of usable regions
 *
 * @todo implement dtb parsing I guess
 */
size_t platform_mem_regions(struct mem_region* out, size_t max)
{
	return 0;
}