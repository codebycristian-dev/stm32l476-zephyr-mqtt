#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
fail() { printf 'FAIL: %s\n' "$1" >&2; exit 1; }
pass() { printf 'PASS: %s\n' "$1"; }

for script in doctor.sh build.sh flash.sh monitor.sh espat-device.sh espat-read-backup.sh espat-validate-preparation.sh prepare-espat-source.sh; do
  bash -n "${REPO_ROOT}/scripts/${script}" || fail "syntax: ${script}"
done
pass "shell syntax"

tmp_root="$(mktemp -d)"
trap 'rm -rf "${tmp_root}"' EXIT
mkdir -p "${tmp_root}/sys/a/tty/a:1.0/ttyACM9" "${tmp_root}/class" "${tmp_root}/dev/serial/by-id" "${tmp_root}/dev"
printf '1a86\n' > "${tmp_root}/sys/a/idVendor"
printf '55d3\n' > "${tmp_root}/sys/a/idProduct"
printf '5B14063285\n' > "${tmp_root}/sys/a/serial"
touch "${tmp_root}/dev/ttyACM9"
ln -s "${tmp_root}/sys/a/tty/a:1.0/ttyACM9" "${tmp_root}/class/ttyACM9"
ln -s ../../ttyACM9 "${tmp_root}/dev/serial/by-id/usb-WCH_5B14063285-if00"
resolved="$(ESPAT_SYS_USB_ROOT="${tmp_root}/sys" ESPAT_TTY_CLASS_ROOT="${tmp_root}/class" ESPAT_DEV_ROOT="${tmp_root}/dev" "${REPO_ROOT}/scripts/espat-device.sh")"
[[ "${resolved}" == "${tmp_root}/dev/serial/by-id/usb-WCH_5B14063285-if00" ]] || fail "ESP resolver returned wrong path"
printf 'different\n' > "${tmp_root}/sys/a/serial"
if ESPAT_SYS_USB_ROOT="${tmp_root}/sys" ESPAT_TTY_CLASS_ROOT="${tmp_root}/class" ESPAT_DEV_ROOT="${tmp_root}/dev" "${REPO_ROOT}/scripts/espat-device.sh" >/dev/null 2>&1; then
  fail "ESP resolver accepted a serial mismatch"
fi
pass "ESP resolver uniquely matches identity and fails closed"

"${REPO_ROOT}/scripts/espat-validate-preparation.sh" >/dev/null || fail "ESP preparation validation failed"
pass "ESP preparation remains non-destructive and private"

(cd /tmp && "${REPO_ROOT}/scripts/doctor.sh" >/dev/null 2>&1) || fail "doctor failed outside repository root"
[[ -x "${REPO_ROOT}/scripts/doctor.sh" ]] || fail "scripts are not executable"
pass "scripts resolve their repository from an unrelated working directory"

if ZEPHYR_VENV=/definitely/missing "${REPO_ROOT}/scripts/doctor.sh" >/dev/null 2>&1; then
  fail "doctor accepted a missing prerequisite"
fi
pass "doctor fails for a missing prerequisite"

if BUILD_DIR=/tmp/outside-repository "${REPO_ROOT}/scripts/build.sh" >/dev/null 2>&1; then
  fail "build accepted an external output directory"
fi
pass "build rejects output outside the repository"
rg -q -- '-DUSER_CACHE_DIR=' "${REPO_ROOT}/scripts/build.sh" || fail "build does not relocate the Zephyr cache"
rg -q '^export CCACHE_DISABLE=1$' "${REPO_ROOT}/scripts/build.sh" || fail "build does not disable the external compiler cache"
pass "build redirects generated caches into the repository"

if BUILD_DIR="${REPO_ROOT}/build/absent" "${REPO_ROOT}/scripts/flash.sh" --dry-run >/dev/null 2>&1; then
  fail "flash dry-run accepted a missing artifact"
fi
pass "flash validates artifacts before runner invocation"

if "${REPO_ROOT}/scripts/monitor.sh" --device /definitely/missing >/dev/null 2>&1; then
  fail "monitor accepted a missing serial device"
fi
pass "monitor fails for a missing serial device"
rg -Fq 'stty -F "${DEVICE}" "${BAUD}" cs8 -cstopb -parenb -ixon -ixoff -crtscts raw -echo' "${REPO_ROOT}/scripts/monitor.sh" || fail "monitor does not enforce the verified raw 115200 8-N-1 configuration"
rg -Fq 'exec cat "${DEVICE}"' "${REPO_ROOT}/scripts/monitor.sh" || fail "monitor does not use the verified raw capture workflow"
pass "monitor enforces raw 115200 8-N-1 without flow control or echo"

if rg -n '(apt|dnf|yum|pip)[[:space:]]+(install|upgrade)|west[[:space:]]+update|chmod' "${REPO_ROOT}/scripts"; then
  fail "workflow contains installation, update, or permission mutation"
fi
if rg -n '(^|[;&|])[[:space:]]*(cp|mv|mkdir|touch|tee)[[:space:]].*(/home/cristian/zephyrproject|WORKSPACE)' "${REPO_ROOT}/scripts"; then
  fail "workflow may write beneath the external workspace"
fi
pass "static workflow safety boundaries"

printf 'All script checks passed.\n'
