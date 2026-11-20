/**
 * @file memory.h
 * @brief Freestanding memory functions: memcpy(), memset(), memmove().
 * @ingroup lib
 *
 * The kernel is built without libc, so it provides its own versions of these. They keep
 * the libc names and behaviour. They work one byte at a time, with no alignment or word-size optimisation.
 *
 * Context: they touch no global state and never block, so they can be called from any
 * context, including interrupt handlers.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

/// @brief Type for a physical address
typedef volatile uint32_t* paddr_t;
/// @brief Type for a virtual address
typedef volatile uint32_t* vaddr_t;

/**
 * @brief Copies @p n bytes from @p src to @p dest.
 *
 * The two regions must not overlap; use memmove() when they might.
 *
 * @param dest Destination, at least @p n bytes long
 * @param src Source, at least @p n bytes long
 * @param n Number of bytes to copy
 * @return @p dest
 */
void* memcpy(void* dest, const void* src, size_t n);

/**
 * @brief Fills @p n bytes at @p s with the byte value @p c.
 *
 * @param s Start of the region to fill, at least @p n bytes long
 * @param c Value to store
 * @param n Number of bytes to fill
 * @return @p s
 */
void* memset(void* s, uint8_t c, size_t n);

/**
 * @brief Copies @p n bytes from @p src to @p dest; the regions may overlap.
 *
 * Copies forward when @p dest is below @p src and backward when it is above, so every
 * source byte is read before it is overwritten. Nothing is copied when @p dest equals
 * @p src.
 *
 * @param dest Destination, at least @p n bytes long
 * @param src Source, at least @p n bytes long
 * @param n Number of bytes to copy
 * @return @p dest
 */
void* memmove(void* dest, const void* src, size_t n);