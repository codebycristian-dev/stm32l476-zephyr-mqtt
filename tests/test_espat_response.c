#include "espat_response.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static struct espat_response parse(const char *text)
{
	struct espat_response response;

	espat_response_init(&response);
	for (size_t i = 0U; i < strlen(text); i++) {
		espat_response_feed(&response, (uint8_t)text[i]);
	}
	espat_response_finish(&response);
	return response;
}

int main(void)
{
	struct espat_identity identity;
	struct espat_response response = parse("AT\r\r\nOK\r\n");
	assert(response.final == ESPAT_FINAL_OK);
	assert(response.echo_count == 1U);
	assert(response.unsolicited_count == 0U);
	assert(espat_response_complete(&response));

	response = parse("AT+GMR\r\r\nAT version:4.1.1.0-dev\r\nSDK version:v5.4.2\r\nOK\r\n");
	assert(response.final == ESPAT_FINAL_OK);
	assert(response.echo_count == 1U);
	espat_response_extract_identity(&response, &identity);
	assert(strcmp(identity.esp_at, "AT version:4.1.1.0-dev") == 0);
	assert(strcmp(identity.esp_idf, "SDK version:v5.4.2") == 0);

	response = parse("ready\r\nWIFI DISCONNECT\r\nERROR\r\n");
	assert(response.final == ESPAT_FINAL_ERROR);
	assert(response.unsolicited_count == 2U);

	response = parse("> ");
	assert(response.prompt_count == 1U);
	assert(response.final == ESPAT_FINAL_NONE);
	assert(!espat_response_complete(&response));

	response = parse("");
	assert(response.final == ESPAT_FINAL_NONE);
	assert(response.length == 0U);
	assert(!espat_response_complete(&response));

	espat_response_init(&response);
	for (size_t i = 0U; i <= ESPAT_RESPONSE_CAPACITY; i++) {
		espat_response_feed(&response, 'x');
	}
	assert(response.length == ESPAT_RESPONSE_CAPACITY);
	assert(response.overflow);

	response = parse("ERROR\r\n");
	espat_response_extract_identity(&response, &identity);
	assert(identity.esp_at[0] == '\0');
	assert(identity.esp_idf[0] == '\0');

	puts("ESP-AT response parser host tests: PASS");
	return 0;
}
