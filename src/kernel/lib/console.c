#include "kernel/console.h"

#include "drivers/console/console_uart.h"

#include "kernel/errno.h"

#include <stddef.h>

static struct console s_consoles[CONSOLE_MAX_SINKS];

static size_t s_sinks_registered;

/**
 * @brief Reset the registry to empty (no sinks).
 *
 * Call once, early in kmain(), before registering any sink. Calling it again
 * later forgets every registered sink.
 */
void console_init()
{
	s_sinks_registered = 0;
}

/**
 * @brief Add a sink; it receives all output from now on.
 *
 * Normally called by a driver's init function (for example
 * console_uart_init()), not by kmain() directly. The struct is copied, so it
 * may live on the caller's stack. Output written before registration is not
 * replayed.
 *
 * @param con  Sink description; @c putc must not be NULL.
 *
 * @return Status of the operation
 * @retval 0 The operation was completed successfully
 * @retval -EINVAL @p con or @p con->putc is NULL
 * @retval -ENOSPC There is no space left to register sinks
 */
int console_register(const struct console* con)
{
	if (s_sinks_registered >= CONSOLE_MAX_SINKS)
		return -ENOSPC;

	if (!con || !con->putc)
		return -EINVAL;

	s_consoles[s_sinks_registered].name = con->name;
	s_consoles[s_sinks_registered].putc = con->putc;

	s_sinks_registered++;

	return SUCCESS;
}

/**
 * @brief Send one byte to every registered sink.
 *
 * With no sinks registered the byte is silently discarded, so kprintf() is
 * always safe to call, even before any sink exists.
 *
 * @param c  Byte to output, passed through untouched.
 */
void console_putc(char c)
{
	for (size_t i = 0; i < s_sinks_registered; i++)
	{
		s_consoles[i].putc(c);
	}
}

/**
 * @brief Send a buffer to every registered sink, byte by byte.
 *
 * This is what kprintf() uses.
 *
 * @param buf  Bytes to output; not required to be NUL-terminated.
 * @param len  Number of bytes.
 */
void console_write(const char* buf, size_t len)
{
	for (size_t i = 0; i < len; i++)
	{
		console_putc(buf[i]);
	}
}