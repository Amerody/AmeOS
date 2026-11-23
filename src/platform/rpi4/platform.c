// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#include "hal/platform.h"

#include "arch/arch.h"
#include "hal/hal_fb.h"
#include "mailbox/mailbox_tags.h"
#include "soc.h"

#include <common/mmio.h>
#include <hal/hal_irq.h>
#include <lib/format.h>
#include <lib/string.h>

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

#define PM_BASE                  (PERIPHERAL_BASE + 0x100000)     ///< PM base address
#define PM                       ((struct PM_Registers*) PM_BASE) ///< PM access

/// @brief Password in bits 31:24 that every write to a PM register carries (the Linux driver
///        ORs it into each write)
#define PM_PASSWORD              0x5a000000

#define PM_WDOG_TIME_SET         0x000fffff ///< WDOG: mask of the 20-bit countdown. Unused
#define PM_RSTC_WRCFG_CLR        0xffffffcf ///< RSTC: clears the WRCFG field (bits 5:4)
#define PM_RSTS_HADWRH_SET       0x00000040 ///< RSTS: bit 6, named after Linux. Unused
#define PM_RSTC_WRCFG_SET        0x00000030 ///< RSTC: the WRCFG field (bits 5:4). Unused
#define PM_RSTC_WRCFG_FULL_RESET 0x00000020 ///< RSTC: WRCFG value for a full reset
#define PM_RSTC_RESET            0x00000102 ///< RSTC: value named after Linux. Unused

/**
 * @brief RSTS value that selects boot partition 63, which the firmware treats as "halt".
 *
 * The Raspberry Pi firmware uses the RSTS register to know which partition
 * to boot from. The partition value is spread into bits 0, 2, 4, 6, 8, 10.
 * Partition 63 is a special partition used by the firmware to indicate halt.
 */
#define PM_RSTS_RASPBERRYPI_HALT 0x555

/// @brief Platform name returned by platform_name(); KERNEL_PLATFORM_NAME is set by CMake
static const char* s_platform_name = KERNEL_PLATFORM_NAME;

/// @brief Board description returned by platform_get_info(); filled once by platform_init()
static char s_info[512];

// Brings up the GIC, then asks the firmware for the board information in one property
// request and formats it into s_info. A failed query only replaces the text with a note:
// the board works without it, so platform_init() still succeeds.
int platform_init()
{
	hal_irq_init();

	// request system data
	alignas(16) uint32_t buf[64];
	struct mailbox_prop  p;

	mailbox_prop_begin(&p, buf, 64);

	// firmware version
	struct mbox_firmware_ver* firmware_version = mailbox_request_firmware_version(&p);

	// board model
	struct mbox_board_model* board_model = mailbox_request_board_model(&p);

	// board_revision
	struct mbox_board_revision* board_revision = mailbox_request_board_revision(&p);

	// board serial
	struct mbox_board_serial* board_serial = mailbox_request_board_serial(&p);

	// arm memory
	struct mbox_arm_memory* arm_memory = mailbox_request_arm_memory(&p);

	// vc memory
	struct mbox_vc_memory* vc_memory = mailbox_request_vc_memory(&p);

	if (mailbox_prop_send(&p) != 0 || !mailbox_prop_tag_ok(firmware_version) || !mailbox_prop_tag_ok(board_model) || !mailbox_prop_tag_ok(board_revision) || !mailbox_prop_tag_ok(board_serial) || !mailbox_prop_tag_ok(arm_memory) || !mailbox_prop_tag_ok(vc_memory))
	{
		snprintf(s_info, sizeof s_info, "Board info unavailable\n");
		return 0; // not fatal: the board works without the information
	}

	uint64_t serial = ((uint64_t) board_serial->high << 32) | board_serial->low;

	(void) snprintf(s_info, sizeof s_info, "Firmware revision: %x\n"
	                                       "Board model:       %x\n"
	                                       "Board revision:    %x\n"
	                                       "Serial:            %lx\n"
	                                       "ARM memory:        %u MiB at %x\n"
	                                       "VC memory:         %u MiB at %x",
	                firmware_version->version, board_model->model, board_revision->revision, serial, arm_memory->size / (1024 * 1024), arm_memory->base, vc_memory->size / (1024 * 1024), vc_memory->base);

	// Platform mem regions data not parsed yet

	return 0;
}

// Arms the PM watchdog with a very short timeout and a full-reset configuration, the same
// sequence as __bcm2835_restart() in the Linux driver. QEMU raspi4b ignores the WDOG
// countdown and resets the machine as soon as RSTC is written.
void platform_reboot()
{
	struct framebuffer* fb = hal_fb_get_info();
	hal_fb_fill_rect(0, 0, fb->width, fb->height, 0x00000000);

	// use a timeout of 10 ticks (~150us)
	mmio_write(&PM->WDOG, 10 | PM_PASSWORD);
	uint32_t val = mmio_read_field(&PM->RSTC, PM_RSTC_WRCFG_CLR, 0);
	val |= PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET;
	mmio_write(&PM->RSTC, val);

	// spin until the system resets
	arch_halt();
}

// QEMU raspi4b exits instead of resetting when RSTS holds the halt partition.
void platform_shutdown()
{
	// The rpi4 can't really power off but we can indicate the bootcode
	// to not reboot, so most of the chip is powered off.

	// Select boot partition 63 (PM_RSTS_RASPBERRYPI_HALT): after the reset the
	// firmware sees the halt partition and doesn't boot the kernel again.
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

const char* platform_get_info()
{
	return s_info;
}

size_t platform_mem_regions(struct mem_region* out, size_t max)
{
	(void) out;
	(void) max;
	return 0;
}
