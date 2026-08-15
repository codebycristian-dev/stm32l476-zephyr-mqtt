#ifndef ESPAT_RESPONSE_H
#define ESPAT_RESPONSE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ESPAT_RESPONSE_CAPACITY 64U
#define ESPAT_LINE_CAPACITY 32U

enum espat_final_result {
	ESPAT_FINAL_NONE,
	ESPAT_FINAL_OK,
	ESPAT_FINAL_ERROR,
};

struct espat_response {
	uint8_t bytes[ESPAT_RESPONSE_CAPACITY];
	size_t length;
	size_t line_length;
	uint8_t line[ESPAT_LINE_CAPACITY];
	unsigned int echo_count;
	unsigned int unsolicited_count;
	unsigned int prompt_count;
	enum espat_final_result final;
	bool overflow;
};

void espat_response_init(struct espat_response *response);
void espat_response_feed(struct espat_response *response, uint8_t byte);
void espat_response_finish(struct espat_response *response);
bool espat_response_complete(const struct espat_response *response);

#endif
