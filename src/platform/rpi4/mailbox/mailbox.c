// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#include "mailbox.h"

#include <common/mmio.h>
#include <kernel/error.h>

#include <soc.h>

/**
 * @file mailbox.c
 * @brief VideoCore mailbox driver
 * @ingroup mailbox
 */

/// @brief All the relevant VideoCore Mailbox Registers
/// @note There is not a lot of publicly available documentation so this is only the required registers sourced from tutorials
/// @warning @p RESERVED_* are not to be used
struct VC_MBOX_Registers
{
	volatile uint32_t READ; ///< VideoCore Mailbox Read Register

	uint32_t RESERVED_04_0C[CALC_OFFSET(0x04, 0x0C)]; ///< 12 Bytes gap between READ and POLL

	volatile uint32_t POLL;   ///< VideoCore Mailbox Poll Register
	volatile uint32_t SENDER; ///< VideoCore Mailbox Sender Register
	volatile uint32_t STATUS; ///< VideoCore Mailbox Status Register
	volatile uint32_t CONFIG; ///< VideoCore Mailbox Config Register
	volatile uint32_t WRITE;  ///< VideoCore Mailbox Write Register
};

#define VC_MBOX_BASE             (PERIPHERAL_BASE + 0x0000B880)             ///< VideoCore Mailbox base address
#define VC_MBOX                  ((struct VC_MBOX_Registers*) VC_MBOX_BASE) ///< VideoCore Mailbox access

/// @brief Response code in the second word of the buffer when the firmware processed the request
#define MBOX_RESPONSE            0x80000000u
/// @brief STATUS bit: the write mailbox is full, so no message can be sent yet
#define MBOX_FULL                0x80000000u
/// @brief STATUS bit: the read mailbox is empty, so no reply has arrived yet
#define MBOX_EMPTY               0x40000000u

/// @brief Mask to check if the response code of a tag is ok
#define MBOX_RESPONSE_OK_MASK    0x80000000u
/// @brief Mask to check the respone value buffer size
#define MBOX_RESPONSE_VB_OK_MASK 0x7FFFFFFFu

/**
 * @brief Sends a buffer on a channel and waits for the firmware's reply.
 *
 * Busy-waits, with no timeout, until the mailbox can take the message, then until the
 * reply to this message arrives. Other messages read in the meantime are discarded.
 *
 * Only the overall response code is checked; the caller checks the values each tag returned.
 *
 * @param channel Mailbox channel; only the low 4 bits are used
 * @param buf Buffer to send
 *
 * @retval 0 The firmware answered with the success response code
 * @retval -1 The firmware answered with an error
 *
 * @todo Once the MMU maps RAM as cacheable, clean the buffer's cache lines
 *       (dc cvac) before the first dsb and invalidate them (dc ivac) after the
 *       reply, or map the buffer non-cacheable.
 */
static int mailbox_call(uint8_t channel, uint32_t* buf)
{
	// 28-bit address (MSB) and 4-bit value (LSB)
	uint32_t r = ((uint32_t) ((uint64_t) buf) & ~0xF) | (channel & 0xF);

	// Wait until we can write
	while (mmio_read(&VC_MBOX->STATUS) & MBOX_FULL)
		;

	/// @todo Create an arch module for memory synchronization
	__asm__ __volatile__("dsb sy" ::: "memory");

	// Write the address of our buffer to the mailbox with the channel appended
	mmio_write(&VC_MBOX->WRITE, r);

	while (1)
	{
		// Is there a reply?
		while (mmio_read(&VC_MBOX->STATUS) & MBOX_EMPTY)
			;

		// Is it a reply to our message?
		if (r == mmio_read(&VC_MBOX->READ))
		{
			__asm__ volatile("dsb sy" ::: "memory");
			return (buf[1] == MBOX_RESPONSE) ? 0 : -1; // Is it successful?
		}
	}
	return 0;
}

void mailbox_prop_begin(struct mailbox_prop* p, uint32_t* buf, size_t words)
{
	p->buf        = buf;
	p->cap_words  = words;
	p->next_idx   = 2; // buffer size (0) + buffer request/respone code (1)
	p->overflowed = false;
}

uint32_t* mailbox_prop_add(struct mailbox_prop* p, uint32_t tag, size_t value_size)
{
	// if the buffer would overflow
	size_t value_words = (value_size + 3) / 4;
	if (p->next_idx + 3 + value_words + 1 > p->cap_words)
	{
		p->overflowed = true;
		return NULL;
	}

	p->buf[p->next_idx++] = tag;         // tag
	p->buf[p->next_idx++] = value_size;  // value buffer size
	p->buf[p->next_idx++] = 0;           // request code
	size_t idx            = p->next_idx; // save the index of the first word in the value buffer to return

	for (size_t i = 0; i < value_words; i++)
		p->buf[idx + i] = 0;
	p->next_idx += value_words;

	return &p->buf[idx];
}

int mailbox_prop_send(struct mailbox_prop* p)
{
	if (p->overflowed)
		return -ERROR_NO_SPACE;

	p->buf[p->next_idx++] = 0;                              // end tag
	p->buf[0]             = p->next_idx * sizeof(uint32_t); // size in bytes
	p->buf[1]             = 0;                              // request code

	return mailbox_call(MAILBOX_CH_PROP, p->buf);
}

bool mailbox_prop_tag_ok(const uint32_t* value)
{
	if (!value)
		return false;

	uint32_t word                  = *(value - 1);
	uint32_t size                  = *(value - 2);
	uint32_t response_lenght_bytes = word & MBOX_RESPONSE_VB_OK_MASK;
	return ((word & MBOX_RESPONSE_OK_MASK) != 0) && (response_lenght_bytes <= size);
}
