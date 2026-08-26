#include "espat_evidence.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define OUTPUT_CAPACITY 8192U

struct output_capture {
	char bytes[OUTPUT_CAPACITY];
	size_t length;
	bool overflow;
};

static void capture_write(const char *data, size_t length, void *context)
{
	struct output_capture *capture = context;

	if (length > sizeof(capture->bytes) - 1U - capture->length) {
		capture->overflow = true;
		return;
	}
	memcpy(&capture->bytes[capture->length], data, length);
	capture->length += length;
	capture->bytes[capture->length] = '\0';
}

static void feed(struct espat_transaction_evidence *evidence,
		 const uint8_t *bytes, size_t length)
{
	evidence->executed = true;
	for (size_t i = 0U; i < length; i++) {
		espat_response_feed(&evidence->response, bytes[i]);
	}
	espat_response_finish(&evidence->response);
}

static void test_complete_report_and_identity(void)
{
	static const uint8_t at_bytes[] = "AT\r\r\nOK\r\n";
	static const uint8_t gmr_bytes[] =
		"AT+GMR\r\r\nAT version:4.1.1.0-dev\r\n"
		"SDK version:v5.4.2\r\nOK\r\n";
	struct espat_transaction_evidence at = {.name = "AT", .tx_count = 4U};
	struct espat_transaction_evidence gmr = {.name = "AT+GMR", .tx_count = 8U};
	struct output_capture output = {0};

	feed(&at, at_bytes, sizeof(at_bytes) - 1U);
	feed(&gmr, gmr_bytes, sizeof(gmr_bytes) - 1U);
	assert(espat_evidence_allows_gmr(&at));
	espat_evidence_emit(&at, &gmr, capture_write, &output);
	assert(!output.overflow);
	assert(strstr(output.bytes, "ESPAT_EVIDENCE_BEGIN v=1 capacity=512\n") != NULL);
	assert(strstr(output.bytes, "AT executed=1 tx=4 rx=9 result=OK") != NULL);
	assert(strstr(output.bytes,
		      "timeout=0 tx_error=0 overflow=0 truncated=0 echo=1 urc=0 prompts=0") != NULL);
	assert(strstr(output.bytes, "PE=0 FE=0 NE=0 ORE=0 ring_overflow=0") != NULL);
	assert(strstr(output.bytes, "AT rx_hex=41 54 0D 0D 0A 4F 4B 0D 0A") != NULL);
	assert(strstr(output.bytes, "AT rx_printable=AT...OK..") != NULL);
	assert(strstr(output.bytes, "AT+GMR at_version=AT version:4.1.1.0-dev") != NULL);
	assert(strstr(output.bytes, "AT+GMR sdk_version=SDK version:v5.4.2") != NULL);
	assert(strstr(output.bytes, "ESPAT_EVIDENCE_END\n") != NULL);
}

static void test_maximum_and_binary_response(void)
{
	struct espat_transaction_evidence at = {.name = "AT", .tx_count = 4U};
	struct espat_transaction_evidence gmr = {.name = "AT+GMR"};
	struct output_capture output = {0};
	uint8_t bytes[ESPAT_RESPONSE_CAPACITY];

	at.executed = true;
	memset(bytes, 'x', sizeof(bytes));
	bytes[0] = 0U;
	for (size_t i = 99U; i + 1U < sizeof(bytes); i += 100U) {
		bytes[i] = '\r';
		bytes[i + 1U] = '\n';
	}
	for (size_t i = 0U; i < ESPAT_RESPONSE_CAPACITY; i++) {
		espat_response_feed(&at.response, bytes[i]);
	}
	espat_response_finish(&at.response);
	at.timed_out = true;
	espat_evidence_emit(&at, &gmr, capture_write, &output);
	assert(!output.overflow);
	assert(strstr(output.bytes,
		      "rx=512 result=TIMEOUT timeout=1 tx_error=0 overflow=0 truncated=0") != NULL);
	assert(strstr(output.bytes, "AT rx_hex=00 78 78 78") != NULL);
	assert(strstr(output.bytes, "AT rx_printable=.xxxxxxxxxxxxxxx") != NULL);
	assert(strstr(output.bytes, "AT+GMR executed=0\n") != NULL);
	assert(output.length > 2100U);
	assert(strstr(output.bytes, "ESPAT_EVIDENCE_END\n") != NULL);
}

