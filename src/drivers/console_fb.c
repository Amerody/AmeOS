#include "drivers/console/console_fb.h"

#include <gfx/color.h>
#include <gfx/font.h>
#include <hal/hal_fb.h>
#include <kernel/console.h>
#include <kernel/errno.h>

#include <lib/memory.h>

static int s_init = 0;

static uint32_t s_cursor_x = 0;
static uint32_t s_cursor_y = 0;
static uint32_t s_max_cols = 0;
static uint32_t s_max_rows = 0;

static uint32_t s_color = COLOR_WHITE;
static uint32_t s_scale = 1;

static inline uint32_t cell_w(void) { return FONT_WIDTH * s_scale; }
static inline uint32_t cell_h(void) { return FONT_HEIGHT * s_scale; }

static void console_fb_scroll()
{
	framebuffer_t* fb = hal_fb_get_info();
	if (!fb || !fb->buffer)
		return;

	uint32_t line_size = fb->pitch * cell_h();
	uint32_t total_size = fb->pitch * fb->height;

	// Move all lines up by one row
	memmove(fb->buffer, (uint8_t*) fb->buffer + line_size, total_size - line_size);

	// Clear the last line
	memset((uint8_t*) fb->buffer + total_size - line_size, 0, line_size);

	s_cursor_y = s_max_rows - 1;
}

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

static void newline(void)
{
	s_cursor_x = 0;
	if (++s_cursor_y >= s_max_rows)
		console_fb_scroll();
}

static void console_fb_putc(char c)
{
	if (c == '\n')
	{
		newline();
		return;
	}

	draw_glyph(s_cursor_x * cell_w(), s_cursor_y * cell_h(), c);

	if (++s_cursor_x >= s_max_cols)
		newline();
}

int console_fb_init()
{
	if (s_init != 0)
		return -EBUSY;

	static struct console fb_console;
	fb_console.name = "fb";
	fb_console.putc = console_fb_putc;
	fb_console.getc = NULL;
	console_register(&fb_console);

	int error = hal_fb_init(1920, 1080, 32);

	framebuffer_t* fb = hal_fb_get_info();
	s_cursor_x = 0;
	s_cursor_y = 0;
	s_max_cols = (fb) ? fb->width / FONT_HEIGHT : 0;
	s_max_rows = (fb) ? fb->height / FONT_HEIGHT : 0;

	return error;
}

void console_set_color(uint32_t color) { s_color = color; }

void console_set_scale(uint32_t scale)
{
	framebuffer_t* fb = hal_fb_get_info();
	if (scale == 0 || !fb || FONT_WIDTH * scale > fb->width || FONT_HEIGHT * scale > fb->height)
		return; // would leave zero columns or rows

	s_scale = scale;

	s_max_cols = fb->width / cell_w();
	s_max_rows = fb->height / cell_h();

	hal_fb_fill_rect(0, 0, fb->width, fb->height, COLOR_BLACK); // old text is at the old scale
	s_cursor_x = s_cursor_y = 0;
}