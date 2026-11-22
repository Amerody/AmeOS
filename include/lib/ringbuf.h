// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Amerody

#pragma once

/**
 * @file ringbuf.h
 * @brief Byte ring buffer with one producer and one consumer.
 * @ingroup lib
 *
 * Lock-free for exactly one producer (e.g. an interrupt handler) and exactly
 * one consumer (e.g. normal kernel code): the producer only writes @c head,
 * the consumer only writes @c tail, and each publishes its index with a
 * release store that the other side reads with an acquire load. Not safe with
 * several producers or several consumers.
 *
 * The indexes count up freely and are wrapped with @c mask, so the capacity
 * must be a power of 2 and all of it is usable.
 */

#include <stddef.h>
#include <stdint.h>

/// @brief Ring buffer state. Initialise with ringbuf_init(); don't touch the fields directly
struct ringbuf
{
	uint8_t* buf;  ///< Storage given to ringbuf_init()
	size_t   mask; ///< Capacity - 1 (the capacity is a power of 2)
	size_t   head; ///< Next slot to write. Written only by the producer
	size_t   tail; ///< Next slot to read. Written only by the consumer
};

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief Initializes a ring buffer
	 *
	 * Not safe while a producer or consumer is using @p rb: stop them first (for
	 * an interrupt producer, disable its interrupt).
	 *
	 * @param rb Pointer to the ring buffer instance
	 * @param storage Array of storage; must outlive @p rb
	 * @param capacity Capacity of the buffer in bytes. Must be a power of 2 (so at least 1);
	 *                 this is not checked yet
	 */
	void ringbuf_init(struct ringbuf* rb, uint8_t* storage, size_t capacity);

	/**
	 * @brief Adds one byte. Producer side only.
	 *
	 * Never blocks: when the buffer is full the byte is not stored. Safe to call
	 * from an interrupt handler.
	 *
	 * @param rb Ring buffer
	 * @param byte Byte to add
	 *
	 * @retval true The byte was stored
	 * @retval false The buffer is full; the byte was dropped
	 */
	bool ringbuf_put(struct ringbuf* rb, uint8_t byte);

	/**
	 * @brief Takes the oldest byte. Consumer side only.
	 *
	 * Never blocks.
	 *
	 * @param rb Ring buffer
	 * @param[out] out Where the byte is stored; untouched if the buffer is empty
	 *
	 * @retval true A byte was stored in @p out
	 * @retval false The buffer is empty
	 */
	bool ringbuf_get(struct ringbuf* rb, uint8_t* out);

#ifdef __cplusplus
}
#endif
