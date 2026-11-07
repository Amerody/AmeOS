#pragma once

// implemented in platform/<platform>/framebuffer

#include <stdint.h>

typedef struct
{
	uint32_t* buffer;
	uint32_t pitch;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
} framebuffer_t;

int hal_fb_init(uint32_t width, uint32_t height, uint32_t depth);

void hal_fb_set_pixel(uint32_t x, uint32_t y, uint32_t color);

void hal_fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);

void hal_fb_draw_line(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t color);

framebuffer_t* hal_fb_get_info();