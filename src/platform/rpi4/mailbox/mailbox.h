// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

/**
 * @file mailbox.h
 * @brief VideoCore mailbox: send a request buffer to the firmware and wait for the reply.
 * @ingroup mailbox
 *
 * Requests go through the single shared mailbox_buffer. The caller fills it with a
 * property request (total size in bytes, request code 0, the tags, then an end tag
 * of 0), calls mailbox_call(MAILBOX_CH_PROP) and reads the replies from the same
 * buffer.
 *
 * Context: one request at a time, with no locking, and mailbox_call() busy-waits for
 * the reply. Not for interrupt handlers.
 */
#pragma once

#include <stdint.h>

/**
 * @brief Request and response buffer shared by every mailbox_call(), 36 words.
 *
 * 16-byte aligned because the mailbox message carries the buffer address with its
 * low 4 bits replaced by the channel number.
 */
extern volatile uint32_t mailbox_buffer[36] __attribute__((aligned(16)));

/// @brief Mailbox channel for property-tag requests
#define MAILBOX_CH_PROP          8

/// @brief Property tag: allocate the framebuffer (value: alignment in, address and size out)
#define MBOX_TAG_FB_ALLOCATE     0x40001
/// @brief Property tag: set the physical (display) width and height
#define MBOX_TAG_FB_SET_PHYS_WH  0x48003
/// @brief Property tag: set the virtual (buffer) width and height
#define MBOX_TAG_FB_SET_VIRT_WH  0x48004
/// @brief Property tag: set the depth, in bits per pixel
#define MBOX_TAG_FB_SET_DEPTH    0x48005
/// @brief Property tag: get the pitch, in bytes per row
#define MBOX_TAG_FB_GET_PITCH    0x40008
/// @brief Property tag: set the virtual offset (x, y) of the displayed area
#define MBOX_TAG_FB_SET_VIRT_OFF 0x48009

/**
 * @brief Sends mailbox_buffer on a channel and waits for the firmware's reply.
 *
 * Busy-waits, with no timeout, until the mailbox can take the message, then until the
 * reply to this message arrives. Other messages read in the meantime are discarded.
 *
 * Only the overall response code (mailbox_buffer[1]) is checked; the caller checks
 * the values each tag returned.
 *
 * @param channel Mailbox channel; only the low 4 bits are used
 *
 * @retval 0 The firmware answered with the success response code
 * @retval -1 The firmware answered with an error
 *
 * @bug No barrier between filling mailbox_buffer (RAM) and writing the WRITE register
 *      (a different peripheral), and none between reading the matching reply and
 *      reading mailbox_buffer. Device-nGnRnE ordering only holds within one peripheral,
 *      so a @c dsb is needed on both sides (Arm ARM, Device memory ordering). Harmless
 *      in QEMU; once the MMU maps RAM as cacheable, the buffer also needs a clean before
 *      the request and an invalidate after the reply.
 */
int mailbox_call(uint8_t channel);
