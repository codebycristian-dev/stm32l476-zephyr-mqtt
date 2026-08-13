#include "byte_ring.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>

static void test_empty_and_full(void)
{
	struct byte_ring ring;
	uint8_t value = 0xaaU;

	byte_ring_init(&ring);
	assert(byte_ring_count(&ring) == 0U);
	assert(!byte_ring_pop(&ring, &value));
	for (uint32_t i = 0U; i < BYTE_RING_CAPACITY; ++i) {
		assert(byte_ring_push(&ring, (uint8_t)i));
	}
	assert(byte_ring_count(&ring) == BYTE_RING_CAPACITY);
	assert(!byte_ring_push(&ring, 0xffU));
}

static void test_fifo_and_wraparound(void)
{
	struct byte_ring ring;
	uint8_t value;

	byte_ring_init(&ring);
	for (uint32_t i = 0U; i < BYTE_RING_CAPACITY; ++i) {
		assert(byte_ring_push(&ring, (uint8_t)i));
	}
	for (uint32_t i = 0U; i < BYTE_RING_CAPACITY / 2U; ++i) {
		assert(byte_ring_pop(&ring, &value));
		assert(value == (uint8_t)i);
	}
	for (uint32_t i = 0U; i < BYTE_RING_CAPACITY / 2U; ++i) {
		assert(byte_ring_push(&ring, (uint8_t)(0x80U + i)));
	}
	for (uint32_t i = BYTE_RING_CAPACITY / 2U; i < BYTE_RING_CAPACITY; ++i) {
		assert(byte_ring_pop(&ring, &value));
		assert(value == (uint8_t)i);
	}
	for (uint32_t i = 0U; i < BYTE_RING_CAPACITY / 2U; ++i) {
		assert(byte_ring_pop(&ring, &value));
		assert(value == (uint8_t)(0x80U + i));
	}
	assert(!byte_ring_pop(&ring, &value));
}

static void test_drop_newest_preserves_buffer(void)
{
	struct byte_ring ring;
	uint8_t value;

	byte_ring_init(&ring);
	for (uint32_t i = 0U; i < BYTE_RING_CAPACITY; ++i) {
		assert(byte_ring_push(&ring, (uint8_t)i));
	}
	assert(!byte_ring_push(&ring, 0xeeU));
	for (uint32_t i = 0U; i < BYTE_RING_CAPACITY; ++i) {
		assert(byte_ring_pop(&ring, &value));
		assert(value == (uint8_t)i);
	}
}

static void test_counter_saturation(void)
{
	uint32_t counter = UINT32_MAX - 1U;

	saturating_increment_u32(&counter);
	assert(counter == UINT32_MAX);
	saturating_increment_u32(&counter);
	assert(counter == UINT32_MAX);
}

int main(void)
{
	test_empty_and_full();
	test_fifo_and_wraparound();
	test_drop_newest_preserves_buffer();
	test_counter_saturation();
	puts("PASS: byte ring empty/full, FIFO, wraparound, overflow preservation, saturation");
	return 0;
}
