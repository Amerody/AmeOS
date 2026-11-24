// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody
#include "commands.h"

#include "kernel/console.h"
#include "kernel/error.h"
#include "kernel/kprintf.h"
#include "kernel/shutdown.h"

#include <hal/hal_timer.h>
#include <hal/platform.h>
#include <lib/memory.h>
#include <lib/string.h>

#include <stddef.h>

/**
 * @file commands.c
 * @brief Built-in shell commands: the command table, shell_dispatch_command() and the command
 *        implementations (see commands.h)
 * @ingroup shell
 *
 * Every command has the same signature: it gets the tokenized line (@p argv[0] is the
 * command name) and returns 0 on success or -1 on an error, after printing a message
 * (the usage, for a usage error). shell_dispatch_command() discards the result.
 */

// help <command>
static int cmd_help(size_t argc, char* argv[]);

// echo <args>
static int cmd_echo(size_t argc, char* argv[]);

// version
static int cmd_version(size_t argc, char* argv[]);

// clear
static int cmd_clear(size_t argc, char* argv[]);

// uptime
static int cmd_uptime(size_t argc, char* argv[]);

// peek
static int cmd_peek(size_t argc, char* argv[]);

static int cmd_mem(size_t argc, char* argv[]);

// reboot
static int cmd_reboot(size_t argc, char* argv[]);

// shutdown
static int cmd_shutdown(size_t argc, char* argv[]);

/// @brief One built-in command: its name, its help text and the function that runs it
struct command
{
	const char* name;                     ///< Name typed to run the command (argv[0])
	const char* help;                     ///< Help message without '\n', printed by cmd_help()
	int (*fn)(size_t argc, char* argv[]); ///< Runs the command; 0 on success, -1 on an error
};

/// @brief Built-in commands, searched in order by shell_dispatch_command() and cmd_help()
static const struct command s_commands[] = {
	{ "help",
	  "Prints the help for a command. Use: help <command>",
	  cmd_help },
	{ "echo",
	  "Prints to the console. Use: echo <args>",
	  cmd_echo },
	{ "version",
	  "Prints the version of the OS. Use: version",
	  cmd_version },
	{ "clear",
	  "Clears the console. Use: clear",
	  cmd_clear },
	{ "uptime",
	  "Prints the uptime of the OS. Use: uptime",
	  cmd_uptime },
	{ "peek",
	  "Prints the value at a memory address. Use: peek <hex_address>",
	  cmd_peek },
	{ "mem",
	  "Prints the memory details. Use: mem",
	  cmd_mem },

	{ "reboot",
	  "Reboots the system. Use: reboot",
	  cmd_reboot },
	{ "shutdown",
	  "Shuts down the system. Use: shutdown",
	  cmd_shutdown },
};

/// @brief Number of entries in #s_commands
#define COMMANDS_COUNT (sizeof(s_commands) / sizeof(s_commands[0]))

void shell_dispatch_command(size_t argc, char* argv[])
{
	for (size_t i = 0; i < COMMANDS_COUNT; i++)
	{
		if (strcmp(argv[0], s_commands[i].name) == 0)
		{
			s_commands[i].fn(argc, argv);
			return;
		}
	}
	kprintf("%s: command not found\n", argv[0]);
}

// command function implementations

/**
 * @brief @c help: lists the command names, or prints the help text of one command.
 *
 * With no argument, prints every command name, one per line. With one argument, prints
 * the help text of the command with that name, or @c "help: <name>: command not found"
 * if there is none.
 *
 * @param argc Number of tokens; 1 or 2 (more is a usage error)
 * @param argv Tokens; @p argv[1], if present, is the command to describe
 *
 * @retval 0 The names or the help text were printed
 * @retval -1 More than one argument (the usage was printed), or no command matched
 */
static int cmd_help(size_t argc, char* argv[])
{
	if (argc > 2)
	{
		kprintf("Use: help <command>\n");
		return -1;
	}

	if (argc == 1)
	{
		for (size_t i = 0; i < COMMANDS_COUNT; i++)
		{
			kprintf("%s\n", s_commands[i].name);
		}
		return 0;
	}
	else
	{
		for (size_t i = 0; i < COMMANDS_COUNT; i++)
		{
			if (strcmp(argv[1], s_commands[i].name) == 0)
			{
				kprintf("%s\n", s_commands[i].help);
				return 0;
			}
		}
	}

	kprintf("help: %s: command not found\n", argv[1]);
	return -1;
}

/**
 * @brief @c echo: prints its arguments separated by single spaces, then a newline.
 *
 * Quoted arguments are printed without their quotes, since the tokenizer removed them.
 *
 * @param argc Number of tokens
 * @param argv Tokens; @p argv[1] to @p argv[argc - 1] are printed
 *
 * @retval 0 Always
 */
static int cmd_echo(size_t argc, char* argv[])
{
	for (size_t i = 1; i < argc; i++)
	{
		console_write(argv[i], strlen(argv[i]));
		if (i + 1 < argc)
			console_putc(' ');
	}
	console_putc('\n');
	return 0;
}

/**
 * @brief @c version: prints the kernel version.
 *
 * The version comes from the KERNEL_VERSION_MAJOR, KERNEL_VERSION_MINOR and
 * KERNEL_VERSION_PATCH compile definitions set in the top-level CMakeLists.txt.
 *
 * @param argc Number of tokens; must be 1
 * @param argv Tokens; unused
 *
 * @retval 0 The version was printed
 * @retval -1 Arguments were given; the usage was printed
 */
