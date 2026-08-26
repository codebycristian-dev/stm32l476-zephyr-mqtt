#include "espat_response.h"

#include <string.h>

static bool line_equals(const struct espat_response *response, const char *text)
{
	size_t length = strlen(text);

	return response->line_length == length &&
	       memcmp(response->line, text, length) == 0;
}

static bool line_is_echo(const struct espat_response *response)
{
	return line_equals(response, "AT") || line_equals(response, "AT+GMR");
}

static void classify_line(struct espat_response *response)
{
	if (response->line_length == 0U) {
		return;
	}
	if (line_is_echo(response)) {
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
	response->ended_with_lf = byte == '\n';
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
	return response != NULL && response->final != ESPAT_FINAL_NONE &&
	       response->ended_with_lf;
}


static void copy_identity_line(char *destination, const uint8_t *line, size_t length)
{
	size_t copy_length = length < ESPAT_IDENTITY_CAPACITY - 1U ?
		length : ESPAT_IDENTITY_CAPACITY - 1U;

	memcpy(destination, line, copy_length);
	destination[copy_length] = '\0';
}

void espat_response_extract_identity(const struct espat_response *response,
				     struct espat_identity *identity)
{
	size_t start = 0U;

	if (identity == NULL) {
		return;
	}
	*identity = (struct espat_identity){0};
	if (response == NULL) {
		return;
	}
	for (size_t i = 0U; i <= response->length; i++) {
		bool at_end = i == response->length;
		uint8_t byte = at_end ? '\n' : response->bytes[i];
		size_t length;

		if (byte != '\r' && byte != '\n') {
			continue;
		}
		length = i - start;
		if (length >= 11U && memcmp(&response->bytes[start], "AT version:", 11U) == 0) {
			copy_identity_line(identity->esp_at, &response->bytes[start], length);
		} else if (length >= 12U &&
			   memcmp(&response->bytes[start], "SDK version:", 12U) == 0) {
			copy_identity_line(identity->esp_idf, &response->bytes[start], length);
		}
		start = i + 1U;
	}
}
