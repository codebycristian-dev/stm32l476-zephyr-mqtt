#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
SOURCE="${ESPAT_SOURCE:-${REPO_ROOT}/private/espat-source/esp-at-v4.1.1.0}"
BASE="${SOURCE}/module_config/module_esp32c6_default"
CUSTOM="${SOURCE}/module_config/module_esp32c6-16mb-tcp"
FACTORY="${SOURCE}/components/customized_partitions/raw_data/factory_param/factory_param_data.csv"

[[ "$(git -C "${SOURCE}" rev-parse HEAD)" == "7c092f9aee793d6dbfae31e7585637baa14fb4ce" ]] || { printf 'FAIL: unexpected ESP-AT source commit\n' >&2; exit 1; }
[[ -d "${BASE}" && -f "${FACTORY}" ]] || { printf 'FAIL: pinned source layout is incomplete\n' >&2; exit 1; }
mkdir -p "${CUSTOM}"
cp "${BASE}/IDF_VERSION" "${BASE}/at_customize.csv" "${BASE}/sdkconfig.defaults" "${BASE}/sdkconfig_silence.defaults" "${CUSTOM}/"
cp "${REPO_ROOT}/firmware/espat-16mb/partitions_at.csv" "${CUSTOM}/partitions_at.csv"
sed -i 's#module_config/module_esp32c6_default/#module_config/module_esp32c6-16mb-tcp/#g' "${CUSTOM}/sdkconfig.defaults" "${CUSTOM}/sdkconfig_silence.defaults"
sed -i -e '$r '"${REPO_ROOT}/firmware/espat-16mb/sdkconfig.append" "${CUSTOM}/sdkconfig.defaults"
sed -i -e '$r '"${REPO_ROOT}/firmware/espat-16mb/sdkconfig.append" "${CUSTOM}/sdkconfig_silence.defaults"
if ! rg -q '^PLATFORM_ESP32C6,ESP32C6-16MB-TCP,' "${FACTORY}"; then
  sed -i -e '$r '"${REPO_ROOT}/firmware/espat-16mb/factory-param-row.csv" "${FACTORY}"
fi
mkdir -p "${SOURCE}/build"
printf '%s\n' '{"platform":"PLATFORM_ESP32C6","module":"ESP32C6-16MB-TCP","silence":0}' > "${SOURCE}/build/module_info.json"
printf 'PASS: prepared pinned custom ESP32-C6 16 MB source configuration\n'
