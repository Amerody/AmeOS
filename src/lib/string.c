#include "lib/string.h"

/**
 * @file string.c
 * @brief Implementation of the freestanding string functions (see string.h)
 * @ingroup lib
 */

/// @brief The byte that ends a string
#define NULL_TERMINATOR '\0'

size_t strlen(const char* s)
{
	size_t len = 0;
	while (*s++)
		len++;
	return len;
}

int strcmp(const char* s1, const char* s2)
{
	while (*s1 && (*s1 == *s2))
	{
		s1++;
		s2++;
	}
	return *(const uint8_t*) s1 - *(const uint8_t*) s2;
}

int strncmp(const char* s1, const char* s2, size_t n)
{
	for (size_t i = 0; i < n; i++)
	{
		int sub = s1[i] - s2[i];
		if (sub == 0)
		{
			if (s1[i] == NULL_TERMINATOR)
				break;
			continue;
		}
		return sub;
	}

	return 0;
}

char* strcpy(char* dest, const char* src)
{
	char* oDest = dest;
	while ((*(uint8_t*) dest++ = (*(uint8_t*) src++)))
		;
	return oDest;
}

char* strncpy(char* dest, const char* src, size_t n)
{
	char* oDest = dest;

	while (n > 0 && *src != NULL_TERMINATOR)
	{
		*(uint8_t*) dest++ = *(uint8_t*) src++;
		n--;
	}

	while (n--)
		*dest++ = NULL_TERMINATOR;

	return oDest;
}