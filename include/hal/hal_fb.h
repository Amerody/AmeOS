// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file hal_fb.h
 * @brief Framebuffer contract: a linear pixel buffer in memory and simple drawing.
 * @ingroup framebuffer
 *
 * Implemented by src/platform/\<PLATFORM\>/. The implementation gets the buffer
 * from the hardware or firmware; callers only ask for a size and a depth.
 *
 * Coordinates are in pixels, with (0, 0) at the top-left corner. Colours are 32-bit
 * values in the framebuffer's pixel format (see gfx/color.h), and the drawing
 * functions write one 32-bit word per pixel, so they assume a depth of 32. They clip
 * to the screen: pixels outside it are skipped.
 *
 * There is no locking and no double buffering: every drawing call writes straight to
 * the visible buffer.
 *
 * See platform.h for the shared conventions (errors, contexts, init order).
 */
#pragma once

#include <stdint.h>

/// @brief Framebuffer description, filled by hal_fb_init() and returned by hal_fb_get_info()
struct framebuffer
{
	uint32_t* buffer; ///< First pixel (top-left); NULL until hal_fb_init() succeeds
	uint32_t  pitch;  ///< Bytes per row; can be more than width * 4
	uint32_t  width;  ///< Width in pixels
	uint32_t  height; ///< Height in pixels
	uint32_t  depth;  ///< Bits per pixel, as given to hal_fb_init()
};

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief Sets up a framebuffer of the given size and depth.
	 *
	 * On success, hal_fb_get_info() describes the buffer and the drawing functions can
	 * be used. May block while the hardware or firmware answers.
	 *
	 * @param width Width in pixels
	 * @param height Height in pixels
	 * @param depth Bits per pixel; the drawing functions assume 32
	 *
	 * @retval 0 The framebuffer is ready
	 * @return Otherwise a negative error code (rpi4: always -1, for a mailbox error or when
	 *         no buffer was allocated)
	 */
	int hal_fb_init(uint32_t width, uint32_t height, uint32_t depth);

	/**
	 * @brief Sets one pixel.
	 *
	 * Does nothing if (@p x, @p y) is outside the screen.
	 *
	 * @param x Column, in pixels
	 * @param y Row, in pixels
	 * @param color Colour in the framebuffer's pixel format
	 */
	void hal_fb_set_pixel(uint32_t x, uint32_t y, uint32_t color);

	/**
	 * @brief Fills a rectangle with one colour, clipped to the screen.
	 *
	 * @param x Column of the top-left corner, in pixels
	 * @param y Row of the top-left corner, in pixels
	 * @param w Width in pixels
	 * @param h Height in pixels
	 * @param color Colour in the framebuffer's pixel format
	 */
	void hal_fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);

	/**
	 * @brief Draws a straight line from (@p x0, @p y0) to (@p x1, @p y1), both ends
	 *        included, clipped to the screen.
	 *
	 * @param x0 Column of the start point, in pixels (signed)
	 * @param y0 Row of the start point, in pixels (signed)
	 * @param x1 Column of the end point, in pixels (signed)
	 * @param y1 Row of the end point, in pixels (signed)
	 * @param color Colour in the framebuffer's pixel format
	 */
	void hal_fb_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);

	/**
	 * @brief Inverts the colour of every pixel in a rectangle, clipped to the screen.
	 *
	 * Each pixel's red, green and blue are inverted and its alpha is kept (rpi4: XOR with
	 * 0x00FFFFFF, which matches the layout in gfx/color.h). Inverting the same rectangle
	 * twice restores it, which is how console_fb shows and hides its cursor. Does nothing if
	 * (@p x, @p y) is outside the screen.
	 *
	 * @param x Column of the top-left corner, in pixels
	 * @param y Row of the top-left corner, in pixels
	 * @param w Width in pixels
	 * @param h Height in pixels
	 */
	void hal_fb_invert_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h);

	/**
	 * @brief Returns the description of the framebuffer.
	 *
	 * @return Pointer to the description; never NULL. Before a successful hal_fb_init(),
	 *         its @c buffer is NULL.
	 */
	struct framebuffer* hal_fb_get_info();

#ifdef __cplusplus
}
#endif
