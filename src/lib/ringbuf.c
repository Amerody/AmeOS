#include "lib/ringbuf.h"

/**
 * @file ringbuf.c
 * @brief Single-producer / single-consumer byte ring buffer (see ringbuf.h)
 */

void ringbuf_init(struct ringbuf* rb, uint8_t* storage, size_t capacity)
{
	// TODO: assert capacity is a power of 2
	rb->buf = storage;
	rb->mask = capacity - 1;
	rb->head = 0;
	rb->tail = 0;
}

bool ringbuf_put(struct ringbuf* rb, uint8_t byte)
{
	size_t head = rb->head;
	size_t tail = __atomic_load_n(&rb->tail, __ATOMIC_ACQUIRE);
	if (head - tail > rb->mask)
		return false; // full
	rb->buf[head & rb->mask] = byte;
	__atomic_store_n(&rb->head, head + 1, __ATOMIC_RELEASE);
	return true;
}

bool ringbuf_get(struct ringbuf* rb, uint8_t* out)
{
	size_t tail = rb->tail;
	size_t head = __atomic_load_n(&rb->head, __ATOMIC_ACQUIRE);
	if (head == tail)
		return false; // empty
	*out = rb->buf[tail & rb->mask];
	__atomic_store_n(&rb->tail, tail + 1, __ATOMIC_RELEASE);
	return true;
}
