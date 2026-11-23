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
	alignas(16) uint32_t buf[32];
	struct mailbox_prop  p;

	mailbox_prop_begin(&p, buf, 32);

	uint32_t* pwh   = mailbox_prop_add(&p, MBOX_TAG_FB_SET_PHYS_WH, 8);
	uint32_t* vwh   = mailbox_prop_add(&p, MBOX_TAG_FB_SET_VIRT_WH, 8);
	uint32_t* dep   = mailbox_prop_add(&p, MBOX_TAG_FB_SET_DEPTH, 4);
	uint32_t* voff  = mailbox_prop_add(&p, MBOX_TAG_FB_SET_VIRT_OFF, 8);
	uint32_t* pitch = mailbox_prop_add(&p, MBOX_TAG_FB_GET_PITCH, 4);
	uint32_t* alloc = mailbox_prop_add(&p, MBOX_TAG_FB_ALLOCATE, 8);
	if (!alloc)
		return -1;

	// Set physical dimensions
	pwh[0] = width;
	pwh[1] = height;

	// Set virtual dimensions
	vwh[0] = width;
	vwh[1] = height;

	// Set depth (32-bit ARGB)
	dep[0] = depth;

	// Set virtual offset (critical for proper initialization!)
	voff[0] = 0; // X offset
	voff[1] = 0; // Y offset

	// Allocate buffer
	alloc[0] = 4096; // Alignment (4096 required!)

	// Check if everything succeeded
	if (mailbox_prop_send(&p) != 0 || !mailbox_prop_tag_ok(pwh) || !mailbox_prop_tag_ok(vwh) || !mailbox_prop_tag_ok(dep) || !mailbox_prop_tag_ok(voff) || !mailbox_prop_tag_ok(pitch) || !mailbox_prop_tag_ok(alloc))
		return -1;

	if (pwh[0] != vwh[0] || pwh[1] != vwh[1])
		return -1; // if the physical and virtual dimensions are not the same, fail

	if (dep[0] != depth)
		return -1; // if the depth isn't what we asked for, fail

	if (alloc[1] < pitch[0] * pwh[1])
		return -1;

	// if everything succeeded, use the values picked by the firmware
	// in case they aren't the same
	s_fb_info.width  = pwh[0];
	s_fb_info.height = pwh[1];
	s_fb_info.depth  = dep[0];

	// Extract framebuffer address and pitch from response
	s_fb_info.buffer = (uint32_t*) (uint64_t) (alloc[0] & 0x3FFFFFFF);
	s_fb_info.pitch  = pitch[0];

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
