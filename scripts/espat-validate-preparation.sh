#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
manifest="${REPO_ROOT}/docs/espat-provisioning-manifest.md"
[[ -r "${manifest}" ]] || { printf 'FAIL: manifest missing\n' >&2; exit 1; }
rg -q 'Authorization status: EXECUTED ONCE' "${manifest}"
rg -q '0x000000.*0x1000000' "${manifest}"
rg -q 'GPIO6.*RX' "${manifest}"
rg -q 'GPIO7.*TX' "${manifest}"
if git -C "${REPO_ROOT}" ls-files 'private/espat-backups/**' 'private/espat-evidence/**' | grep -q .; then
  printf 'FAIL: private ESP evidence is tracked\n' >&2
  exit 1
fi
if rg -n 'erase-flash|erase-region|write-flash|write_flash|AT\+RESTORE' \
  "${REPO_ROOT}/scripts/espat-device.sh" "${REPO_ROOT}/scripts/espat-read-backup.sh"; then
  printf 'FAIL: executable ESP workflow contains a destructive command\n' >&2
  exit 1
fi
printf 'PASS: preparation is fail-closed and private evidence is untracked\n'
