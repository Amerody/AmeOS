/**
 * @file errno.h
 * @brief Error codes definition
 *
 * Used by most of the implementations.
 */

#pragma once

enum
{
	SUCCESS = 0,

	EINVAL, ///< Invalid
	EBUSY,  ///< Busy
	ENODEV, ///< No device
	ENOSPC, ///< No space
};
