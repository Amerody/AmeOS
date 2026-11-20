#include "kernel/shell.h"
#include "kernel/console.h"
#include "kernel/kprintf.h"
#include "lib/string.h"

#include <stddef.h>
#include <stdint.h>

#define BUFFER_SIZE 256u

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

static void dispatch_command()
{
	// read the line to dispatch as a command

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
		dispatch_command();
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