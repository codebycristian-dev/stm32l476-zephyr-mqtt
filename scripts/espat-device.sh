#!/usr/bin/env bash
set -euo pipefail

EXPECTED_VID="1a86"
EXPECTED_PID="55d3"
EXPECTED_SERIAL="5B14063285"
SYS_USB_ROOT="${ESPAT_SYS_USB_ROOT:-/sys/bus/usb/devices}"
DEV_ROOT="${ESPAT_DEV_ROOT:-/dev}"
TTY_CLASS_ROOT="${ESPAT_TTY_CLASS_ROOT:-/sys/class/tty}"

fail() { printf 'FAIL: %s\n' "$1" >&2; exit 1; }
read_value() { tr -d '\r\n' < "$1"; }

matches=()
for device in "${SYS_USB_ROOT}"/*; do
  [[ -r "${device}/idVendor" && -r "${device}/idProduct" && -r "${device}/serial" ]] || continue
  [[ "$(read_value "${device}/idVendor")" == "${EXPECTED_VID}" ]] || continue
  [[ "$(read_value "${device}/idProduct")" == "${EXPECTED_PID}" ]] || continue
  [[ "$(read_value "${device}/serial")" == "${EXPECTED_SERIAL}" ]] || continue
  matches+=("${device}")
done

(( ${#matches[@]} == 1 )) || fail "expected exactly one ${EXPECTED_VID}:${EXPECTED_PID}/${EXPECTED_SERIAL}; found ${#matches[@]}"
usb_name="$(basename "${matches[0]}")"
mapfile -t ttys < <(
  for tty_link in "${TTY_CLASS_ROOT}"/ttyACM* "${TTY_CLASS_ROOT}"/ttyUSB*; do
    [[ -e "${tty_link}" ]] || continue
    tty_real="$(readlink -f "${tty_link}")"
    [[ "/${tty_real}/" == *"/${usb_name}/"* ]] && basename "${tty_link}"
  done | sort -u
)
(( ${#ttys[@]} == 1 )) || fail "matched USB device does not expose exactly one serial endpoint"

stable="${DEV_ROOT}/serial/by-id/usb-1a86_USB_Single_Serial_${EXPECTED_SERIAL}-if00"
if [[ ! -e "${stable}" ]]; then
  mapfile -t links < <(find "${DEV_ROOT}/serial/by-id" -maxdepth 1 -type l -name "*${EXPECTED_SERIAL}*" -print 2>/dev/null)
  (( ${#links[@]} == 1 )) || fail "stable by-id path for matched device is missing or ambiguous"
  stable="${links[0]}"
fi
[[ "$(basename "$(readlink -f "${stable}")")" == "${ttys[0]}" ]] || fail "stable path does not resolve to matched USB endpoint"
printf '%s\n' "${stable}"
