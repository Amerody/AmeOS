// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file mailbox.h
 * @brief VideoCore mailbox: send a request buffer to the firmware and wait for the reply.
 * @ingroup mailbox
 *
 * Property requests are built in a buffer owned by the caller:
 *   1. mailbox_prop_begin() starts a request in the buffer,
 *   2. mailbox_prop_add() appends one tag and returns a pointer to its value words,
 *      through which the caller writes the tag's arguments,
 *   3. mailbox_prop_send() adds the header and end tag, sends the buffer on
 *      #MAILBOX_CH_PROP and waits for the reply,
 *   4. mailbox_prop_tag_ok() checks each tag's answer, which the caller then reads
 *      through the same value pointers.
 *
 * Buffer layout (32-bit words): total size in bytes, request/response code, the tags
 * (tag id, value buffer size in bytes, request/response code, value words), end tag 0.
 * See https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
 *
 * Context: no locking; each request uses its own buffer, and sending busy-waits for
 * the reply. Not for interrupt handlers.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

/// @brief Property tag: get the firmware revision (value: one u32 out)
#define MBOX_TAG_GET_FIRM_VER       0x00000001
/// @brief Property tag: get the board model (value: one u32 out)
#define MBOX_TAG_GET_BOARD_MODEL    0x00010001
/// @brief Property tag: get the board revision code (value: one u32 out)
#define MBOX_TAG_GET_BOARD_REVISION 0x00010002
/// @brief Property tag: get the board serial number (value: one u64 out)
#define MBOX_TAG_GET_BOARD_SERIAL   0x00010004
/// @brief Property tag: get the ARM's memory region (value: base address and size in
///        bytes out, both u32)
#define MBOX_TAG_GET_ARM_MEMORY     0x00010005
/// @brief Property tag: get the VideoCore's memory region (value: base address and size
///        in bytes out, both u32)
#define MBOX_TAG_GET_VC_MEMORY      0x00010006

/// @brief Property tag: allocate the framebuffer (value: alignment in, address and size out)
#define MBOX_TAG_FB_ALLOCATE        0x00040001
/// @brief Property tag: set the physical (display) width and height
#define MBOX_TAG_FB_SET_PHYS_WH     0x00048003
/// @brief Property tag: set the virtual (buffer) width and height
#define MBOX_TAG_FB_SET_VIRT_WH     0x00048004
/// @brief Property tag: set the depth, in bits per pixel
#define MBOX_TAG_FB_SET_DEPTH       0x00048005
/// @brief Property tag: get the pitch, in bytes per row
#define MBOX_TAG_FB_GET_PITCH       0x00040008
/// @brief Property tag: set the virtual offset (x, y) of the displayed area
#define MBOX_TAG_FB_SET_VIRT_OFF    0x00048009

// The Allocate Memory tag and following ones are interesting for a future graphics library

/**
 * @brief State of one property request being built in a caller-provided buffer.
 *
 * Filled in by mailbox_prop_begin(); the caller never touches the fields. One request
 * per struct; the struct can live on the stack next to the buffer.
 */
struct mailbox_prop
{
	uint32_t* buf;        ///< Start of the request buffer (16-byte aligned)
	size_t    cap_words;  ///< Capacity of @c buf, in 32-bit words
	size_t    next_idx;   ///< Index in @c buf where the next tag header goes
	bool      overflowed; ///< Set if a mailbox_prop_add() didn't fit; makes send fail
};

/**
 * @brief Starts a new property request in @p buf.
 *
 * Records the buffer and its capacity in @p p and positions the cursor after the
 * two header words (total size and request code), which are only written by
 * mailbox_prop_send(). Nothing is written to @p buf yet.
 *
 * The buffer must stay valid, and must not be reused for another request, until the
 * caller has finished reading the reply.
 *
 * @pre @p buf is 16-byte aligned (the mailbox message replaces the address's low 4
 *      bits with the channel number), e.g. `alignas(16) uint32_t buf[32];`
 * @pre @p words is at least 3 (two header words plus the end tag)
 *
 * @param p     Builder state to initialise
 * @param buf   Request buffer, owned by the caller
 * @param words Capacity of @p buf, in 32-bit words
 */
void mailbox_prop_begin(struct mailbox_prop* p, uint32_t* buf, size_t words);

/**
 * @brief Appends one tag to the request and returns where its value words are.
 *
 * Writes the three tag header words (tag id, @p value_size as given, request code 0),
 * zeroes the value words and moves the cursor past them. The space reserved for the
 * value is @p value_size rounded up to whole 32-bit words.
 *
 * The caller writes the tag's arguments (if any) through the returned pointer before
 * mailbox_prop_send(), and reads the firmware's answer through the same pointer
 * after it. The firmware writes its answer into the same words, so @p value_size must
 * be the larger of the request size and the response size of that tag.
 *
 * Space for the end tag is always kept free: the tag only fits if its header, its
 * value words and one more word fit in the buffer.
 *
 * @param p          Builder state, from mailbox_prop_begin()
 * @param tag        Tag id (MBOX_TAG_*)
 * @param value_size Size of the tag's value buffer, in bytes
 *
 * @return Pointer to the tag's first value word inside the request buffer, or NULL if
 *         the tag doesn't fit (the request is then marked as overflowed and
 *         mailbox_prop_send() fails)
 */
uint32_t* mailbox_prop_add(struct mailbox_prop* p, uint32_t tag, size_t value_size);

/**
 * @brief Finishes the request, sends it on the property channel and waits for the reply.
 *
 * Writes the end tag (0) at the cursor, the total size in bytes (header + tags + end
 * tag) in word 0 and the request code 0 in word 1, then calls mailbox_call() on
 * #MAILBOX_CH_PROP and checks the overall response code in word 1.
 *
 * A success only means the firmware parsed the buffer. Each tag can still have been
 * left unanswered: check it with mailbox_prop_tag_ok().
 *
 * Busy-waits with no timeout (see mailbox_call()). Not for interrupt handlers.
 *
 * @param p Builder state, from mailbox_prop_begin() and zero or more mailbox_prop_add()
 *
 * @retval 0 The firmware answered with the success code (0x80000000)
 * @retval -ERROR_NO_SPACE A mailbox_prop_add() didn't fit; nothing was sent
 * @retval -1 The firmware answered with an error code (0x80000001: error parsing the
 *         request)
 */
int mailbox_prop_send(struct mailbox_prop* p);

/**
 * @brief Checks that the firmware answered one tag completely.
 *
 * Reads the tag's request/response code (the word just before @p value): the tag was
 * answered if bit 31 is set, and the answer is complete if the response length in
 * bits 30:0 is no larger than the tag's value buffer size (the word before that). A
 * longer response means the firmware truncated it to fit.
 *
 * @pre mailbox_prop_send() returned 0 for the request @p value belongs to
 *
 * @param value Pointer returned by mailbox_prop_add(), or by one of the
 *              mailbox_request_*() helpers in mailbox_tags.h, for that tag; NULL is
 *              allowed
 *
 * @retval true The tag was answered and its value words are complete
 * @retval false @p value is NULL (the tag didn't fit), or the tag wasn't answered (e.g.
 *         unknown to this firmware) or was truncated
 */
bool mailbox_prop_tag_ok(const void* value);
