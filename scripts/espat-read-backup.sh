#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd -P)"
PYTHON="${ESPAT_PYTHON:-/home/cristian/.venvs/zephyr/bin/python}"
BACKUP_DIR="${ESPAT_BACKUP_DIR:-${REPO_ROOT}/private/espat-backups}"
EXPECTED_SIZE=16777216
READ_BAUD="${ESPAT_READ_BAUD:-921600}"

[[ "${1:-}" == "--read-only-backup" ]] || { printf 'Usage: %s --read-only-backup\n' "$0" >&2; exit 2; }
port="$(${SCRIPT_DIR}/espat-device.sh)"
mkdir -p "${BACKUP_DIR}"
stamp="$(date -u +%Y%m%dT%H%M%SZ)"
backup="${BACKUP_DIR}/esp32c6-${stamp}-full-16MiB.bin"
log="${BACKUP_DIR}/esp32c6-${stamp}-read.log"

flash_output="$("${PYTHON}" -m esptool --chip esp32c6 --port "${port}" --baud "${READ_BAUD}" --before default-reset --after hard-reset flash-id)"
printf '%s\n' "${flash_output}" | tee -a "${log}"
grep -Eq 'Detected flash size:[[:space:]]*16MB' <<<"${flash_output}" || { printf 'FAIL: physical flash is not exactly 16MB\n' >&2; exit 1; }
"${SCRIPT_DIR}/espat-device.sh" >/dev/null
printf 'COMMAND: python -m esptool --chip esp32c6 --port <matched-by-id> --baud %s read-flash 0x000000 0x1000000 %s\n' "${READ_BAUD}" "$(basename "${backup}")" | tee -a "${log}"
"${PYTHON}" -m esptool --chip esp32c6 --port "${port}" --baud "${READ_BAUD}" --before default-reset --after hard-reset read-flash 0x000000 0x1000000 "${backup}" | tee -a "${log}"
[[ -r "${backup}" ]] || { printf 'FAIL: backup is unreadable\n' >&2; exit 1; }
[[ "$(stat -c %s "${backup}")" -eq "${EXPECTED_SIZE}" ]] || { printf 'FAIL: backup size mismatch\n' >&2; exit 1; }
sha256sum "${backup}" | tee "${backup}.sha256"
sha256sum --check "${backup}.sha256"
git -C "${REPO_ROOT}" check-ignore -q "${backup}" || { printf 'FAIL: backup is not ignored by Git\n' >&2; exit 1; }
git -C "${REPO_ROOT}" ls-files --error-unmatch "${backup}" >/dev/null 2>&1 && { printf 'FAIL: backup is tracked\n' >&2; exit 1; }
printf 'PASS: preserved %s (%s bytes)\n' "${backup}" "${EXPECTED_SIZE}"
