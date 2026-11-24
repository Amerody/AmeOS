// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#include "hal/platform.h"

#include "arch/arch.h"
#include "hal/hal_fb.h"
#include "mailbox/mailbox_tags.h"
#include "soc.h"

#include <common/mmio.h>
#include <hal/hal_irq.h>
#include <kernel/error.h>
#include <lib/fdt.h>
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

/// @brief Buffer to store possible errors during initialization
static char s_errors[512];
/// @brief Index of the next character to write in @p s_errors
static size_t s_idx_errors;

/**
 * @brief Helper to add an error message to the buffer
 * 
 * Only plain text errors, no formatting. NULL terminated strings,
 * the NULL character will be removed upon insertion.
 * 
 * If there are at least sizeof(s_errors) - 1 characters, the rest 
 * of the input is ignored and a NULL character is written at the 
 * end of the buffer
 *
 * @bug The size check only runs once, before copying: a message longer than the space
 *      left is copied whole and writes past the end of @p s_errors. The check needs to be
 *      inside the copy loop to match the description above.
 *
 * @param err Message to append, usually ending with @c '\\n'
 */
static void add_error(const char* err)
{
	char c;
	while ((c = *err++) != '\0')
	{
		if (s_idx_errors >= sizeof(s_errors))
		{
			s_errors[sizeof(s_errors) - 1] = '\0';
			return;
		}

		s_errors[s_idx_errors++] = c;
	}
	s_errors[s_idx_errors] = '\0';
}

// ---- RAM map from the device tree -------------------------------------------------------

/// @brief Page size the regions are aligned to
/// @todo Take it from the arch/MMU code once the translation granule is chosen
#define MEM_PAGE_SIZE           0x1000u

#define MEM_MAX_RAM_RANGES      16u ///< Capacity of s_ram (RAM regions after splitting)
#define MEM_MAX_RESERVED_RANGES 16u ///< Capacity of the list of ranges to remove

/**
 * @brief A physical range [base, end) during the RAM map computation.
 *
 * Uses an end address instead of a size, so every overlap test is a comparison
 * and no subtraction can go negative. Converted to struct mem_region at the end.
 */
struct mem_range
{
	uint64_t base; ///< First byte
	uint64_t end;  ///< One past the last byte
};

/// @brief Usable RAM, sorted, non-overlapping and page-aligned once platform_init() returns
static struct mem_range s_ram[MEM_MAX_RAM_RANGES];
/// @brief Number of valid entries in s_ram
static size_t s_ram_count;

/**
 * @brief Appends [base, base + size) to @p ranges.
 * @param ranges List to append to
 * @param count  In/out: number of valid entries in @p ranges
 * @param max    Capacity of @p ranges in entries
 * @param base   First byte of the range
 * @param size   Size of the range in bytes
 * @return false if the list is full or base + size overflows (nothing is added);
 *         true otherwise, including for an empty range, which is skipped.
 */
static bool range_add(struct mem_range* ranges, size_t* count, size_t max, uint64_t base, uint64_t size)
{
	if (size == 0)
		return true;
	if (base + size < base || *count >= max)
		return false;

	ranges[*count].base = base;
	ranges[*count].end  = base + size;
	(*count)++;
	return true;
}

/**
 * @brief Removes [rb, re) from every range in @p ranges, splitting ranges where needed.
 *
 * Doesn't keep the order (range_sort_and_merge() restores it).
 *
 * @param ranges List to remove from
 * @param count  In/out: number of valid entries in @p ranges
 * @param max    Capacity of @p ranges in entries (a split adds one)
 * @param rb     First byte to remove
 * @param re     One past the last byte to remove
 * @return false if a split needed a free slot and there was none. The range is then
 *         cut short at @p rb, so the result loses some RAM but never contains reserved memory.
 */
static bool range_subtract(struct mem_range* ranges, size_t* count, size_t max, uint64_t rb, uint64_t re)
{
	bool   ok = true;
	size_t i  = 0;
	while (i < *count)
	{
		struct mem_range* r = &ranges[i];

		if (re <= r->base || rb >= r->end)
		{
			i++; // no overlap
		}
		else if (rb <= r->base && re >= r->end)
		{
			// Covered entirely: replace with the last range and look at slot i again
			*r = ranges[--(*count)];
		}
		else if (rb <= r->base)
		{
			r->base = re; // cuts off the start
			i++;
		}
		else if (re >= r->end)
		{
			r->end = rb; // cuts off the end
			i++;
		}
		else
		{
			// Strictly inside: keep [base, rb) here and add [re, end) at the end. The new
			// range lies after re, so this removal never needs to look at it again
			uint64_t old_end = r->end;
			r->end           = rb;
			if (*count < max)
			{
				ranges[*count].base = re;
				ranges[*count].end  = old_end;
				(*count)++;
			}
			else
			{
				ok = false;
			}
			i++;
		}
	}
	return ok;
}

/**
 * @brief Rounds every range inward to page boundaries and drops the ones that become empty
 * @param ranges List to align
 * @param count  In/out: number of valid entries in @p ranges
 */
