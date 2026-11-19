/**
 * @file string.h
 * @brief Freestanding string functions: strlen(), strcmp(), strncmp(), strcpy(), strncpy().
 * @ingroup lib
 *
 * The kernel is built without libc, so it provides its own versions of these. They keep
 * the libc names and behaviour, but lengths are @c uint32_t instead of @c size_t.
 * A string is a sequence of bytes ended by a NUL (@c '\0').
 *
 * Context: they touch no global state and never block, so they can be called from any
 * context, including interrupt handlers.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Counts the bytes of a string.
 *
 * @param s NUL-terminated string
 * @return Number of bytes before the terminating NUL
 */
size_t strlen(const char* s);

/**
 * @brief Compares two strings byte by byte.
 *
 * Bytes are compared as unsigned values (@c uint8_t).
 *
 * @param s1 First NUL-terminated string
 * @param s2 Second NUL-terminated string
 * @return 0 if the strings are equal, otherwise the difference between the first two bytes
 *         that differ: negative if @p s1 sorts before @p s2, positive if after
 */
int strcmp(const char* s1, const char* s2);

/**
 * @brief Compares at most @p n bytes of two strings.
 *
 * Stops at the first difference, at the end of the strings, or after @p n bytes,
 * whichever comes first. Bytes are compared as @c char values.
 *
 * @param s1 First string; it only needs to be NUL-terminated if shorter than @p n
 * @param s2 Second string; it only needs to be NUL-terminated if shorter than @p n
 * @param n Maximum number of bytes to compare
 * @return 0 if the compared bytes are equal (always the case when @p n is 0), otherwise the
 *         difference between the first two bytes that differ: negative if @p s1 sorts
 *         before @p s2, positive if after
 */
int strncmp(const char* s1, const char* s2, size_t n);

/**
 * @brief Copies a string, including its terminating NUL.
 *
 * @param dest Destination; must have room for strlen(@p src) + 1 bytes and must not
 *             overlap @p src
 * @param src NUL-terminated string to copy
 * @return @p dest
 */
char* strcpy(char* dest, const char* src);

/**
 * @brief Copies at most @p n bytes of a string, padding with NULs.
 *
 * Always writes exactly @p n bytes to @p dest: the bytes of @p src up to its NUL, then NULs
 * up to @p n.
 *
 * @warning If @p src is @p n bytes or longer, @p dest is **not** NUL-terminated.
 *
 * @param dest Destination, at least @p n bytes long; must not overlap @p src
 * @param src String to copy; it only needs to be NUL-terminated if shorter than @p n
 * @param n Number of bytes to write to @p dest
 * @return @p dest
 */
char* strncpy(char* dest, const char* src, size_t n);