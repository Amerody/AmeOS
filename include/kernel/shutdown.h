// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#pragma once

enum shutdown_action
{
	SHUTDOWN_HALT,
	SHUTDOWN_POWEROFF,
	SHUTDOWN_REBOOT,
};

#ifdef __cplusplus
extern "C"
{
#endif

	[[noreturn]] void kernel_shutdown(enum shutdown_action action);

#ifdef __cplusplus
}
#endif