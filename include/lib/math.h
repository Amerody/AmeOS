// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file math.h
 * @brief Small integer helpers: minimum and maximum for @c int and @c size_t.
 * @ingroup lib
 *
 * One @c static @c inline function per type. Each argument is evaluated exactly once.
 *
 * Context: pure functions, usable from any context.
 */
#pragma once

#include <stddef.h>

/**
 * @brief Smaller of two @c int values.
 *
 * @param a First value
 * @param b Second value
 * @return @p a if it is less than @p b, otherwise @p b
 */
static inline int min_int(int a, int b)
{
	return a < b ? a : b;
}

/**
 * @brief Smaller of two @c size_t values.
 *
 * @param a First value
 * @param b Second value
 * @return @p a if it is less than @p b, otherwise @p b
 */
static inline size_t min_size(size_t a, size_t b)
{
	return a < b ? a : b;
}

/**
 * @brief Larger of two @c int values.
 *
 * @param a First value
 * @param b Second value
 * @return @p a if it is greater than @p b, otherwise @p b
 */
static inline int max_int(int a, int b)
{
	return a > b ? a : b;
}

/**
 * @brief Larger of two @c size_t values.
 *
 * @param a First value
 * @param b Second value
 * @return @p a if it is greater than @p b, otherwise @p b
 */
static inline size_t max_size(size_t a, size_t b)
{
	return a > b ? a : b;
}
