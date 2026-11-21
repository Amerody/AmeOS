// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#pragma once

#define RGB(r, g, b) \
	(0xFF000000u | (((uint32_t) (b) & 0xFFu) << 16) | (((uint32_t) (g) & 0xFFu) << 8) | ((uint32_t) (r) & 0xFFu))

#define COLOR_BLACK RGB(0, 0, 0)
#define COLOR_WHITE RGB(255, 255, 255)
#define COLOR_RED RGB(255, 0, 0)
#define COLOR_GREEN RGB(0, 255, 0)
#define COLOR_BLUE RGB(0, 0, 255)