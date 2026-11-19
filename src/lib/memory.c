#include "lib/memory.h"

/**
 * @file memory.c
 * @brief Byte-at-a-time implementation of the freestanding memory functions (see memory.h)
 * @ingroup lib
 */

void* memcpy(void* dest, const void* src, size_t n)
{
	uint8_t* d = (uint8_t*) dest;
	const uint8_t* s = (const uint8_t*) src;
	while (n--)
		*d++ = *s++;
	return dest;
}

void* memset(void* s, uint8_t c, size_t n)
{
	uint8_t* p = (uint8_t*) s;
	while (n--)
		*p++ = (uint8_t) c;
	return s;
}

void* memmove(void* dest, const void* src, size_t n)
{
	uint8_t* d = (uint8_t*) dest;
	const uint8_t* s = (const uint8_t*) src;

	if (d < s)
	{
		// Copy forward
		while (n--)
			*d++ = *s++;
	}
	else if (d > s)
	{
		// Copy backward (handles overlapping regions)
		d += n;
		s += n;
		while (n--)
			*--d = *--s;
	}
	return dest;
}