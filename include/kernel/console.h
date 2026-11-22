// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file console.h
 * @brief Kernel console: a registry of output sinks that kprintf() writes to,
 *        which can also be read from.
 * @ingroup console
 *
 * The registry (src/kernel/lib/console.c) knows nothing about devices. It keeps
 * a small list of sinks and sends every byte to all of them. A sink may also
 * provide input (a @c getc), which console_getc() reads, a way to clear
 * its screen (a @c clear), which console_clear() calls, and a way to wait
 * until its output has left the device (a @c flush), which console_flush()
 * calls. Concrete sinks
 * (UART, framebuffer, ...) live in src/drivers/; kmain() initialises
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

		/**
		 * Clear the device's screen and move its cursor to the top-left corner.
		 * NULL if the device has nothing to clear. Same rules as @c putc.
		 */
		void (*clear)(void);

		/**
		 * Wait until every byte already given to @c putc has left the device (for a
		 * UART: its transmit FIFO has drained). NULL if the device has nothing to
		 * wait for. Same rules as @c putc; it may busy-wait.
		 */
		void (*flush)(void);
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
	 * @param con  Sink description; @c putc must not be NULL, @c getc, @c clear and
	 *             @c flush may be.
	 *
	 * @retval 0 The sink was registered
	 * @retval -ERROR_INVALID_ARG @p con or its @c putc is NULL
	 * @retval -ERROR_NO_SPACE CONSOLE_MAX_SINKS sinks are already registered (checked first, so
	 *                 this is returned even when @p con is invalid)
	 */
	int console_register(const struct console* con);

	/**
	 * @brief Send one byte to every registered sink.
	 *
	 * With no sinks registered the byte is silently discarded, so kprintf() is
	 * always safe to call, even before any sink exists.
	 *
	 * @param c  Byte to output, passed through untouched.
	 *
	 * @bug The file description says this is safe from interrupt handlers, but the
	 *      framebuffer sink (console_fb.c) does unprotected read-modify-writes of its
	 *      cursor and scrolls with a long memmove(). Output from an interrupt handler
	 *      that lands in the middle of normal-context output can scroll twice (losing a
	 *      line), overwrite characters or corrupt the scrolled image. The UART sink has
	 *      a similar race (see uart_mini_putc()). Latent: no handler prints today.
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

	/**
	 * @brief Clear every registered sink that has a @c clear.
	 *
	 * Sinks are cleared in registration order; those without a @c clear are
	 * skipped. With no sinks registered this does nothing. Used by the shell's
	 * @c clear command.
	 *
	 * Takes no locks, like console_putc(), but a sink's @c clear can take long
	 * (the framebuffer sink fills the whole screen).
	 */
	void console_clear();

	/**
	 * @brief Wait until every registered sink that has a @c flush has sent all its
	 *        output.
	 *
	 * Sinks are flushed in registration order; those without a @c flush are
	 * skipped. With no sinks registered this does nothing. Call it before anything
	 * that would cut the output short: kernel_shutdown() calls it before resetting
	 * the board, so its last message isn't lost.
	 *
	 * Takes no locks, like console_putc(), so it works with CPU IRQs masked. It
	 * busy-waits as long as the slowest sink needs (the UART sink until its
	 * transmit FIFO has drained).
	 */
	void console_flush();

#ifdef __cplusplus
}
#endif
