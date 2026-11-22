// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#include "kernel/shell.h"
#include "commands.h"
#include "kernel/console.h"

#include <stddef.h>
#include <stdint.h>

/**
 * @file shell.c
 * @brief Shell line editor: line buffer, escape-sequence filter, tokenizer and prompt
 *        (see shell.h)
 * @ingroup shell
 */

/// @brief Size of #s_line_buffer in bytes, including the terminating NUL
#define BUFFER_SIZE 256u
/// @brief Maximum number of tokens per line; tokens past this are dropped
#define MAX_ARGS 16u

/// @brief The escape byte that starts a terminal escape sequence
#define ESC 0x1Bu

/// @brief The line being typed; NUL-terminated only when the line ends
static uint8_t s_line_buffer[BUFFER_SIZE];
/// @brief Number of characters in #s_line_buffer, which is also where the next one goes
static size_t s_cursor_pos = 0;
/// @brief Was the last byte (escape sequences aside) a @c '\\r'? Lets "\r\n" end one line
static bool s_last_byte_r = false;

/// @brief States of the escape-sequence filter in process_esc_state()
typedef enum
{
	STATE_NORMAL,  ///< Not in an escape sequence
	STATE_GOT_ESC, ///< Got @c ESC, waiting for the byte that says what kind of sequence it is
	STATE_GOT_CSI  ///< Inside an @c "ESC [" or @c "ESC O" sequence, waiting for its final byte
} esc_state_t;

/// @brief Current state of the escape-sequence filter
static esc_state_t s_escape_sequence_state = STATE_NORMAL;

/**
 * @brief Moore FSM to handle an escape sequence
 *
 * Advances #s_escape_sequence_state by one byte. shell_feed() calls it for @c ESC and
 * for every byte while a sequence is in progress, and drops those bytes:
 * - #STATE_NORMAL: the byte is @c ESC, go to #STATE_GOT_ESC.
 * - #STATE_GOT_ESC: @c '[' or @c 'O' goes to #STATE_GOT_CSI; any other byte ends the
 *   sequence.
 * - #STATE_GOT_CSI: bytes below 0x40 (parameters) keep the state; any other byte is the
 *   final byte and ends the sequence.
 *
 * @param c Received byte
 */
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

/**
 * @brief Tells whether a byte separates tokens.
 *
 * @param c Byte to test
 *
 * @retval true @p c is a space or a tab
 * @retval false Any other byte
 */
static bool is_space(char c)
{
	return c == ' ' || c == '\t';
}

/**
 * @brief Tokenizes a line in place. Separators get overwritten with '\0',
 *        and argv[i] points into the line. "Double quotes" group words into one arg.
 *
 * The quotes are not part of the token. A quote with no closing quote runs to the end
 * of the line.
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

/**
 * @brief Tokenizes #s_line_buffer in place and runs it with shell_dispatch_command().
 *
 * Does nothing for a line with no tokens. The buffer is modified by tokenize(), so it
 * can't be used as the line afterwards.
 *
 * Precondition: #s_line_buffer is NUL-terminated.
 */
static void parse_command()
{
	char* argv[MAX_ARGS + 1];
	// tokenize command
	size_t argc = tokenize((char*) s_line_buffer, argv, MAX_ARGS);

	if (argc == 0)
		return; // empty line

	shell_dispatch_command(argc, argv);
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

	s_last_byte_r = false;

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