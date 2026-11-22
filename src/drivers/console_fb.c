// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#include "drivers/console/console_fb.h"

#include <gfx/color.h>
#include <gfx/font.h>
#include <hal/hal_fb.h>
#include <kernel/console.h>
#include <kernel/error.h>
#include <lib/memory.h>

/**
 * @file console_fb.c
 * @brief Framebuffer console sink: draws console output as text on hal_fb (see console_fb.h)
 * @ingroup console
 */

/// @brief True once console_fb_init() has run, even if hal_fb_init() failed
static bool s_init = false;

/// @brief Cursor column, in cells
static uint32_t s_cursor_x = 0;
/// @brief Cursor row, in cells
static uint32_t s_cursor_y = 0;
/// @brief Number of columns on the screen
static uint32_t s_max_cols = 0;
/// @brief Number of rows on the screen
static uint32_t s_max_rows = 0;

/// @brief Text color, set by console_set_color()
static uint32_t s_color = COLOR_WHITE;
/// @brief Size on screen of one font pixel, in pixels; set by console_set_scale()
static uint32_t s_scale = 1;

/// @brief True while the cell at the cursor is drawn inverted (see cursor_show())
static bool s_cursor_visible = false;

/// @brief Width of one character cell at the current scale
/// @return Width in pixels
static inline uint32_t cell_w(void) { return FONT_WIDTH * s_scale; }
/// @brief Height of one character cell at the current scale
/// @return Height in pixels
static inline uint32_t cell_h(void) { return FONT_HEIGHT * s_scale; }

/**
 * @brief Scrolls the screen up by one row of cells and clears the bottom row.
 *
 * Moves the framebuffer contents directly, then puts the cursor on the last row.
 * Does nothing if the framebuffer has no buffer.
 */
static void console_fb_scroll()
{
	struct framebuffer* fb = hal_fb_get_info();
	if (!fb || !fb->buffer)
		return;

	uint32_t line_size  = fb->pitch * cell_h();
	uint32_t total_size = fb->pitch * fb->height;

	// Move all lines up by one row
	memmove(fb->buffer, (uint8_t*) fb->buffer + line_size, total_size - line_size);

	// Clear the last line
	memset((uint8_t*) fb->buffer + total_size - line_size, 0, line_size);

	s_cursor_y = s_max_rows - 1;
}

/**
 * @brief Draws one character cell: a black background, then the glyph in #s_color.
 *
 * Each glyph row is one byte, with the leftmost pixel in bit 7. Runs of lit pixels in
 * a row are drawn as one rectangle.
 *
 * @param px X position of the cell's top-left corner, in pixels
 * @param py Y position of the cell's top-left corner, in pixels
 * @param c Character to draw
 */
static void draw_glyph(uint32_t px, uint32_t py, char c)
{
	const uint8_t* glyph = font_get_glyph(c);

	hal_fb_fill_rect(px, py, cell_w(), cell_h(), COLOR_BLACK); // cell background

	for (uint32_t row = 0; row < FONT_HEIGHT; row++)
	{
		uint32_t col = 0;
		while (col < FONT_WIDTH)
		{
			if (!(glyph[row] & (0x80u >> col)))
			{
				col++;
				continue;
			}

			uint32_t start = col; // merge a run of lit pixels
			while (col < FONT_WIDTH && (glyph[row] & (0x80u >> col)))
				col++;

			hal_fb_fill_rect(px + start * s_scale, py + row * s_scale, (col - start) * s_scale, s_scale, s_color);
		}
	}
}

/// @brief Moves the cursor to the start of the next row, scrolling if it was on the last one
static void newline(void)
{
	s_cursor_x = 0;
	if (++s_cursor_y >= s_max_rows)
		console_fb_scroll();
}

/**
 * @brief Removes the cursor by inverting the cursor cell back to its normal pixels.
 *
 * Does nothing if the cursor isn't shown. Must be called before the cursor cell is
 * drawn over or the cursor moves, so the cell isn't left inverted.
 */
static void cursor_hide()
{
	if (!s_cursor_visible)
		return;

	uint32_t start_x = s_cursor_x * FONT_WIDTH * s_scale;
	uint32_t start_y = s_cursor_y * FONT_HEIGHT * s_scale;
	hal_fb_invert_rect(start_x, start_y, FONT_WIDTH * s_scale, FONT_HEIGHT * s_scale);
	s_cursor_visible = false;
}

