// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#pragma once

#include <stdint.h>

#define FONT_WIDTH 8 // native size, tied to the glyph data
#define FONT_HEIGHT 14

const uint8_t* font_get_glyph(unsigned char c);