static void range_align_inward(struct mem_range* ranges, size_t* count)
{
	size_t kept = 0;
	for (size_t i = 0; i < *count; i++)
	{
		uint64_t base = (ranges[i].base + MEM_PAGE_SIZE - 1) & ~(uint64_t) (MEM_PAGE_SIZE - 1);
		uint64_t end  = ranges[i].end & ~(uint64_t) (MEM_PAGE_SIZE - 1);
		if (base < ranges[i].base) // rounding up wrapped past the top of the address space
			continue;
		if (base < end)
			ranges[kept++] = (struct mem_range){ base, end };
	}
	*count = kept;
}

/**
 * @brief Sorts @p ranges by base (insertion sort: the list is tiny) and merges touching ones
 * @param ranges List to sort
 * @param count  In/out: number of valid entries in @p ranges (smaller after merging)
 */
static void range_sort_and_merge(struct mem_range* ranges, size_t* count)
{
	for (size_t i = 1; i < *count; i++)
	{
		struct mem_range key = ranges[i];
		size_t           j   = i;
		while (j > 0 && ranges[j - 1].base > key.base)
		{
			ranges[j] = ranges[j - 1];
			j--;
		}
		ranges[j] = key;
	}

	size_t kept = 0;
	for (size_t i = 0; i < *count; i++)
	{
		if (kept > 0 && ranges[i].base <= ranges[kept - 1].end)
		{
			if (ranges[i].end > ranges[kept - 1].end)
				ranges[kept - 1].end = ranges[i].end;
		}
		else
		{
			ranges[kept++] = ranges[i];
		}
	}
	*count = kept;
}

/**
 * @brief Adds every @c reg tuple of @p node (cells taken from @p parent) to @p ranges.
 * @param node   Node whose @c reg to read
 * @param parent @p node's parent (holds the cell counts)
 * @param ranges List to append to
 * @param count  In/out: number of valid entries in @p ranges
 * @param max    Capacity of @p ranges in entries
 * @return false if a tuple didn't fit or @c reg is malformed.
 */
static bool add_reg_ranges(struct fdt_node node, struct fdt_node parent, struct mem_range* ranges, size_t* count, size_t max)
{
	uint64_t addr, size;
	int      ret;
	uint32_t i = 0;
	for (; (ret = fdt_get_reg(node, parent, i, &addr, &size)) == SUCCESS; i++)
	{
		if (!range_add(ranges, count, max, addr, size))
			return false;
	}
	return ret == -ERROR_NOT_FOUND; // the normal end of the tuples
}

/**
 * @brief Fills s_ram from the device tree: the /memory nodes minus everything reserved.
 *
 * Reserved: the memory reservation block, the static (@c reg) children of
 * /reserved-memory, and the DTB itself. The kernel image is not removed here: core
 * does that (see platform_mem_regions()). Problems are reported with add_error(); on a
 * problem that could leave reserved memory in the list, s_ram is emptied.
 *
 * Needs fdt_init() to have succeeded; otherwise s_ram stays empty.
 */
static void mem_init_from_dtb()
{
	s_ram_count = 0;
	if (fdt_get_blob_address() == 0)
		return; // platform_init() already reported the invalid DTB

	struct fdt_node root, node;
	if (fdt_get_root(&root) != SUCCESS)
	{
		add_error("DTB: no root node, no RAM map\n");
		return;
	}

	// 1. RAM: every enabled /memory or /memory@... node
	bool dtb_error = false;
	int  ret;
	for (ret = fdt_first_subnode(root, &node); ret == SUCCESS; ret = fdt_next_subnode(node, &node))
	{
		if (!fdt_is_node_named(node, "memory") || !fdt_is_node_enabled(node))
			continue;
		if (!add_reg_ranges(node, root, s_ram, &s_ram_count, MEM_MAX_RAM_RANGES))
			dtb_error = true;
	}
	if (dtb_error)
		add_error("DTB: a memory node didn't fit or is malformed, some RAM ignored\n");
	if (ret != -ERROR_NOT_FOUND)
		add_error("DTB: malformed tree while looking for memory nodes\n");

	// 2. What must not be used
	struct mem_range reserved[MEM_MAX_RESERVED_RANGES];
	size_t           reserved_count = 0;
	bool             reserved_ok    = true;

	struct fdt_reserve_entry rsv;
	for (uint32_t i = 0; (ret = fdt_get_reserve(i, &rsv)) == SUCCESS; i++)
		reserved_ok &= range_add(reserved, &reserved_count, MEM_MAX_RESERVED_RANGES, rsv.address, rsv.size);
	reserved_ok &= ret == -ERROR_NOT_FOUND;

	struct fdt_node rmem;
	ret = fdt_find_subnode(root, "reserved-memory", &rmem);
	if (ret == SUCCESS)
	{
		// Only identity mapping is supported: an empty "ranges" (spec 3.5)
		struct fdt_prop ranges;
		if (fdt_get_prop(rmem, "ranges", &ranges) != SUCCESS || ranges.len != 0)
			add_error("DTB: /reserved-memory ranges isn't empty, addresses used untranslated\n");

		for (ret = fdt_first_subnode(rmem, &node); ret == SUCCESS; ret = fdt_next_subnode(node, &node))
		{
			// Children with only "size" (e.g. linux,cma) are dynamic pools for Linux: skip them
			struct fdt_prop reg;
			if (!fdt_is_node_enabled(node) || fdt_get_prop(node, "reg", &reg) != SUCCESS)
				continue;
			reserved_ok &= add_reg_ranges(node, rmem, reserved, &reserved_count, MEM_MAX_RESERVED_RANGES);
		}
		reserved_ok &= ret == -ERROR_NOT_FOUND;
	}
	else if (ret != -ERROR_NOT_FOUND)
	{
		reserved_ok = false;
	}

	reserved_ok &= range_add(reserved, &reserved_count, MEM_MAX_RESERVED_RANGES, fdt_get_blob_address(), fdt_get_blob_size());

	if (!reserved_ok)
	{
		// Some reserved range may be missing from the list: handing out that RAM could
		// overwrite firmware data, so hand out none
		add_error("DTB: couldn't read every reserved range, no RAM map\n");
		s_ram_count = 0;
		return;
	}

	// 3. Remove the reserved ranges, then round inward so partly reserved pages go too
	for (size_t i = 0; i < reserved_count; i++)
	{
		if (!range_subtract(s_ram, &s_ram_count, MEM_MAX_RAM_RANGES, reserved[i].base, reserved[i].end))
			add_error("DTB: too many RAM regions after removing reservations, some RAM ignored\n");
	}
	range_align_inward(s_ram, &s_ram_count);
	range_sort_and_merge(s_ram, &s_ram_count);
}

