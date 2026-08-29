#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
test_binary="$(mktemp /tmp/stm32l476-byte-ring-test.XXXXXX)"
parser_binary="$(mktemp /tmp/stm32l476-espat-parser-test.XXXXXX)"
evidence_binary="$(mktemp /tmp/stm32l476-espat-evidence-test.XXXXXX)"
evidence_object="$(mktemp /tmp/stm32l476-espat-evidence-stack.XXXXXX.o)"
trap 'rm -f "${test_binary}" "${parser_binary}" "${evidence_binary}" "${evidence_object}"' EXIT

cc -std=c11 -Wall -Wextra -Werror -pedantic \
  -I"${repo_root}/include" \
  "${repo_root}/src/byte_ring.c" "${repo_root}/tests/test_byte_ring.c" \
  -o "${test_binary}"
"${test_binary}"

cc -std=c11 -Wall -Wextra -Werror -pedantic \
  -I"${repo_root}/include" \
  "${repo_root}/src/espat_response.c" "${repo_root}/tests/test_espat_response.c" \
  -o "${parser_binary}"
"${parser_binary}"

# Keep the evidence implementation free of large automatic objects. Large
# transaction/response storage belongs in the caller's fixed static snapshots.
cc -std=c11 -Wall -Wextra -Werror -pedantic -Wstack-usage=192 \
  -I"${repo_root}/include" \
  -c "${repo_root}/src/espat_evidence.c" -o "${evidence_object}"

cc -std=c11 -Wall -Wextra -Werror -pedantic \
  -I"${repo_root}/include" \
  "${repo_root}/src/espat_response.c" \
  "${repo_root}/src/espat_evidence.c" \
  "${repo_root}/tests/test_espat_evidence.c" \
  -o "${evidence_binary}"
"${evidence_binary}"
