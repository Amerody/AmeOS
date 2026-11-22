// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file color.h
 * @brief 32-bit colour values for the framebuffer: the RGB() macro and a few named colours.
 * @ingroup gfx
 *
 * A colour is a @c uint32_t with red in bits 0-7, green in bits 8-15, blue in bits 16-23
 * and 0xFF in bits 24-31 (alpha, opaque). hal_fb writes these values to the buffer as they
 * are, so they only show the right colour when the framebuffer uses that pixel layout.
 *
 * Uses @c uint32_t but doesn't include <stdint.h>: include it first.
 */
#pragma once

/**
 * @brief Builds a colour from red, green and blue components.
 *
 * Each component is cast to @c uint32_t and masked to 8 bits, so values above 255 wrap.
 * Alpha is always 0xFF.
 *
 * @param r Red, 0-255
 * @param g Green, 0-255
 * @param b Blue, 0-255
 */
#define RGB(r, g, b) \
	(0xFF000000u | (((uint32_t) (b) & 0xFFu) << 16) | (((uint32_t) (g) & 0xFFu) << 8) | ((uint32_t) (r) & 0xFFu))

/// @brief Black
#define COLOR_BLACK RGB(0, 0, 0)
/// @brief White
#define COLOR_WHITE RGB(255, 255, 255)
/// @brief Pure red
#define COLOR_RED   RGB(255, 0, 0)
/// @brief Pure green
#define COLOR_GREEN RGB(0, 255, 0)
/// @brief Pure blue
#define COLOR_BLUE  RGB(0, 0, 255)
