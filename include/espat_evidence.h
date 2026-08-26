#ifndef ESPAT_EVIDENCE_H
#define ESPAT_EVIDENCE_H

#include <stdbool.h>
#include <stddef.h>

#include "espat_response.h"
#include "usart1_transport.h"

struct espat_transaction_evidence {
	const char *name;
	size_t tx_count;
	struct espat_response response;
	struct usart1_error_counters errors;
	bool executed;
	bool transmit_failed;
	bool timed_out;
};

typedef void (*espat_evidence_write_fn)(const char *data, size_t length,
					void *context);

const char *espat_evidence_classification(
	const struct espat_transaction_evidence *evidence);
bool espat_evidence_allows_gmr(
	const struct espat_transaction_evidence *at_evidence);
void espat_evidence_emit(const struct espat_transaction_evidence *at_evidence,
			 const struct espat_transaction_evidence *gmr_evidence,
			 espat_evidence_write_fn write, void *context);

#endif
