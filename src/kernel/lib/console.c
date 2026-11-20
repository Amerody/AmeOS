#include "kernel/console.h"
#include "kernel/errno.h"

#include <drivers/console/console_uart.h>

#include <stddef.h>

/**
 * @file console.c
 * @brief Console sink registry: fans every output byte out to all registered sinks
 *        (see console.h)
 * @ingroup console
 */

/// @brief Registered sinks, in registration order. Only the first #s_sinks_registered are valid
static struct console s_consoles[CONSOLE_MAX_SINKS];

/// @brief Number of valid entries in #s_consoles. Reset to 0 by console_init()
static size_t s_sinks_registered;

void console_init()
{
	s_sinks_registered = 0;
}

int console_register(const struct console* con)
{
	if (s_sinks_registered >= CONSOLE_MAX_SINKS)
		return -ENOSPC;

	if (!con || !con->putc)
		return -EINVAL;

	s_consoles[s_sinks_registered] = *con;

	s_sinks_registered++;

	return SUCCESS;
}

void console_putc(char c)
{
	for (size_t i = 0; i < s_sinks_registered; i++)
	{
		s_consoles[i].putc(c);
	}
}

void console_write(const char* buf, size_t len)
{
	for (size_t i = 0; i < len; i++)
	{
		console_putc(buf[i]);
	}
}

bool console_getc(char* c)
{
	for (size_t i = 0; i < s_sinks_registered; i++)
	{
		if (s_consoles[i].getc != NULL && s_consoles[i].getc(c))
			return true;
	}

	return false;
}

void console_clear()
{
	for (size_t i = 0; i < s_sinks_registered; i++)
	{
		if (s_consoles[i].clear != NULL)
			s_consoles[i].clear();
	}
}