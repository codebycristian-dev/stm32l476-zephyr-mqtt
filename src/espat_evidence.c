#include "espat_evidence.h"

#include <stdint.h>
#include <string.h>

#define EMIT_CHUNK_CAPACITY 64U

static bool has_uart_error(const struct usart1_error_counters *errors)
{
	return errors->parity != 0U || errors->framing != 0U ||
	       errors->noise != 0U || errors->overrun != 0U ||
	       errors->ring_overflow != 0U;
}

const char *espat_evidence_classification(
	const struct espat_transaction_evidence *evidence)
{
	if (evidence == NULL || !evidence->executed) {
		return "NOT_EXECUTED";
	}
	if (evidence->response.overflow) {
		return "OVERFLOW";
	}
	if (evidence->transmit_failed) {
		return "TX_ERROR";
	}
	if (has_uart_error(&evidence->errors)) {
		return "UART_ERROR";
	}
	if (evidence->response.final == ESPAT_FINAL_OK) {
		return "OK";
	}
	if (evidence->response.final == ESPAT_FINAL_ERROR) {
		return "ERROR";
	}
	return "TIMEOUT";
}

bool espat_evidence_allows_gmr(
	const struct espat_transaction_evidence *at_evidence)
{
	return at_evidence != NULL && at_evidence->executed &&
	       at_evidence->response.final == ESPAT_FINAL_OK &&
	       !at_evidence->transmit_failed && !at_evidence->timed_out &&
	       !at_evidence->response.overflow &&
	       !has_uart_error(&at_evidence->errors);
}

static void emit_text(espat_evidence_write_fn write, void *context,
		      const char *text)
{
	write(text, strlen(text), context);
}

static void emit_unsigned(espat_evidence_write_fn write, void *context,
			  uint32_t value)
{
	char digits[10];
	size_t length = 0U;

	do {
		digits[length++] = (char)('0' + value % 10U);
		value /= 10U;
	} while (value != 0U);
	for (size_t i = 0U; i < length / 2U; i++) {
		char temporary = digits[i];

		digits[i] = digits[length - 1U - i];
		digits[length - 1U - i] = temporary;
	}
	write(digits, length, context);
}

static void emit_field(espat_evidence_write_fn write, void *context,
		       const char *name, uint32_t value)
{
	emit_text(write, context, " ");
	emit_text(write, context, name);
	emit_text(write, context, "=");
	emit_unsigned(write, context, value);
}

static void emit_hex(const struct espat_response *response,
		     espat_evidence_write_fn write, void *context)
{
	static const char digits[] = "0123456789ABCDEF";
	char chunk[EMIT_CHUNK_CAPACITY];
	size_t used = 0U;

	for (size_t i = 0U; i < response->length; i++) {
		uint8_t value = response->bytes[i];
		char encoded[3] = {
			digits[value >> 4], digits[value & 0x0fU],
			i + 1U < response->length ? ' ' : '\0'
		};
		size_t count = i + 1U < response->length ? 3U : 2U;

		if (used + count > sizeof(chunk)) {
			write(chunk, used, context);
			used = 0U;
		}
		memcpy(&chunk[used], encoded, count);
		used += count;
	}
	if (used != 0U) {
		write(chunk, used, context);
	}
}

static void emit_printable(const struct espat_response *response,
			   espat_evidence_write_fn write, void *context)
{
	char chunk[EMIT_CHUNK_CAPACITY];
	size_t used = 0U;

	for (size_t i = 0U; i < response->length; i++) {
		uint8_t value = response->bytes[i];

		chunk[used++] = value >= 0x20U && value <= 0x7eU ?
			(char)value : '.';
		if (used == sizeof(chunk)) {
			write(chunk, used, context);
			used = 0U;
		}
	}
	if (used != 0U) {
		write(chunk, used, context);
	}
}

static void emit_identity_line(const struct espat_response *response,
			       const char *prefix,
			       espat_evidence_write_fn write, void *context)
{
	size_t prefix_length = strlen(prefix);
	size_t start = 0U;

	for (size_t i = 0U; i <= response->length; i++) {
		bool at_end = i == response->length;
		uint8_t byte = at_end ? '\n' : response->bytes[i];
		size_t length;

		if (byte != '\r' && byte != '\n') {
			continue;
		}
		length = i - start;
		if (length >= prefix_length &&
		    memcmp(&response->bytes[start], prefix, prefix_length) == 0) {
			/* Match the existing bounded identity-field behavior without
			 * constructing a second copy on the main thread stack.
			 */
			length = length < ESPAT_IDENTITY_CAPACITY - 1U ?
				length : ESPAT_IDENTITY_CAPACITY - 1U;
			write((const char *)&response->bytes[start], length, context);
			return;
		}
		start = i + 1U;
	}
	emit_text(write, context, "unavailable");
}

static void emit_transaction(const struct espat_transaction_evidence *evidence,
			     espat_evidence_write_fn write, void *context)
{
	emit_text(write, context, evidence->name);
	emit_field(write, context, "executed", evidence->executed);
	if (!evidence->executed) {
		emit_text(write, context, "\n");
		return;
	}
	emit_field(write, context, "tx", (uint32_t)evidence->tx_count);
	emit_field(write, context, "rx", (uint32_t)evidence->response.length);
	emit_text(write, context, " result=");
	emit_text(write, context, espat_evidence_classification(evidence));
	emit_field(write, context, "timeout", evidence->timed_out);
	emit_field(write, context, "tx_error", evidence->transmit_failed);
	emit_field(write, context, "overflow", evidence->response.overflow);
	emit_field(write, context, "truncated", evidence->response.overflow);
	emit_field(write, context, "echo", evidence->response.echo_count);
	emit_field(write, context, "urc", evidence->response.unsolicited_count);
	emit_field(write, context, "prompts", evidence->response.prompt_count);
	emit_field(write, context, "PE", evidence->errors.parity);
	emit_field(write, context, "FE", evidence->errors.framing);
	emit_field(write, context, "NE", evidence->errors.noise);
	emit_field(write, context, "ORE", evidence->errors.overrun);
	emit_field(write, context, "ring_overflow", evidence->errors.ring_overflow);
	emit_text(write, context, "\n");
	emit_text(write, context, evidence->name);
	emit_text(write, context, " rx_hex=");
	emit_hex(&evidence->response, write, context);
	emit_text(write, context, "\n");
	emit_text(write, context, evidence->name);
	emit_text(write, context, " rx_printable=");
	emit_printable(&evidence->response, write, context);
	emit_text(write, context, "\n");
}

void espat_evidence_emit(const struct espat_transaction_evidence *at_evidence,
			 const struct espat_transaction_evidence *gmr_evidence,
			 espat_evidence_write_fn write, void *context)
{
	if (at_evidence == NULL || gmr_evidence == NULL || write == NULL) {
		return;
	}
	emit_text(write, context, "ESPAT_EVIDENCE_BEGIN v=1 capacity=512\n");
	emit_transaction(at_evidence, write, context);
	emit_transaction(gmr_evidence, write, context);
	if (gmr_evidence->executed) {
		emit_text(write, context, "AT+GMR at_version=");
		emit_identity_line(&gmr_evidence->response, "AT version:",
				   write, context);
		emit_text(write, context, "\nAT+GMR sdk_version=");
		emit_identity_line(&gmr_evidence->response, "SDK version:",
				   write, context);
		emit_text(write, context, "\n");
	}
	emit_text(write, context, "ESPAT_EVIDENCE_END\n");
}
