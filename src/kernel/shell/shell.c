#include "kernel/shell.h"
#include "kernel/commands.h"
#include "kernel/console.h"
#include "kernel/kprintf.h"
#include "lib/string.h"

#include <stddef.h>
#include <stdint.h>

#define BUFFER_SIZE 256u
#define MAX_ARGS 16u

#define ESC 0x1Bu

static uint8_t s_line_buffer[BUFFER_SIZE];
static size_t s_cursor_pos = 0;
static bool s_last_byte_r = false; // was the last byte a '\r'?

typedef enum
{
	STATE_NORMAL,
	STATE_GOT_ESC,
	STATE_GOT_CSI
} esc_state_t;

static esc_state_t s_escape_sequence_state = STATE_NORMAL;

/// @brief Moore FSM to handle an escape sequence
static void process_esc_state(char c)
{
	if (s_escape_sequence_state == STATE_NORMAL)
		s_escape_sequence_state = STATE_GOT_ESC;
	else if (s_escape_sequence_state == STATE_GOT_ESC)
		if (c == '[' || c == 'O')
			s_escape_sequence_state = STATE_GOT_CSI;
		else
			s_escape_sequence_state = STATE_NORMAL;
	else if (s_escape_sequence_state == STATE_GOT_CSI && c < 0x40)
		s_escape_sequence_state = STATE_GOT_CSI; // the compiler optimizes this to a return instruction
	else
		s_escape_sequence_state = STATE_NORMAL;
}

static bool is_space(char c)
{
	return c == ' ' || c == '\t';
}

/**
 * @brief Tokenizes a line in place. Separators get overwritten with '\0',
 *        and argv[i] points into the line. "Double quotes" group words into one arg.
 *
 * @param[in,out] line     null-terminated line (gets modified)
 * @param[out]    argv     must have room for max_args + 1 entries (NULL terminated)
 * @param[in]     max_args extra tokens past this are dropped
 * @return number of arguments (argc)
 */
static size_t tokenize(char* line, char* argv[], size_t max_args)
{
	size_t argc = 0;
	char* p = line;
	while (*p != '\0')
	{
		while (is_space(*p))
			p++;

		if (*p == '\0' || argc == max_args)
			break;

		if (*p == '"')
		{
			argv[argc++] = ++p; // token starts after the quote
			while (*p != '\0' && *p != '"')
				p++;
		}
		else
		{
			argv[argc++] = p;
			while (*p != '\0' && !is_space(*p))
				p++;
		}

		if (*p != '\0')
			*p++ = '\0'; // end the token, step past the separator
	}

	argv[argc] = NULL;
	return argc;
}

static void parse_command()
{
	char* argv[MAX_ARGS + 1];
	// tokenize command
	size_t argc = tokenize((char*) s_line_buffer, argv, MAX_ARGS);

	if (argc == 0)
		return; // empty line

	dispatch_command(argc, argv);
	// the output will end on a '\n'
}

/// @brief prints the "$ " prompt. It isn't in the buffer so it cant be deleted
static void print_prompt()
{
	console_write("$ ", 2);
}

void shell_init()
{
	s_cursor_pos = 0;
	s_last_byte_r = false;
	s_escape_sequence_state = STATE_NORMAL;

	print_prompt();
}

void shell_feed(char c)
{
	// if in an escape sequence:  advance the ESC state machine, return
	if (c == ESC || s_escape_sequence_state != STATE_NORMAL)
	{
		process_esc_state(c);
		return;
	}

	if (c == '\n' && s_last_byte_r)
	{
		s_last_byte_r = false;
		return;
	}

	switch (c)
	{
	case '\r':
	case '\n':
		if (c == '\r')
			s_last_byte_r = true;
		console_putc('\n');
		s_line_buffer[s_cursor_pos] = '\0';
		s_cursor_pos = 0;
		parse_command();
		print_prompt();
		break;

	case 0x7F:
	case '\b':
		if (s_cursor_pos > 0)
		{
			s_cursor_pos--;
			console_write("\b \b", 3);
		}
		break;

	default:
		if (c >= 0x20 && c <= 0x7E)
		{
			// printable

			// if there's room:
			if (s_cursor_pos < BUFFER_SIZE - 1)
			{
				s_line_buffer[s_cursor_pos++] = c;
				console_putc(c);
			}
		}
		break;
	}
}