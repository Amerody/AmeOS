#pragma once

/**
 * @file shell.h
 * @brief Kernel shell: turns console input bytes into lines and runs them as commands.
 * @ingroup shell
 *
 * The shell does not read input itself. kmain() takes bytes from console_getc() and
 * passes each one to shell_feed(), which echoes it, edits the current line and, at the
 * end of a line, runs it through shell_dispatch_command() (see commands.h). Output goes through
 * the console, so it appears on every registered sink.
 *
 * @code
 * shell_init();                 // prints the first prompt
 * char c;
 * if (console_getc(&c))
 *     shell_feed(c);
 * @endcode
 *
 * Context: the line buffer and the escape-sequence state are file statics with no
 * locking, so call both functions from one context only (the kmain() loop), never from
 * an interrupt handler.
 */

/**
 * @brief Resets the line buffer and the escape-sequence state, then prints the
 *        @c "$ " prompt.
 *
 * Call once, after the console sinks are registered (it prints), before the first
 * shell_feed().
 */
void shell_init();

/**
 * @brief Feeds one received byte to the shell.
 *
 * - @c ESC starts an escape sequence (@c "ESC [" or @c "ESC O" followed by parameter
 *   bytes and a final byte in 0x40-0x7E). The whole sequence is swallowed, so arrow and
 *   function keys are ignored.
 * - @c '\\r' or @c '\\n' ends the line: echoes @c '\\n', runs the line with
 *   shell_dispatch_command() (empty lines are skipped) and prints a new prompt. A @c '\\n'
 *   right after a @c '\\r' is ignored, so a @c "\r\n" pair counts as one line end.
 * - @c '\\b' or DEL (0x7F) removes the last character of the line and erases it on
 *   screen with @c "\b \b". Does nothing on an empty line.
 * - Printable ASCII (0x20-0x7E) is appended to the line and echoed, as long as the line
 *   has fewer than 255 characters; after that, characters are dropped without notice.
 * - Every other byte is ignored.
 *
 * Only the end of the line can be edited: there is no cursor movement inside the line.
 *
 * Runs the command synchronously, so the call takes as long as the command does.
 * Not reentrant and not for interrupt handlers (see the file description).
 *
 * @pre shell_init() has been called.
 *
 * @param c Received byte
 *
 */
void shell_feed(char c);