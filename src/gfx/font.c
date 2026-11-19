#include "gfx/font.h"

#include <stdint.h>

static const uint8_t font_data[256][FONT_HEIGHT] = {
#include "gfx/terminus_14.inc"
};

// Returns a pointer to the 8-byte array defining a character.
const uint8_t* font_get_glyph(unsigned char c)
{
	return font_data[c];
}