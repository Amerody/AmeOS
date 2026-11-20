/**
 * @file errno.h
 * @brief Error codes definition
 * @ingroup kernel
 *
 * Used by most of the implementations.
 */

#pragma once

/**
 * @brief Kernel error codes.
 *
 * The values are positive; functions that fail return them negated (for example
 * @c -EINVAL), and #SUCCESS (0) on success.
 */
enum
{
	SUCCESS = 0, ///< No error

	EINVAL, ///< Invalid argument
	EBUSY,  ///< Busy: already initialised or in use
	ENODEV, ///< No such device
	ENOSPC, ///< No space left
};
