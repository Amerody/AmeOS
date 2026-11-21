// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

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

/**
 * @brief Tells whether a byte is whitespace for strtoi() and strtox().
 *
 * @param c Byte to test
 *
 * @retval true @p c is a space, @c '\\t', @c '\\n', @c '\\r', @c '\\v' or @c '\\f'
 * @retval false Any other byte
 */
static inline bool is_space(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

/**
 * @brief Tells whether a byte is a decimal digit.
 *
 * @param c Byte to test
 *
 * @retval true @p c is @c '0' to @c '9'
 * @retval false Any other byte
 */
static inline bool is_digit(char c)
{
	return c >= '0' && c <= '9';
}

/**
 * @brief Converts a hexadecimal digit to its value.
 *
 * @param c Byte to convert: @c '0'-'9', @c 'a'-'f' or @c 'A'-'F'
 *
 * @return 0 to 15 for a hex digit
 * @retval -1 @p c is not a hex digit
 */
static inline int hex_val(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

int strtoi(const char* s, int32_t* out)
{
	if (!s || !out)
		return -1;

	while (is_space(*s))
		s++;

	bool neg = false;
	if (*s == '+' || *s == '-')
	{
		neg = (*s == '-');
		s++;
	}

	if (!is_digit(*s))
		return -1; // no digits at all

	// |INT32_MIN| is one larger than INT32_MAX
	const uint32_t limit = neg ? (uint32_t) INT32_MAX + 1u : (uint32_t) INT32_MAX;
	uint32_t val = 0;

	while (is_digit(*s))
	{
		uint32_t d = (uint32_t) (*s - '0');
		if (val > (limit - d) / 10) /* val * 10 + d would exceed limit */
			return -1;
		val = val * 10 + d;
		s++;
	}

	while (is_space(*s))
		s++;
	if (*s != '\0')
		return -1; /* trailing garbage */

	if (neg)
		*out = (val == (uint32_t) INT32_MAX + 1u) ? INT32_MIN : -(int32_t) val;
	else
		*out = (int32_t) val;
	return 0;
}

int strtox(const char* s, uint32_t* out)
{
	if (!s || !out)
		return -1;

	while (is_space(*s))
		s++;

	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
		s += 2;

	if (hex_val(*s) < 0)
		return -1; // no digits at all

	uint32_t val = 0;
	int d;
	while ((d = hex_val(*s)) >= 0)
	{
		if (val >> 28) // top nibble set: shifting would overflow
			return -1;
		val = (val << 4) | (uint32_t) d;
		s++;
	}

	while (is_space(*s))
		s++;
	if (*s != '\0')
		return -1; // trailing garbage

	*out = val;
	return 0;
}
