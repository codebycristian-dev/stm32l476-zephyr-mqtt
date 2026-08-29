#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
UART0="${ESPAT_UART0_DEVICE:-}"
UART1="${ESPAT_UART1_DEVICE:-}"
AUTH="${ESPAT_POST_PROVISION_AUTHORIZATION:-}"

[[ "${1:-}" == "--authorized-post-provision-check" ]] || {
  printf 'FAIL: explicit --authorized-post-provision-check is required\n' >&2
  exit 2
}
[[ "${AUTH}" == "I_AUTHORIZE_CREDENTIAL_FREE_DEVICE_CHECKS" ]] || {
  printf 'FAIL: ESPAT_POST_PROVISION_AUTHORIZATION is missing or invalid\n' >&2
  exit 2
}
matched="$(${SCRIPT_DIR}/espat-device.sh)"
[[ -n "${UART0}" && -n "${UART1}" ]] || {
  printf 'FAIL: set distinct ESPAT_UART0_DEVICE and ESPAT_UART1_DEVICE paths\n' >&2
  exit 2
}
[[ "${UART0}" == "${matched}" ]] || { printf 'FAIL: UART0 is not the freshly matched target\n' >&2; exit 1; }
[[ "${UART0}" != "${UART1}" && -c "${UART0}" && -c "${UART1}" ]] || {
  printf 'FAIL: UART device paths are missing, identical, or not character devices\n' >&2
  exit 1
}

printf 'AUTHORIZED CHECK PLAN (credential-free):\n'
printf '1. Capture bounded UART0 boot output at 115200 8N1, raw, no flow control.\n'
printf '2. On UART1 send only AT\\r\\n and AT+GMR\\r\\n at 115200 8N1, raw, no flow control.\n'
printf '3. Require valid responses to prove host TX -> GPIO6 RX and GPIO7 TX -> host RX.\n'
printf 'This script intentionally stops at the reviewed plan; use the documented bounded capture procedure.\n'
