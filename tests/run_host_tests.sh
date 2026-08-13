#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
test_binary="$(mktemp /tmp/stm32l476-byte-ring-test.XXXXXX)"
trap 'rm -f "${test_binary}"' EXIT

cc -std=c11 -Wall -Wextra -Werror -pedantic \
  -I"${repo_root}/include" \
  "${repo_root}/src/byte_ring.c" "${repo_root}/tests/test_byte_ring.c" \
  -o "${test_binary}"
"${test_binary}"
