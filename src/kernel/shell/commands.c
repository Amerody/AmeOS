// built-in commands
#include "commands.h"
#include <stddef.h>

#include "hal/hal_timer.h"
#include "kernel/console.h"
#include "kernel/kprintf.h"
#include <lib/memory.h>
#include <lib/string.h>

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

// reboot
static int cmd_reboot(size_t argc, char* argv[]);

/// @brief One built-in command: its name, its help text and the function that runs it
typedef struct
{
	const char* name;                     ///< Name typed to run the command (argv[0])
	const char* help;                     ///< Help message without '\n', printed by cmd_help()
	int (*fn)(size_t argc, char* argv[]); ///< Runs the command; 0 on success, -1 on usage error
} command_t;

/// @brief Built-in commands, searched in order by shell_dispatch_command() and cmd_help()
static const command_t s_commands[] = {
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
	  "Prints the value at a memory address. Use: peek <address>",
	  cmd_peek },
	{ "reboot",
	  "Reboots the system",
	  cmd_reboot },
};

/// @brief Number of entries in #s_commands
#define COMMANDS_SIZE (sizeof(s_commands) / sizeof(s_commands[0]))

void shell_dispatch_command(size_t argc, char* argv[])
{
	for (size_t i = 0; i < COMMANDS_SIZE; i++)
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
		for (size_t i = 0; i < COMMANDS_SIZE; i++)
		{
			kprintf("%s\n", s_commands[i].name);
		}
		return 0;
	}
	else
	{
		for (size_t i = 0; i < COMMANDS_SIZE; i++)
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

	uint64_t hours = now_s / 3600;
	uint64_t minutes = (now_s % 3600) / 60;
	uint64_t seconds = now_s % 60;
	kprintf("Uptime: %lu hours %lu minutes %lu seconds\n", hours, minutes, seconds);

	return 0;
}

/**
 * @brief @c peek: not implemented yet; prints @c "TODO".
 *
 * @param argc Number of tokens; unused
 * @param argv Tokens; unused
 *
 * @retval 0 Always
 *
 * @todo Print the value at the address given in @p argv[1].
 */
static int cmd_peek(size_t argc, char* argv[])
{
	if (argc != 2)
	{
		kprintf("Use: peek <address>\n");
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
 * @brief @c reboot: not implemented yet; prints @c "TODO".
 *
 * @param argc Number of tokens; unused
 * @param argv Tokens; unused
 *
 * @retval 0 Always
 *
 * @todo Reboot the system.
 */
static int cmd_reboot(size_t argc, char* argv[])
{
	(void) argc;
	(void) argv;
	kprintf("TODO\n");
	return 0;
}