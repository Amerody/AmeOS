#include "lib/memory.h"

void* memcpy(void* dest, const void* src, uint32_t n)
{
	uint8_t* d = (uint8_t*) dest;
	const uint8_t* s = (const uint8_t*) src;
	while (n--)
		*d++ = *s++;
	return dest;
}

void* memset(void* s, int c, uint32_t n)
{
	uint8_t* p = (uint8_t*) s;
	uint8_t val = (uint8_t) c;
	while (n--)
		*p++ = (uint8_t) val;
	return s;
}

void* memmove(void* dest, const void* src, uint32_t n)
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