static void test_maximum_identity_fields(void)
{
	struct espat_transaction_evidence at = {.name = "AT", .tx_count = 4U};
	struct espat_transaction_evidence gmr = {.name = "AT+GMR", .tx_count = 8U};
	struct output_capture output = {0};
	uint8_t response[ESPAT_RESPONSE_CAPACITY];
	size_t used = 0U;

	response[used++] = '\0';
	response[used++] = 0xffU;
	response[used++] = '\r';
	response[used++] = '\n';
	used += (size_t)snprintf((char *)response + used, sizeof(response) - used,
				 "AT version:");
	memset(response + used, 'A', ESPAT_IDENTITY_CAPACITY);
	used += ESPAT_IDENTITY_CAPACITY;
	response[used++] = '\r';
	response[used++] = '\n';
	used += (size_t)snprintf((char *)response + used, sizeof(response) - used,
				 "SDK version:");
	memset(response + used, 'I', ESPAT_IDENTITY_CAPACITY);
	used += ESPAT_IDENTITY_CAPACITY;
	response[used++] = '\r';
	response[used++] = '\n';
	response[used++] = 'O';
	response[used++] = 'K';
	response[used++] = '\r';
	response[used++] = '\n';
	feed(&at, (const uint8_t *)"OK\r\n", 4U);
	feed(&gmr, response, used);
	espat_evidence_emit(&at, &gmr, capture_write, &output);
	assert(!output.overflow);
	assert(strstr(output.bytes, "AT+GMR at_version=AT version:") != NULL);
	assert(strstr(output.bytes, "AT+GMR sdk_version=SDK version:") != NULL);
	assert(strstr(output.bytes, "ESPAT_EVIDENCE_END\n") != NULL);
}

static void test_fail_closed_and_overflow(void)
{
	static const uint8_t error_bytes[] = "ERROR\r\n";
	struct espat_transaction_evidence at = {.name = "AT", .tx_count = 4U};
	struct espat_transaction_evidence gmr = {.name = "AT+GMR"};
	struct output_capture output = {0};

	feed(&at, error_bytes, sizeof(error_bytes) - 1U);
	assert(!espat_evidence_allows_gmr(&at));
	at.errors.noise = 1U;
	assert(strcmp(espat_evidence_classification(&at), "UART_ERROR") == 0);
	at.errors.noise = 0U;
	for (size_t i = at.response.length; i <= ESPAT_RESPONSE_CAPACITY; i++) {
		espat_response_feed(&at.response, 'x');
	}
	assert(!espat_evidence_allows_gmr(&at));
	assert(strcmp(espat_evidence_classification(&at), "OVERFLOW") == 0);
	espat_evidence_emit(&at, &gmr, capture_write, &output);
	assert(strstr(output.bytes, "result=OVERFLOW") != NULL);
	assert(strstr(output.bytes, "overflow=1 truncated=1") != NULL);
	assert(strstr(output.bytes, "AT+GMR executed=0") != NULL);
}

static void test_gmr_gate_rejects_every_failure_signal(void)
{
	struct espat_transaction_evidence at = {.name = "AT", .executed = true};

	at.response.final = ESPAT_FINAL_OK;
	assert(espat_evidence_allows_gmr(&at));
	at.transmit_failed = true;
	assert(!espat_evidence_allows_gmr(&at));
	at.transmit_failed = false;
	at.timed_out = true;
	assert(!espat_evidence_allows_gmr(&at));
	at.timed_out = false;
	at.response.overflow = true;
	assert(!espat_evidence_allows_gmr(&at));
	at.response.overflow = false;
	at.errors.parity = 1U;
	assert(!espat_evidence_allows_gmr(&at));
	at.errors.parity = 0U;
	at.errors.framing = 1U;
	assert(!espat_evidence_allows_gmr(&at));
	at.errors.framing = 0U;
	at.errors.noise = 1U;
	assert(!espat_evidence_allows_gmr(&at));
	at.errors.noise = 0U;
	at.errors.overrun = 1U;
	assert(!espat_evidence_allows_gmr(&at));
	at.errors.overrun = 0U;
	at.errors.ring_overflow = 1U;
	assert(!espat_evidence_allows_gmr(&at));
}

int main(void)
{
	test_complete_report_and_identity();
	test_maximum_and_binary_response();
	test_maximum_identity_fields();
	test_fail_closed_and_overflow();
	test_gmr_gate_rejects_every_failure_signal();
	puts("ESP-AT deterministic evidence host tests: PASS");
	return 0;
}
