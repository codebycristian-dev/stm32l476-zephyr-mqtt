#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd -P)"
VENV_DIR="${ZEPHYR_VENV:-/home/cristian/.venvs/zephyr}"
WORKSPACE="${ZEPHYR_WORKSPACE:-/home/cristian/zephyrproject}"
BUILD_DIR="${BUILD_DIR:-${REPO_ROOT}/build/nucleo_l476rg}"
WEST="${VENV_DIR}/bin/west"
ARTIFACT="${BUILD_DIR}/zephyr/zephyr.elf"
DRY_RUN=0

[[ "${1:-}" == "--dry-run" ]] && DRY_RUN=1
[[ $# -le 1 ]] || { printf 'Usage: %s [--dry-run]\n' "$0" >&2; exit 2; }
case "$(realpath -m -- "${BUILD_DIR}")/" in
  "${REPO_ROOT}/"*) ;;
  *) printf 'ERROR: build directory must be inside repository: %s\n' "${REPO_ROOT}" >&2; exit 2 ;;
esac
[[ -x "${WEST}" ]] || { printf 'ERROR: West missing: %s\n' "${WEST}" >&2; exit 1; }
[[ -f "${ARTIFACT}" ]] || { printf 'ERROR: build artifact missing: %s; run scripts/build.sh first.\n' "${ARTIFACT}" >&2; exit 1; }
command -v STM32_Programmer_CLI >/dev/null || { printf 'ERROR: STM32_Programmer_CLI is not on PATH.\n' >&2; exit 1; }

printf 'Target: nucleo_l476rg\nArtifact: %s\nCommand: %q flash --skip-rebuild --build-dir %q\n' \
  "${ARTIFACT}" "${WEST}" "${BUILD_DIR}"
(( DRY_RUN == 1 )) && { printf 'Dry run only; hardware was not programmed.\n'; exit 0; }
cd -- "${WORKSPACE}"
"${WEST}" flash --skip-rebuild --build-dir "${BUILD_DIR}"
