#!/usr/bin/env bash
set -u

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd -P)"
VENV_DIR="${ZEPHYR_VENV:-/home/cristian/.venvs/zephyr}"
WORKSPACE="${ZEPHYR_WORKSPACE:-/home/cristian/zephyrproject}"
WEST="${VENV_DIR}/bin/west"
PYTHON="${VENV_DIR}/bin/python"
BOARD_DIR="${WORKSPACE}/zephyr/boards/st/nucleo_l476rg"
failures=0

pass() { printf 'PASS: %s\n' "$1"; }
fail() { printf 'FAIL: %s\n' "$1" >&2; failures=$((failures + 1)); }

printf 'Repository: %s\nVirtual environment: %s\nZephyr workspace: %s\n' \
  "${REPO_ROOT}" "${VENV_DIR}" "${WORKSPACE}"

if [[ -x "${PYTHON}" ]]; then pass "Python: $("${PYTHON}" --version 2>&1)"; else fail "Python missing: ${PYTHON}"; fi
if [[ -x "${WEST}" ]]; then
  west_version="$("${WEST}" --version 2>&1 || true)"
  [[ "${west_version}" == *"West version: v1.5.0"* ]] && pass "${west_version}" || fail "expected West 1.5.0; found ${west_version}"
else
  fail "West missing: ${WEST}"
fi
if [[ -f "${WORKSPACE}/zephyr/VERSION" ]] && grep -q '^VERSION_MAJOR = 4$' "${WORKSPACE}/zephyr/VERSION" && grep -q '^VERSION_MINOR = 4$' "${WORKSPACE}/zephyr/VERSION"; then
  pass "Zephyr 4.4.0 source: ${WORKSPACE}/zephyr"
else
  fail "Zephyr 4.4.0 source not found at ${WORKSPACE}/zephyr"
fi
[[ -d "${BOARD_DIR}" ]] && pass "Board support: nucleo_l476rg" || fail "board support missing: ${BOARD_DIR}"

if [[ -n "${ZEPHYR_SDK_INSTALL_DIR:-}" && -d "${ZEPHYR_SDK_INSTALL_DIR}" ]]; then
  pass "Zephyr SDK/toolchain: ${ZEPHYR_SDK_INSTALL_DIR}"
elif compgen -G '/home/cristian/zephyr-sdk-*' >/dev/null; then
  pass "Zephyr SDK/toolchain: $(compgen -G '/home/cristian/zephyr-sdk-*' | head -n 1)"
else
  fail "Zephyr SDK/toolchain not found (set ZEPHYR_SDK_INSTALL_DIR)"
fi

programmer="$(command -v STM32_Programmer_CLI 2>/dev/null || true)"
[[ -n "${programmer}" ]] && pass "STM32CubeProgrammer: ${programmer}" || fail "STM32_Programmer_CLI is not on PATH"
if command -v stty >/dev/null; then
  serial_devices="$(compgen -G '/dev/serial/by-id/*' || true)"
  [[ -n "${serial_devices}" ]] || serial_devices="$(compgen -G '/dev/ttyACM*' || true)"
  pass "Serial prerequisite: stty available; detected devices: ${serial_devices:-none (connect ST-LINK before monitoring)}"
else
  fail "serial prerequisite missing: stty"
fi

if (( failures > 0 )); then
  printf 'Diagnostics failed: %d prerequisite(s) unavailable or mismatched. No changes were made.\n' "${failures}" >&2
  exit 1
fi
printf 'Diagnostics passed. No changes were made.\n'
