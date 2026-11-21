// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#include "gfx/font.h"

#include <stdint.h>

/**
 * @file font.c
 * @brief Glyph table of the built-in font and its lookup (see font.h)
 * @ingroup gfx
 */

/**
 * @brief Glyph bitmaps, one row of #FONT_HEIGHT bytes per byte value.
 *
 * Filled by the designated initializers in gfx/terminus_14.inc; entries the file doesn't
 * list stay all zero (blank).
 */
static const uint8_t s_font_data[256][FONT_HEIGHT] = {
#include "gfx/terminus_14.inc"
};

// The table has an entry for every unsigned char value, so no bounds check is needed.
const uint8_t* font_get_glyph(unsigned char c)
{
	return s_font_data[c];
}