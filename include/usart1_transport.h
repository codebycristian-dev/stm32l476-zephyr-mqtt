#ifndef USART1_TRANSPORT_H
#define USART1_TRANSPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "byte_ring.h"

#define USART1_RX_CAPACITY BYTE_RING_CAPACITY
#define USART1_BAUD_RATE 115200U

struct usart1_error_counters {
	uint32_t parity;
	uint32_t framing;
	uint32_t noise;
	uint32_t overrun;
	uint32_t ring_overflow;
};

struct usart1_clock_info {
	uint32_t sysclk_hz;
	uint32_t pclk2_hz;
	uint32_t peripheral_hz;
	uint32_t brr;
	uint32_t nominal_baud;
	uint8_t usart1_clock_source;
	uint8_t apb2_divisor;
};

/* Returns 0, or a negative errno value if the live clock state is invalid. */
int usart1_transport_init(void);

/* Synchronous, binary-safe, application-context-only transmission. The call
 * returns after TC proves the final stop bit has left PA9. Empty buffers are
 * accepted; NULL is rejected when length is nonzero.
 */
int usart1_transport_write(const uint8_t *data, size_t length);

/* Non-blocking: true means one byte was returned, false means empty/bad arg. */
bool usart1_transport_read(uint8_t *byte);
size_t usart1_transport_rx_count(void);
void usart1_transport_get_errors(struct usart1_error_counters *snapshot);
void usart1_transport_get_clock(struct usart1_clock_info *snapshot);

#endif
