// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#include <common/mmio.h>
#include <hal/hal_fb.h>
#include <mailbox/mailbox.h>

/**
 * @file fb.c
 * @brief hal_fb implementation for the Raspberry Pi 4: the framebuffer is requested
 *        from the firmware through the mailbox (see hal_fb.h)
 * @ingroup framebuffer
 */

/// @brief Absolute value of @p x
#define ABS(x)            ((x) < 0 ? -(x) : (x))

/// @brief Mask used to invert a cell's color via XOR. Dependant on endianness (RGBA vs ABGR)
#define COLOR_INVERT_MASK 0x00FFFFFFu

/// @brief The framebuffer description returned by hal_fb_get_info()
static struct framebuffer s_fb_info;

// One property request with the tags SET_PHYS_WH, SET_VIRT_WH, SET_DEPTH, SET_VIRT_OFF,
// GET_PITCH and ALLOCATE. Every failure (mailbox error, or no buffer allocated) returns -1.
// for the mailbox protocol refer to
// https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
int hal_fb_init(uint32_t width, uint32_t height, uint32_t depth)
{
	mailbox_buffer[0] = 35 * 4; // Total size
	mailbox_buffer[1] = 0;      // Request

	// Set physical dimensions
	mailbox_buffer[2] = MBOX_TAG_FB_SET_PHYS_WH;
	mailbox_buffer[3] = 8;      // Value size
	mailbox_buffer[4] = 0;      // request/response code
	mailbox_buffer[5] = width;  // value buffer word 1
	mailbox_buffer[6] = height; // value buffer word 2

	// Set virtual dimensions
	mailbox_buffer[7]  = MBOX_TAG_FB_SET_VIRT_WH;
	mailbox_buffer[8]  = 8;
	mailbox_buffer[9]  = 0;
	mailbox_buffer[10] = width;
	mailbox_buffer[11] = height;

	// Set depth (32-bit ARGB)
	mailbox_buffer[12] = MBOX_TAG_FB_SET_DEPTH;
	mailbox_buffer[13] = 4;
	mailbox_buffer[14] = 0;
	mailbox_buffer[15] = depth;

	// Set virtual offset (critical for proper initialization!)
	mailbox_buffer[16] = MBOX_TAG_FB_SET_VIRT_OFF;
	mailbox_buffer[17] = 8;
	mailbox_buffer[18] = 0;
	mailbox_buffer[19] = 0; // X offset
	mailbox_buffer[20] = 0; // Y offset

	// Get pitch
	mailbox_buffer[21] = MBOX_TAG_FB_GET_PITCH;
	mailbox_buffer[22] = 4;
	mailbox_buffer[23] = 0;
	mailbox_buffer[24] = 0;

	// Allocate buffer
	mailbox_buffer[25] = MBOX_TAG_FB_ALLOCATE;
	mailbox_buffer[26] = 8;
	mailbox_buffer[27] = 0;
	mailbox_buffer[28] = 4096; // Alignment (4096 required!)
	mailbox_buffer[29] = 0;

	// End tag
	mailbox_buffer[30] = 0;

	if (mailbox_call(MAILBOX_CH_PROP) != 0)
	{
		return -1;
	}

	// Check if allocation succeeded
	if (mailbox_buffer[28] == 0)
	{
		return -1;
	}
	// if allocation succeeded, use the values picked by the firmware
	// in case they aren't the same
	s_fb_info.width  = mailbox_buffer[5];
	s_fb_info.height = mailbox_buffer[6];
	s_fb_info.depth  = mailbox_buffer[15];
	if (s_fb_info.depth != 32)
		return -1; // if the depth isn't 32 bits, fail

	// Extract framebuffer address and pitch from response
	s_fb_info.buffer = (uint32_t*) (uint64_t) (mailbox_buffer[28] & 0x3FFFFFFF);
	s_fb_info.pitch  = mailbox_buffer[24];
	if (mailbox_buffer[29] < s_fb_info.pitch * s_fb_info.height)
		return -1;

	return 0;
}

void hal_fb_set_pixel(uint32_t x, uint32_t y, uint32_t color)
{
	if (x >= s_fb_info.width || y >= s_fb_info.height)
		return;

	s_fb_info.buffer[y * (s_fb_info.pitch / 4) + x] = color;
}

void hal_fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
	for (uint64_t row = y; row < (uint64_t) y + h && row < s_fb_info.height; row++)
	{
		for (uint64_t col = x; col < (uint64_t) x + w && col < s_fb_info.width; col++)
		{
			hal_fb_set_pixel(col, row, color);
		}
	}
}

// Uses Bresenham's Algorithm to draw lines efficiently without floating-point math
void hal_fb_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color)
{
	int dx  = ABS(x1 - x0);
	int dy  = ABS(y1 - y0);
	int sx  = x0 < x1 ? 1 : -1;
	int sy  = y0 < y1 ? 1 : -1;
	int err = dx - dy;

	while (1)
	{
		hal_fb_set_pixel(x0, y0, color);
		if (x0 == x1 && y0 == y1)
			break;

		int e2 = 2 * err;
		if (e2 > -dy)
		{
			err -= dy;
			x0 += sx;
		}
		if (e2 < dx)
		{
			err += dx;
			y0 += sy;
		}
	}
}

void hal_fb_invert_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
	if (x >= s_fb_info.width || y >= s_fb_info.height)
		return;

	for (uint64_t row = y; row < (uint64_t) y + h && row < s_fb_info.height; row++)
	{
		for (uint64_t col = x; col < (uint64_t) x + w && col < s_fb_info.width; col++)
		{
			s_fb_info.buffer[row * (s_fb_info.pitch / 4) + col] ^= COLOR_INVERT_MASK;
		}
	}
}

struct framebuffer* hal_fb_get_info() { return &s_fb_info; }
