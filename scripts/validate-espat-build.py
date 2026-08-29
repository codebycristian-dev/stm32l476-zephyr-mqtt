#!/usr/bin/env python3
"""Validate the ignored ESP-AT build without contacting a device."""

from __future__ import annotations

import csv
import hashlib
import json
import subprocess
import sys
from pathlib import Path

FLASH_SIZE = 0x1000000
EXPECTED_FILES = {
    0x0: "bootloader/bootloader.bin",
    0x8000: "partition_table/partition-table.bin",
    0xD000: "ota_data_initial.bin",
    0x1E000: "at_customize.bin",
    0x1F000: "customized_partitions/mfg_nvs.bin",
    0x60000: "esp-at.bin",
}
EXPECTED_PARTITIONS = (
    ("otadata", "data", "ota", 0xD000, 0x2000),
    ("phy_init", "data", "phy", 0xF000, 0x1000),
    ("nvs", "data", "nvs", 0x10000, 0xE000),
    ("at_customize", "64", "0", 0x1E000, 0x42000),
    ("ota_0", "app", "ota_0", 0x60000, 0x7D0000),
    ("ota_1", "app", "ota_1", 0x830000, 0x7D0000),
)


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main() -> None:
    if len(sys.argv) != 2:
        fail("usage: validate-espat-build.py <esp-at-source>")
    source = Path(sys.argv[1]).resolve()
    build = source / "build"
    sdkconfig = source / "sdkconfig"
    args_path = build / "flasher_args.json"
    for required in (sdkconfig, args_path, build / "download.config"):
        if not required.is_file():
            fail(f"missing generated file: {required}")

    config = sdkconfig.read_text()
    required_config = (
        'CONFIG_IDF_TARGET="esp32c6"',
        "CONFIG_IDF_TARGET_ESP32C6=y",
        'CONFIG_ESPTOOLPY_FLASHMODE="dio"',
        'CONFIG_ESPTOOLPY_FLASHFREQ="80m"',
        'CONFIG_ESPTOOLPY_FLASHSIZE="16MB"',
        'CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="module_config/module_esp32c6-16mb-tcp/partitions_at.csv"',
    )
    for value in required_config:
        if value not in config:
            fail(f"generated sdkconfig lacks {value}")
    forbidden_enabled = (
        "CONFIG_AT_MQTT_COMMAND_SUPPORT=y",
        "CONFIG_AT_HTTP_COMMAND_SUPPORT=y",
        "CONFIG_BT_ENABLED=y",
        "CONFIG_AT_BLE_COMMAND_SUPPORT=y",
        "CONFIG_AT_BLUFI_COMMAND_SUPPORT=y",
        "CONFIG_AT_OTA_SUPPORT=y",
        "CONFIG_AT_MDNS_COMMAND_SUPPORT=y",
        "CONFIG_AT_PING_COMMAND_SUPPORT=y",
        "CONFIG_AT_SIGNALING_COMMAND_SUPPORT=y",
        "CONFIG_AT_SMARTCONFIG_COMMAND_SUPPORT=y",
        "CONFIG_AT_WPS_COMMAND_SUPPORT=y",
        "CONFIG_AT_USERWKMCU_COMMAND_SUPPORT=y",
        "CONFIG_AT_USER_COMMAND_SUPPORT=y",
    )
    for value in forbidden_enabled:
        if value in config:
            fail(f"out-of-scope feature enabled: {value}")

    flash = json.loads(args_path.read_text())
    if flash.get("flash_settings") != {
        "flash_mode": "dio", "flash_size": "16MB", "flash_freq": "80m"
    }:
        fail("flasher_args.json flash settings disagree")
    if flash.get("extra_esptool_args", {}).get("chip") != "esp32c6":
        fail("flasher_args.json target is not esp32c6")
    actual_files = {int(k, 0): v for k, v in flash.get("flash_files", {}).items()}
    if actual_files != EXPECTED_FILES:
        fail("generated offset/artifact map differs from the reviewed layout")

    ranges: list[tuple[int, int, str]] = []
    for offset, relative in sorted(EXPECTED_FILES.items()):
        path = build / relative
        if not path.is_file():
            fail(f"missing flashable binary: {relative}")
        end = offset + path.stat().st_size
        if end > FLASH_SIZE:
            fail(f"{relative} exceeds 16 MiB flash")
        ranges.append((offset, end, relative))
    for previous, current in zip(ranges, ranges[1:]):
        if previous[1] > current[0]:
            fail(f"flash ranges overlap: {previous[2]} and {current[2]}")

    partitions = source / "module_config/module_esp32c6-16mb-tcp/partitions_at.csv"
    repository_partitions = source.parents[2] / "firmware/espat-16mb/partitions_at.csv"
    partition_tool = source / "esp-idf/components/partition_table/gen_esp32part.py"
    decoded = subprocess.run(
        [sys.executable, str(partition_tool), str(build / "partition_table/partition-table.bin")],
        check=True, capture_output=True, text=True,
    ).stdout

    def read_partitions(lines: list[str]) -> tuple[tuple[str, str, str, int, int], ...]:
        result = []
        def normalized(value: str) -> str:
            return str(int(value, 0)) if value.startswith("0x") or value.isdigit() else value
        for row in csv.reader(line for line in lines if line.strip() and not line.lstrip().startswith("#")):
            name, kind, subtype, offset, size = (value.strip() for value in row[:5])
            result.append((name, normalized(kind), normalized(subtype), int(offset, 0), int(size.rstrip("K"), 0) * (1024 if size.endswith("K") else 1)))
        return tuple(result)

    configured = read_partitions(partitions.read_text().splitlines())
    repository_owned = read_partitions(repository_partitions.read_text().splitlines())
    generated = read_partitions(decoded.splitlines())
    if configured != EXPECTED_PARTITIONS or repository_owned != EXPECTED_PARTITIONS:
        fail("repository/custom module partition CSV differs from the reviewed exact layout")
    if generated != EXPECTED_PARTITIONS:
        fail("decoded generated partition binary differs from the reviewed exact layout")

    partition_ranges: list[tuple[int, int, str, str]] = []
    for name, kind, _subtype, start, length in generated:
        alignment = 0x10000 if kind == "app" else 0x1000
        if start % alignment:
            fail(f"partition {name} violates {alignment:#x} alignment")
        if start + length > FLASH_SIZE:
            fail(f"partition {name} exceeds 16 MiB flash")
        partition_ranges.append((start, start + length, name, kind))
    partition_ranges.sort()
    for previous, current in zip(partition_ranges, partition_ranges[1:]):
        if previous[1] > current[0]:
            fail(f"partitions overlap: {previous[2]} and {current[2]}")
    app_sizes = [end - start for start, end, _name, kind in partition_ranges if kind == "app"]
    if not app_sizes or min(app_sizes) < (build / "esp-at.bin").stat().st_size:
        fail("application partition is inadequate")

    factory_csv = build / "customized_partitions/mfg_nvs.csv"
    values = {}
    with factory_csv.open(newline="") as stream:
        for row in csv.reader(stream):
            if len(row) >= 4:
                values[row[0]] = row[3]
    expected_uart = {
        "module_name": "ESP32C6-16MB-TCP", "uart_port": "1",
        "uart_baudrate": "115200", "uart_tx_pin": "7", "uart_rx_pin": "6",
        "uart_cts_pin": "-1", "uart_rts_pin": "-1",
    }
    if any(values.get(key) != value for key, value in expected_uart.items()):
        fail("generated factory NVS UART mapping disagrees")

    for relative in (
        "factory/factory_ESP32C6-16MB-TCP.bin",
        "factory/factory_ESP32C6-16MB-TCP_unfilled.bin",
    ):
        if not (build / relative).is_file():
            fail(f"missing combined factory image: {relative}")
    if (build / "factory/factory_ESP32C6-16MB-TCP.bin").stat().st_size != FLASH_SIZE:
        fail("filled factory image is not exactly 16 MiB")
    factory = (build / "factory/factory_ESP32C6-16MB-TCP.bin").read_bytes()
    occupied = bytearray(FLASH_SIZE)
    for offset, relative in EXPECTED_FILES.items():
        payload = (build / relative).read_bytes()
        if factory[offset:offset + len(payload)] != payload:
            fail(f"filled factory image disagrees with {relative} at {offset:#x}")
        occupied[offset:offset + len(payload)] = b"\x01" * len(payload)
    if any(value != 0xFF for value, used in zip(factory, occupied) if not used):
        fail("filled factory image contains non-0xFF bytes outside generated input regions")
    if set((build / "ota_data_initial.bin").read_bytes()) != {0xFF}:
        fail("initial OTA metadata is not fully erased")
    ota0 = next(item for item in generated if item[0] == "ota_0")
    if actual_files.get(ota0[3]) != "esp-at.bin":
        fail("generated flash map does not initialize OTA0 with esp-at.bin")

    for offset, relative in sorted(EXPECTED_FILES.items()):
        path = build / relative
        print(f"{offset:#08x} {path.stat().st_size:8d} {sha256(path)} {relative}")
    for name, kind, subtype, offset, length in generated:
        print(f"partition {name:12s} {kind}/{subtype} offset={offset:#08x} size={length:#08x} ({length}) end={offset + length:#09x}")
    print("PASS: generated ESP-AT build, decoded partitions, OTA initialization, factory composition, UART mapping, and artifacts are coherent")


if __name__ == "__main__":
    main()