static int cmd_version(size_t argc, char* argv[])
{
	(void) argv;
	if (argc != 1)
	{
		kprintf("Use: version\n");
		return -1;
	}

	kprintf("AmeOS version: %d.%d.%d\n", KERNEL_VERSION_MAJOR, KERNEL_VERSION_MINOR, KERNEL_VERSION_PATCH);
	return 0;
}

/**
 * @brief @c clear: clears every console sink with console_clear().
 *
 * @param argc Number of tokens; must be 1
 * @param argv Tokens; unused
 *
 * @retval 0 The console was cleared
 * @retval -1 Arguments were given; the usage was printed
 */
static int cmd_clear(size_t argc, char* argv[])
{
	(void) argv;
	if (argc != 1)
	{
		kprintf("Use: clear\n");
		return -1;
	}

	console_clear();
	return 0;
}

/**
 * @brief @c uptime: prints the time since boot as hours, minutes and seconds.
 *
 * The time comes from hal_timer_now_sec(), so it counts from hal_timer_init() and is
 * rounded down to whole seconds. Hours keep counting past 24; there is no day field.
 *
 * @param argc Number of tokens; must be 1
 * @param argv Tokens; unused
 *
 * @retval 0 The uptime was printed
 * @retval -1 Arguments were given; the usage was printed
 */
static int cmd_uptime(size_t argc, char* argv[])
{
	(void) argv;
	if (argc != 1)
	{
		kprintf("Use: uptime\n");
		return -1;
	}

	uint64_t now_s = hal_timer_now_sec();

	uint64_t hours   = now_s / 3600;
	uint64_t minutes = (now_s % 3600) / 60;
	uint64_t seconds = now_s % 60;
	kprintf("Uptime: %lu hours %lu minutes %lu seconds\n", hours, minutes, seconds);

	return 0;
}

/**
 * @brief @c peek: prints the 32-bit word at a memory address.
 *
 * @p argv[1] is parsed with strtox(), so it is hexadecimal with an optional @c 0x prefix,
 * and must be 4-byte aligned. The word is read once through a #paddr_t, then the address
 * and the value are printed as lowercase hex with no prefix: @c "<address>: <value>".
 *
 * The address is used directly as a pointer, which is the physical address only while the
 * MMU is off. Nothing checks that there is memory or a device at the address, and reading
 * a register whose read has side effects triggers them. Addresses above 0xFFFFFFFF can't
 * be given, since strtox() parses 32 bits.
 *
 * @param argc Number of tokens; must be 2
 * @param argv Tokens; @p argv[1] is the address
 *
 * @retval 0 The word was printed
 * @retval -1 Wrong number of arguments (the usage was printed), or the address is not
 *            valid hex or not 4-byte aligned (a message was printed)
 */
static int cmd_peek(size_t argc, char* argv[])
{
	if (argc != 2)
	{
		kprintf("Use: peek <hex_address>\n");
		return -1;
	}

	uint32_t value;
	if (strtox(argv[1], &value) != 0)
	{
		kprintf("Invalid address\n");

		return -1;
	}

	if (value % 4u != 0)
	{
		kprintf("Address must be 4-byte aligned\n");
		return -1;
	}

	paddr_t ptr = (paddr_t) (uintptr_t) value;
	kprintf("%x: %x\n", value, *ptr);
	return 0;
}

/**
 * @brief Shell command @c mem: prints the RAM map from platform_mem_regions().
 *
 * One line per region, @c "@<base> size <size>" in lowercase hex. These are the regions
 * the platform hands to core: the kernel image is still inside them, and they say nothing
 * about which pages are in use. At most 16 regions are printed, even if the platform
 * reports more (the count line shows the total).
 *
 * @param argc Number of tokens; must be 1
 * @param argv Tokens; unused
 *
 * @retval 0 The map (or "RAM regions not mapped" if it's empty) was printed
 * @retval -1 Extra arguments were given (the usage was printed)
 */
static int cmd_mem(size_t argc, char* argv[])
{
	(void) argv;
	if (argc > 1)
	{
		kprintf("Use: mem\n");
		return -1;
	}

	struct mem_region regions[16];
	size_t            count = platform_mem_regions(regions, 16);
	if (count != 0)
	{
		kprintf("RAM map: %lu region(s)\n", count);
		for (size_t i = 0; i < count && i < 16; i++)
			kprintf("  @%lx size %lx\n", regions[i].base, regions[i].size);
	}
	else
	{
		kprintf("RAM regions not mapped\n");
	}

	return 0;
}

/**
 * @brief @c reboot: reboots the system with kernel_shutdown(#SHUTDOWN_REBOOT).
 *
 * On success it doesn't return: the board resets and the kernel boots again.
 *
 * @param argc Number of tokens; must be 1
 * @param argv Tokens; unused
 *
 * @retval -1 Arguments were given; the usage was printed
 */
static int cmd_reboot(size_t argc, char* argv[])
{
	(void) argv;
	if (argc != 1)
	{
		kprintf("Use: reboot\n");
		/// @todo add parameters
		return -1;
	}

	kernel_shutdown(SHUTDOWN_REBOOT);
	return 0;
}

/**
 * @brief @c shutdown: powers the system off with kernel_shutdown(#SHUTDOWN_POWEROFF).
 *
 * On success it doesn't return. On QEMU, the emulator exits.
 *
 * @param argc Number of tokens; must be 1
 * @param argv Tokens; unused
 *
 * @retval -1 Arguments were given; the usage was printed
 */
static int cmd_shutdown(size_t argc, char* argv[])
{
	(void) argv;
	if (argc != 1)
	{
		kprintf("Use: shutdown\n");
		/// @todo add parameters
		return -1;
	}

	kernel_shutdown(SHUTDOWN_POWEROFF);
	return 0;
}
