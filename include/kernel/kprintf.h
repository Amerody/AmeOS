#pragma once

/**
 * @addtogroup print
 * @{
 */

/**
 * @file kprintf.h
 *
 * @brief Kernel printf
 */

#include <stdarg.h>

/**
 * @brief Prints a formatted string to the console
 */
__attribute__((format(printf, 1, 2))) void kprintf(const char* fmt, ...);

void kvprintf(const char* fmt, va_list ap);

/** @} */