/**
 * @brief Shows the cursor by inverting the pixels of the cell at the cursor position.
 *
 * Does nothing if the cursor is already shown, so the cell is never inverted twice.
 */
static void cursor_show()
{
	if (s_cursor_visible)
		return;

	uint32_t start_x = s_cursor_x * FONT_WIDTH * s_scale;
	uint32_t start_y = s_cursor_y * FONT_HEIGHT * s_scale;
	hal_fb_invert_rect(start_x, start_y, FONT_WIDTH * s_scale, FONT_HEIGHT * s_scale);
	s_cursor_visible = true;
}

/**
 * @brief Console @c putc of the framebuffer sink.
 *
 * @c '\\n' moves to the next row; any other byte is drawn at the cursor, which then
 * advances, wrapping to the next row at the end of a line.
 *
 * @c '\\r' moves the cursor to the beginning of the row.
 *
 * @c '\\b' moves the cursor back one cell.
 *
 * The cursor is hidden before the byte is handled and shown again afterwards.
 *
 * @param c Byte to output
 *
 * @bug @c '\\b' stops at column 0 and doesn't move back to the end of the previous row.
 *      When a shell line has wrapped, backspacing past the start of a row leaves the
 *      characters on the previous row on screen, although the shell removed them from
 *      its line buffer.
 */
static void console_fb_putc(char c)
{
	// hide the cursor
	cursor_hide();

	if (c == '\n')
	{
		newline();
	}
	else if (c == '\r')
	{
		s_cursor_x = 0;
	}
	else if (c == '\b')
	{
		if (s_cursor_x > 0)
			s_cursor_x--;
	}
	else
	{
		draw_glyph(s_cursor_x * cell_w(), s_cursor_y * cell_h(), c);

		if (++s_cursor_x >= s_max_cols)
			newline();
	}

	// show the cursor again
	cursor_show();
}

/**
 * @brief Console @c clear of the framebuffer sink: fills the screen with black and moves
 *        the cursor to the top-left cell.
 *
 * Does nothing if the framebuffer has no buffer.
 */
static void console_fb_clear()
{
	struct framebuffer* fb = hal_fb_get_info();
	if (!fb || !fb->buffer)
		return;

	cursor_hide();
	hal_fb_fill_rect(0, 0, fb->width, fb->height, COLOR_BLACK);
	s_cursor_x = 0;
	s_cursor_y = 0;
	cursor_show();
}

/**
 * @brief Console @c flush of the framebuffer sink: does nothing.
 *
 * console_fb_putc() draws each character into the framebuffer before it returns, so
 * there is no pending output to wait for. A NULL @c flush would behave the same.
 */
static void console_fb_flush()
{
	// nothing to do
	return;
}

int console_fb_init()
{
	if (s_init)
		return -ERROR_BUSY;

	static struct console fb_console;
	fb_console.name  = "fb";
	fb_console.putc  = console_fb_putc;
	fb_console.getc  = NULL;
	fb_console.clear = console_fb_clear;
	fb_console.flush = console_fb_flush;
	console_register(&fb_console);

	int error = hal_fb_init(800, 600, 32);

	struct framebuffer* fb = hal_fb_get_info();
	s_cursor_x             = 0;
	s_cursor_y             = 0;
	s_max_cols             = (fb) ? fb->width / FONT_WIDTH : 0;
	s_max_rows             = (fb) ? fb->height / FONT_HEIGHT : 0;

	s_init = true;
	return error;
}

void console_set_color(uint32_t color) { s_color = color; }

void console_set_scale(uint32_t scale)
{
	struct framebuffer* fb = hal_fb_get_info();
	if (scale == 0 || !fb || FONT_WIDTH * scale > fb->width || FONT_HEIGHT * scale > fb->height)
		return; // would leave zero columns or rows

	s_scale = scale;

	s_max_cols = fb->width / cell_w();
	s_max_rows = fb->height / cell_h();

	console_fb_clear(); // old text is at the old scale
	s_cursor_x = s_cursor_y = 0;
}