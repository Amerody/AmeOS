// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file commands.h
 * @brief Built-in shell commands: looks up a tokenized command line and runs it.
 * @ingroup shell
 *
 * Implemented by commands.c, which holds the command table. Internal to the shell: only
 * shell.c includes it, which is why it lives in src/kernel/shell/ and not in include/.
 */
#pragma once

#include <stddef.h>

/**
 *
 * @brief Runs the built-in command named by @p argv[0].
 *
 * Compares @p argv[0] with each command name in the table (case-sensitive) and calls the
 * first match with @p argc and @p argv. If no command matches, prints
 * @c "<name>: command not found". The command's return value is discarded.
 *
 * Same context as shell_feed(): one context only, not for interrupt handlers. Runs the
 * command synchronously.
 *
 * @param argc Number of entries in @p argv; at least 1
 * @param argv Tokens of the command line, each NUL-terminated; @p argv[0] is the command
 *             name and must not be NULL. The command may read but should not keep them:
 *             they point into the shell's line buffer
 */
void shell_dispatch_command(size_t argc, char* argv[]);