uintptr_t g_peripheral_base = SOC_PERIPHERAL_BASE_DEFAULT;
uintptr_t g_arm_local_base  = SOC_ARM_LOCAL_BASE_DEFAULT;

/**
 * @brief Sets g_peripheral_base and g_arm_local_base from the device tree: /soc's
 *        translations of #SOC_PERIPHERAL_BUS_BASE and #SOC_ARM_LOCAL_BUS_BASE.
 *
 * Each one keeps its default (and the problem is reported with add_error()) if the DTB
 * has no /soc or its @c ranges doesn't cover that window. Must run before any driver
 * touches a register. Needs fdt_init() to have succeeded.
 */
static void peripheral_base_init_from_dtb()
{
	struct fdt_node root, soc;
	if (fdt_get_root(&root) != SUCCESS || fdt_find_subnode(root, "soc", &soc) != SUCCESS)
	{
		add_error("DTB: no /soc node, using the default peripheral bases\n");
		return;
	}

	uint64_t base;
	if (fdt_translate_address(soc, root, SOC_PERIPHERAL_BUS_BASE, &base) == SUCCESS)
		g_peripheral_base = (uintptr_t) base;
	else
		add_error("DTB: no /soc translation for the peripherals, using the default base\n");

	if (fdt_translate_address(soc, root, SOC_ARM_LOCAL_BUS_BASE, &base) == SUCCESS)
		g_arm_local_base = (uintptr_t) base;
	else
		add_error("DTB: no /soc translation for the ARM-local peripherals, using the default base\n");
}

// Reads the DTB first (the peripheral base addresses come from it), then brings up the GIC, asks
// the firmware for the board information in one property request and formats it into
// s_info, and builds the RAM map. A failed query only replaces the text with a note:
// the board works without it, so platform_init() still succeeds.
int platform_init(uintptr_t dtb)
{
	s_idx_errors = 0;

	// Before any MMIO: the peripheral base comes from the device tree
	if (fdt_init(dtb) == SUCCESS)
		peripheral_base_init_from_dtb();
	else
		add_error("DTB: invalid header, no RAM map and default peripheral bases\n");

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
	}
	else
	{
		uint64_t serial = ((uint64_t) board_serial->high << 32) | board_serial->low;

		(void) snprintf(s_info, sizeof s_info, "Firmware revision: %x\n"
		                                       "Board model:       %x\n"
		                                       "Board revision:    %x\n"
		                                       "Serial:            %lx\n"
		                                       "ARM memory:        %u MiB at %x\n"
		                                       "VC memory:         %u MiB at %x",
		                firmware_version->version, board_model->model, board_revision->revision, serial, arm_memory->size / (1024 * 1024), arm_memory->base, vc_memory->size / (1024 * 1024), vc_memory->base);
	}

	mem_init_from_dtb();

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

const char* platform_get_init_errors()
{
	return (s_idx_errors != 0) ? s_errors : NULL;
}

const char* platform_get_info()
{
	return s_info;
}

size_t platform_mem_regions(struct mem_region* out, size_t max)
{
	for (size_t i = 0; i < s_ram_count && i < max; i++)
	{
		out[i].base = (uintptr_t) s_ram[i].base;
		out[i].size = (size_t) (s_ram[i].end - s_ram[i].base);
	}
	return s_ram_count;
}
