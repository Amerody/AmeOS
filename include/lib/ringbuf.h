#pragma once

#include <stddef.h>
#include <stdint.h>

struct ringbuf
{
	uint8_t* buf;
	size_t mask; // capacity - 1 (capacity must be a power of 2)
	size_t head; // written only by the producer (ISR)
	size_t tail; // written only by the consumer (console)
};

/**
 * @brief Initializes a ring buffer
 *
 * @param rb Pointer to the ring buffer instance
 * @param storage Array of storage
 * @param capacity Capacity of the buffer. Must be a power of 2
 */
void ringbuf_init(struct ringbuf* rb, uint8_t* storage, size_t capacity);

bool ringbuf_put(struct ringbuf* rb, uint8_t byte); // false if full

bool ringbuf_get(struct ringbuf* rb, uint8_t* out); // false if empty