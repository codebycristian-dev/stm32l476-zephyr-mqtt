#include "byte_ring.h"

#include <limits.h>

#define BYTE_RING_MASK (BYTE_RING_CAPACITY - 1U)

_Static_assert((BYTE_RING_CAPACITY & BYTE_RING_MASK) == 0U,
	       "BYTE_RING_CAPACITY must be a power of two");

void byte_ring_init(struct byte_ring *ring)
{
	ring->head = 0U;
	ring->tail = 0U;
}

bool byte_ring_push(struct byte_ring *ring, uint8_t byte)
{
	uint32_t head = __atomic_load_n(&ring->head, __ATOMIC_RELAXED);
	uint32_t tail = __atomic_load_n(&ring->tail, __ATOMIC_ACQUIRE);

	if ((uint32_t)(head - tail) == BYTE_RING_CAPACITY) {
		return false;
	}

	ring->data[head & BYTE_RING_MASK] = byte;
	__atomic_store_n(&ring->head, head + 1U, __ATOMIC_RELEASE);
	return true;
}

bool byte_ring_pop(struct byte_ring *ring, uint8_t *byte)
{
	uint32_t tail;
	uint32_t head;

	if (byte == NULL) {
		return false;
	}

	tail = __atomic_load_n(&ring->tail, __ATOMIC_RELAXED);
	head = __atomic_load_n(&ring->head, __ATOMIC_ACQUIRE);
	if (tail == head) {
		return false;
	}

	*byte = ring->data[tail & BYTE_RING_MASK];
	__atomic_store_n(&ring->tail, tail + 1U, __ATOMIC_RELEASE);
	return true;
}

size_t byte_ring_count(const struct byte_ring *ring)
{
	uint32_t head = __atomic_load_n(&ring->head, __ATOMIC_ACQUIRE);
	uint32_t tail = __atomic_load_n(&ring->tail, __ATOMIC_ACQUIRE);

	return (size_t)(head - tail);
}

void saturating_increment_u32(uint32_t *counter)
{
	if (*counter != UINT32_MAX) {
		(*counter)++;
	}
}
