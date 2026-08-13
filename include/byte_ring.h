#ifndef BYTE_RING_H
#define BYTE_RING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Capacity must be a power of two and is fully usable. */
#define BYTE_RING_CAPACITY 64U

struct byte_ring {
	uint8_t data[BYTE_RING_CAPACITY];
	uint32_t head;
	uint32_t tail;
};

void byte_ring_init(struct byte_ring *ring);
bool byte_ring_push(struct byte_ring *ring, uint8_t byte);
bool byte_ring_pop(struct byte_ring *ring, uint8_t *byte);
size_t byte_ring_count(const struct byte_ring *ring);
void saturating_increment_u32(uint32_t *counter);

#endif
