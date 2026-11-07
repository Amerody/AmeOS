#include "gfx/font.h"

#include <stdint.h>

static const uint8_t font_data[256][FONT_HEIGHT] = {
#include "gfx/terminus_14.inc"
};

// Returns a pointer to the 8-byte array defining a character.
const uint8_t* font_get_glyph(unsigned char c)
{
	// Masking with 0x7F (127) serves two purposes:
	// 1. It forces any negative char values or chars above 127 to safely wrap within bounds.
	// 2. It avoids the need for a branching if-statement.
	return font_data[c];
}