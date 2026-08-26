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

int main(void)
{
	test_complete_report_and_identity();
	test_maximum_and_binary_response();
	test_fail_closed_and_overflow();
	puts("ESP-AT deterministic evidence host tests: PASS");
	return 0;
}
