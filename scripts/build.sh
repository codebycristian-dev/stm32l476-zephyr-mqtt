#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd -P)"
VENV_DIR="${ZEPHYR_VENV:-/home/cristian/.venvs/zephyr}"
WORKSPACE="${ZEPHYR_WORKSPACE:-/home/cristian/zephyrproject}"
BUILD_DIR="${BUILD_DIR:-${REPO_ROOT}/build/nucleo_l476rg}"
WEST="${VENV_DIR}/bin/west"
USER_CACHE_DIR="${REPO_ROOT}/build/.zephyr-cache"

case "$(realpath -m -- "${BUILD_DIR}")/" in
  "${REPO_ROOT}/"*) ;;
  *) printf 'ERROR: build directory must be inside repository: %s\n' "${REPO_ROOT}" >&2; exit 2 ;;
esac
[[ -x "${WEST}" ]] || { printf 'ERROR: West missing: %s\n' "${WEST}" >&2; exit 1; }
[[ -d "${WORKSPACE}/zephyr" ]] || { printf 'ERROR: Zephyr workspace missing: %s\n' "${WORKSPACE}" >&2; exit 1; }

printf 'Building %s for nucleo_l476rg\nBuild directory: %s\nWorkspace (read-only input): %s\n' \
  "${REPO_ROOT}" "${BUILD_DIR}" "${WORKSPACE}"
export CCACHE_DISABLE=1
cd -- "${WORKSPACE}"
"${WEST}" build --pristine=always --board nucleo_l476rg --source-dir "${REPO_ROOT}" --build-dir "${BUILD_DIR}" -- -DUSER_CACHE_DIR="${USER_CACHE_DIR}"
printf 'Build complete. The Zephyr output above reports FLASH/ROM and RAM used and available.\n'
