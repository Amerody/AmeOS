// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file error.h
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
 * @c -ERROR_INVALID_ARG), and #SUCCESS (0) on success.
 */
enum
{
	SUCCESS = 0,   ///< No error
	ERROR_GENERIC, ///< Generic error

	ERROR_INVALID_ARG, ///< Invalid argument
	ERROR_BUSY,        ///< Busy: already initialised or in use
	ERROR_NO_DEVICE,   ///< No such device
	ERROR_NO_SPACE,    ///< No space left
	ERROR_NOT_FOUND,   ///< Something was not found
};
