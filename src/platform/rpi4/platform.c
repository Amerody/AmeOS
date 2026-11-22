// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#include "hal/platform.h"
#include "arch/arch.h"
#include "soc.h"

#include <hal/hal_irq.h>

#include <common/mmio.h>
#include <stddef.h>

/**
 * @file platform.c
 * @brief Raspberry Pi 4 implementation of the platform.h contract (see platform.h)
 * @ingroup drivers
 */

/// @brief All the PM Registers
/// @warning @p RESERVED_* are not to be used
/// @note The rpi4 power management registers are not documented well. This is taken from
/// 	  the Linux driver (https://github.com/torvalds/linux/blob/master/drivers/watchdog/bcm2835_wdt.c)
/// @todo move this to hal_wdt or its appropiate file (pm.c for example) once it's needed
struct PM_Registers
{
	uint32_t RESERVED_00_18[CALC_OFFSET(0x00, 0x18)]; ///< 28 bytes gap until PM_RSTC

	volatile uint32_t RSTC; ///< Reset Control.
	volatile uint32_t RSTS; ///< Reset Status
	volatile uint32_t WDOG; ///< Watchdog countdown (20 bits) in ticks of about 16us
};

#define PM_BASE                  (PERIPHERAL_BASE + 0x100000)     ///< GICC base address
#define PM                       ((struct PM_Registers*) PM_BASE) ///< GICC access

#define PM_PASSWORD              0x5a000000

#define PM_WDOG_TIME_SET         0x000fffff
#define PM_RSTC_WRCFG_CLR        0xffffffcf
#define PM_RSTS_HADWRH_SET       0x00000040
#define PM_RSTC_WRCFG_SET        0x00000030
#define PM_RSTC_WRCFG_FULL_RESET 0x00000020
#define PM_RSTC_RESET            0x00000102

/*
 * The Raspberry Pi firmware uses the RSTS register to know which partition
 * to boot from. The partition value is spread into bits 0, 2, 4, 6, 8, 10.
 * Partition 63 is a special partition used by the firmware to indicate halt.
 */
#define PM_RSTS_RASPBERRYPI_HALT 0x555

/// @brief Platform name returned by platform_name(); KERNEL_PLATFORM_NAME is set by CMake
static const char* s_platform_name = KERNEL_PLATFORM_NAME;

void platform_init()
{
	hal_irq_init();

	// Platform mem regions data not parsed yet
}

void platform_reboot()
{
	// use a timeout of 10 ticks (~150us)
	mmio_write(&PM->WDOG, 10 | PM_PASSWORD);
	uint32_t val = mmio_read_field(&PM->RSTC, PM_RSTC_WRCFG_CLR, 0);
	val |= PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET;
	mmio_write(&PM->RSTC, val);

	// spin until the system resets
	arch_halt();
}

void platform_shutdown()
{
	// The rpi4 can't really power off but we can indicate the bootcode
	// to not reboot, so most of the chip is powered off.

	// Set the watchdog hard reset bit to distinguish the reset from
	// a normal (full) reset.
	uint32_t val = mmio_read(&PM->RSTS);
	val |= PM_PASSWORD | PM_RSTS_RASPBERRYPI_HALT;
	mmio_write(&PM->RSTS, val);

	// continue with normal reboot mechanism
	platform_reboot();
}

const char* platform_name()
{
	return s_platform_name;
}

size_t platform_mem_regions(struct mem_region* out, size_t max)
{
	(void) out;
	(void) max;
	return 0;
}