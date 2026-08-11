#!/usr/bin/env bash
set -euo pipefail

DEVICE="/dev/ttyACM0"
BAUD=115200
if [[ "${1:-}" == "--device" && -n "${2:-}" && $# -eq 2 ]]; then
  DEVICE="$2"
elif [[ $# -ne 0 ]]; then
  printf 'Usage: %s [--device /dev/ttyACM0]\n' "$0" >&2
  exit 2
fi
[[ -e "${DEVICE}" ]] || { printf 'ERROR: serial device does not exist: %s\n' "${DEVICE}" >&2; exit 1; }
[[ -r "${DEVICE}" && -w "${DEVICE}" ]] || { printf 'ERROR: serial device is not accessible: %s; check your existing group membership. Permissions were not changed.\n' "${DEVICE}" >&2; exit 1; }

printf 'Opening ST-LINK virtual COM console %s at %d baud, 8-N-1.\n' "${DEVICE}" "${BAUD}"
command -v stty >/dev/null || { printf 'ERROR: stty is required to configure the serial device. No package was installed.\n' >&2; exit 1; }
command -v cat >/dev/null || { printf 'ERROR: cat is required to read the serial device. No package was installed.\n' >&2; exit 1; }

stty -F "${DEVICE}" "${BAUD}" cs8 -cstopb -parenb -ixon -ixoff -crtscts raw -echo
printf 'Serial mode: raw, no echo, no software flow control, no hardware flow control. Press Ctrl-C to stop.\n'
exec cat "${DEVICE}"
