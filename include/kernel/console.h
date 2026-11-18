/**
 * @file console.h
 * @brief Kernel console: a registry of output sinks that kprintf() writes to,
 *        which can also be read from.
 *
 * The registry (src/kernel/lib/console.c) knows nothing about devices. It keeps
 * a small list of sinks and sends every byte to all of them. A sink may also
 * provide input (a @c getc), which console_getc() reads. Concrete sinks
 * (UART, framebuffer, ...) live in src/drivers/console/; kmain() initialises
 * the registry and then asks each driver it wants to register its sink:
 *
 * @code
 * console_init();
 * console_uart_init(1);           // driver registers its sink (UART1)
 * console_fb_init();
 * kprintf("hello\n");             // kprintf() ends up in console_write()
 * @endcode
 *
 * Output is raw bytes. The registry does no newline translation: each sink
 * handles that itself, because it depends on the device (a serial terminal
 * wants "\r\n", a framebuffer console treats '\n' natively).
 *
 * Context: console_putc() and console_write() take no locks and allocate
 * nothing, so they are safe from interrupt handlers, exception handlers and
 * the panic path. There is no internal locking: callers that must keep lines
 * from interleaving (once SMP exists) serialise above this layer.
 */
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** @brief Maximum number of sinks that can be registered at once. */
#define CONSOLE_MAX_SINKS 4

	/** @brief An output sink, optionally also an input source. */
	struct console
	{
		/** Short name for diagnostics, e.g. "uart". Must stay valid (use a string
		 *  literal). */
		const char* name;

		/**
		 * Send one byte to the device. Must work with CPU IRQs masked and without
		 * kernel services (see the context note above). Must not call back into
		 * the console.
		 */
		void (*putc)(char c);

		/**
		 * Take one received byte from the device, if there is one. Must never
		 * block: return false when nothing is available. NULL for output-only
		 * devices (e.g. the framebuffer). Must not call back into the console.
		 * Returns true if a byte was stored in @p c.
		 */
		bool (*getc)(char* c);
	};

	/**
	 * @brief Reset the registry to empty (no sinks).
	 *
	 * Call once, early in kmain(), before registering any sink. Calling it again
	 * later forgets every registered sink.
	 */
	void console_init();

	/**
	 * @brief Add a sink; it receives all output from now on.
	 *
	 * Normally called by a driver's init function (for example
	 * console_uart_init()), not by kmain() directly. The struct is copied, so it
	 * may live on the caller's stack. Output written before registration is not
	 * replayed.
	 *
	 * @param con  Sink description; @c putc must not be NULL, @c getc may be.
	 * @return 0 on success, -EINVAL if @p con or its @c putc is NULL, -ENOSPC if
	 *         CONSOLE_MAX_SINKS sinks are already registered.
	 */
	int console_register(const struct console* con);

	/**
	 * @brief Send one byte to every registered sink.
	 *
	 * With no sinks registered the byte is silently discarded, so kprintf() is
	 * always safe to call, even before any sink exists.
	 *
	 * @param c  Byte to output, passed through untouched.
	 */
	void console_putc(char c);

	/**
	 * @brief Send a buffer to every registered sink, byte by byte.
	 *
	 * This is what kprintf() uses.
	 *
	 * @param buf  Bytes to output; not required to be NUL-terminated.
	 * @param len  Number of bytes.
	 */
	void console_write(const char* buf, size_t len);

	/**
	 * @brief Take one received byte from the first sink that has one. Never
	 *        blocks.
	 *
	 * Sinks are asked in registration order; those without a @c getc are
	 * skipped. Waiting for input means calling this in a loop.
	 *
	 * Unlike console_putc(), not for interrupt handlers: the drivers' input
	 * buffers have a single consumer, so read from one context only.
	 *
	 * @param[out] c  Where the byte is stored; untouched if there is none.
	 *
	 * @retval true A byte was stored in @p c
	 * @retval false No sink had a byte
	 */
	bool console_getc(char* c);

#ifdef __cplusplus
}
#endif