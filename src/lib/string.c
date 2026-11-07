#include "lib/string.h"

#define NULL_TERMINATOR '\0'

uint32_t strlen(const char* s)
{
	uint32_t len = 0;
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

int strncmp(const char* s1, const char* s2, uint32_t n)
{
	for (uint32_t i = 0; i < n; i++)
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
	while ((*dest++ = *src++))
		;
	return oDest;
}

char* strncpy(char* dest, const char* src, uint32_t n)
{
	char* oDest = dest;

	while (n > 0 && *src != NULL_TERMINATOR)
	{
		*dest++ = *src++;
		n--;
	}

	while (n--)
		*dest++ = NULL_TERMINATOR;

	return oDest;
}