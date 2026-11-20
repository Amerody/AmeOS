// built-in commands
#include "kernel/commands.h"
#include <stddef.h>

#include "kernel/console.h"
#include "kernel/kprintf.h"
#include <lib/string.h>

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

typedef struct
{
	const char* name;
	const char* help; // help message without '\n'
	int (*fn)(size_t argc, char* argv[]);
} command_t;

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

#define COMMANDS_SIZE (sizeof(s_commands) / sizeof(s_commands[0]))

void dispatch_command(size_t argc, char* argv[])
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
			}
		}
	}

	return 0;
}

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

static int cmd_uptime(size_t argc, char* argv[])
{
	(void) argc;
	(void) argv;
	kprintf("TODO\n");
	return 0;
}

static int cmd_peek(size_t argc, char* argv[])
{
	(void) argc;
	(void) argv;
	kprintf("TODO\n");
	return 0;
}

static int cmd_reboot(size_t argc, char* argv[])
{
	(void) argc;
	(void) argv;
	kprintf("TODO\n");
	return 0;
}