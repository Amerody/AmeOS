// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file soc.h
 * @brief BCM2711 constants shared by the rpi4 drivers.
 * @ingroup drivers
 */
#pragma once

#include <stdint.h>

/**
 * @brief VideoCore bus address of the main peripheral window.
 *
 * This is the address the device tree's /soc children use in their @c reg
 * (e.g. the mini UART at 0x7e215040), and the one the BCM2711 ARM Peripherals
 * document lists registers at (section 1.2, "Address map"). The CPU sees the
 * window elsewhere: /soc's @c ranges gives where (see #PERIPHERAL_BASE).
 */
#define SOC_PERIPHERAL_BUS_BASE 0x7E000000UL

/// @brief CPU address of the peripheral window in low peripheral mode, used until the DTB says otherwise
#define SOC_PERIPHERAL_BASE_DEFAULT 0xFE000000UL

/**
 * @brief CPU address of the main peripheral window.
 *
 * Starts as #SOC_PERIPHERAL_BASE_DEFAULT. platform_init() replaces it, before any
 * driver touches a register, with the translation of #SOC_PERIPHERAL_BUS_BASE
 * through /soc's @c ranges, so a firmware in high peripheral mode
 * (arm_peri_high=1 in config.txt) works too. Not written after platform_init().
 */
extern uintptr_t g_peripheral_base;

/// @brief Peripheral base address: the drivers' register blocks are offsets from it
#define PERIPHERAL_BASE g_peripheral_base

/**
 * @brief Bus address of the ARM-local peripheral window (GIC-400, ARM timers, local interrupt controller).
 *
 * The device tree's /soc children in this window use it in their @c reg (e.g. the
 * GIC at 0x40041000). It's a separate entry of /soc's @c ranges from the main
 * window, so it can't be computed from #PERIPHERAL_BASE.
 */
#define SOC_ARM_LOCAL_BUS_BASE 0x40000000UL

/// @brief CPU address of the ARM-local window in low peripheral mode, used until the DTB says otherwise
#define SOC_ARM_LOCAL_BASE_DEFAULT 0xFF800000UL

/**
 * @brief CPU address of the ARM-local peripheral window.
 *
 * Starts as #SOC_ARM_LOCAL_BASE_DEFAULT; platform_init() replaces it with the
 * translation of #SOC_ARM_LOCAL_BUS_BASE through /soc's @c ranges, before the GIC
 * is touched. Not written after platform_init().
 */
extern uintptr_t g_arm_local_base;

/// @brief ARM-local base address: the GIC's register blocks are offsets from it
#define ARM_LOCAL_BASE g_arm_local_base
