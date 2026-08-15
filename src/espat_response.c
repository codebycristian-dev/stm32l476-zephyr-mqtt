#include "espat_response.h"

#include <string.h>

static bool line_equals(const struct espat_response *response, const char *text)
{
	size_t length = strlen(text);

	return response->line_length == length &&
	       memcmp(response->line, text, length) == 0;
}

static void classify_line(struct espat_response *response)
{
	if (response->line_length == 0U) {
		return;
	}
	if (line_equals(response, "AT")) {
		response->echo_count++;
	} else if (response->line[0] == '>') {
		/* Prompts are counted as bytes arrive, not as unsolicited lines. */
	} else if (line_equals(response, "OK")) {
		response->final = ESPAT_FINAL_OK;
	} else if (line_equals(response, "ERROR") || line_equals(response, "FAIL")) {
		response->final = ESPAT_FINAL_ERROR;
	} else {
		response->unsolicited_count++;
	}
	response->line_length = 0U;
}

void espat_response_init(struct espat_response *response)
{
	if (response != NULL) {
		*response = (struct espat_response){0};
	}
}

void espat_response_feed(struct espat_response *response, uint8_t byte)
{
	if (response == NULL) {
		return;
	}
	if (response->length < sizeof(response->bytes)) {
		response->bytes[response->length++] = byte;
	} else {
		response->overflow = true;
	}
	if (byte == '>') {
		response->prompt_count++;
	}
	if (byte == '\r' || byte == '\n') {
		classify_line(response);
	} else if (response->line_length < sizeof(response->line)) {
		response->line[response->line_length++] = byte;
	} else {
		response->overflow = true;
	}
}

void espat_response_finish(struct espat_response *response)
{
	if (response != NULL) {
		classify_line(response);
	}
}

bool espat_response_complete(const struct espat_response *response)
{
	return response != NULL && response->final != ESPAT_FINAL_NONE;
}
