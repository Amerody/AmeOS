// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file console_fb.h
 * @brief Console sink that draws text on the framebuffer.
 * @ingroup console
 *
 * Implemented by src/drivers/console_fb.c. It is portable: it uses
 * only hal_fb, so it works unchanged on any platform that provides an implementation
 * of that HAL. Everything hardware-specific is hidden inside the platform's hal_fb
 * implementation.
 *
 * Text is drawn with the gfx font in a grid of cells, each FONT_WIDTH x FONT_HEIGHT
 * pixels times the scale (see console_set_scale()). Each character is drawn on a black
 * cell background in the current colour (see console_set_color()). When the cursor
 * moves past the last row, the screen scrolls up by one row. @c '\\n' (next row),
 * @c '\\r' (start of the row) and @c '\\b' (one cell back) are interpreted; every other
 * byte is drawn as a glyph. The cursor cell is shown by inverting its pixels. The sink
 * is output-only: it has no @c getc. Its @c clear (see console_clear()) fills the
 * screen with black and moves the cursor to the top-left cell.
 *
 * The cursor, colour and scale are kept in file statics, with no locking.
 *
 * Called from kmain() after console_init():
 *
 * @code
 * console_init();
 * console_fb_init();
 * @endcode
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief Initialise the framebuffer and register it as a console sink.
	 *
	 * Registers an output-only sink named "fb" with console_register(), then calls
	 * hal_fb_init(800, 600, 32) and moves the cursor to the top-left cell. The
	 * resolution was chosen arbitrarily, while the color format is RGBA8 (32 bits).
	 *
	 * Call after console_init().
	 *
	 * @retval 0 The framebuffer is set up and registered
	 * @retval -ERROR_BUSY console_fb_init() was already called, even if that earlier call
	 *                failed
	 * @return Otherwise the negative error from hal_fb_init()
	 *
	 * @bug The result of console_register() is ignored, so -ERROR_NO_SPACE is never returned:
	 *      when the registry is full, the framebuffer is set up but gets no console
	 *      output.
	 * @bug The sink is registered before hal_fb_init(). If hal_fb_init() fails, the
	 *      sink stays registered on a framebuffer that isn't set up.
	 */
	int console_fb_init();

	/**
	 * @brief Sets the colour of the text drawn from now on.
	 *
	 * Text already on the screen keeps its colour. The cell background is always black.
	 *
	 * @param color Colour in the framebuffer's pixel format: use RGB() or a COLOR_*
	 *              value from gfx/color.h
	 */
	void console_set_color(uint32_t color);

	/**
	 * @brief Sets the text scale: each font pixel is drawn as a @p scale x @p scale block.
	 *
	 * Recomputes the number of columns and rows, clears the whole screen to black (text
	 * drawn at the old scale wouldn't line up with the new cells) and moves the cursor
	 * to the top-left cell.
	 *
	 * Does nothing if @p scale is 0 or if a single cell wouldn't fit on the screen,
	 * which is also the case before console_fb_init().
	 *
	 * @param scale Scale factor; 1 is the font's native size
	 *
	 * @bug The fit check computes FONT_WIDTH * @p scale and FONT_HEIGHT * @p scale in 32
	 *      bits. For @p scale 0x80000000 both wrap to 0, the check passes, and the column
	 *      and row counts then divide by a cell size of 0 (undefined behaviour in C).
	 */
	void console_set_scale(uint32_t scale);

#ifdef __cplusplus
}
#endif