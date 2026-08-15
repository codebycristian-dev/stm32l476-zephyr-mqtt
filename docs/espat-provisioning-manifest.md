# ESP32-C6 ESP-AT preparation manifest

Authorization status: NOT AUTHORIZED. This is review-only text; no erase or
write operation has been executed. Any identity, backup digest, firmware hash,
artifact, offset, or command change invalidates a future authorization.

## Target and strategy

- USB bridge: WCH `1a86:55d3`, serial `5B14063285`; stable by-id path resolved fresh
- Chip: ESP32-C6 revision 0.2 (must be freshly reconfirmed)
- Physical flash: 16 MB / `0x1000000` bytes (must be freshly reconfirmed)
- Strategy: custom local build; official release compatibility is not established
- ESP-AT: tag `v4.1.1.0`, commit `7c092f9aee793d6dbfae31e7585637baa14fb4ce`
- ESP-IDF: branch `release/v5.4`, commit `8ad0d3d8f2faab752635bee36070313c47c07a13`, as pinned by this ESP-AT module configuration
- Source: `https://github.com/espressif/esp-at`, recursively checked out under ignored `private/espat-source/`

Espressif's release matrix labels the available C6 artifact
`ESP32-C6-4MB-AT-V4.1.1.0.zip`, specifies 4 MB flash, and supports only the
listed MINI-1/WROOM-01 hardware by default. A detected 16 MB capacity does not
prove that image or its partition layout is compatible with this board.

## Required custom configuration

- Target: ESP32-C6, physical/declared flash size 16 MB
- Flash: DIO, 80 MHz, 16 MB
- AT port: UART1; GPIO6 RX, GPIO7 TX; 115200 baud; 8N1; no RTS/CTS
- Scope: base, Wi-Fi, and TCP/IP commands; MQTT is disabled/not depended upon
- UART framing/flow: 115200, 8 data bits, no parity, 1 stop bit, no hardware flow control; factory CTS/RTS values are `-1`
- Partition layout: `firmware/espat-16mb/partitions_at.csv`; OTA metadata `0xd000..0xf000`, PHY `0xf000..0x10000`, NVS `0x10000..0x1e000`, AT customization `0x1e000..0x60000`, OTA0 `0x60000..0x830000`, OTA1 `0x830000..0x1000000`; ranges are non-overlapping and bounded by 16 MB
- Build commands: `scripts/prepare-espat-source.sh`, then from the pinned source with its installed environment, `./build.py build`

Firmware artifacts are not yet prepared because the pinned ESP-IDF checkout did not complete. Before authorization, record exact
offsets, byte sizes, and SHA-256 for `bootloader.bin`, `partition-table.bin`,
`ota_data_initial.bin`, `at_customize.bin`, `mfg_nvs.bin`, and `esp-at.bin`, and
validate all ranges. No placeholder hash is acceptable.

## Backup evidence

- Convention: `private/espat-backups/esp32c6-<UTC>-full-16MiB.bin`
- Required read: `python -m esptool --chip esp32c6 --port <matched-by-id> read-flash 0x000000 0x1000000 <backup>`
- Path: `private/espat-backups/esp32c6-20260815T035358Z-full-16MiB.bin`
- Size: 16,777,216 bytes
- SHA-256: `26ab342b257fe9ab5146beaa61a5b7d277457cdf49f1a899d7cdb799b22e2695`
- esptool: 5.3.1

## Proposed destructive workflow — do not execute

Only after all pending values are replaced and fresh explicit authorization is
given for the exact device, backup digest, manifest digest, and commands:

```text
python -m esptool --chip esp32c6 --port <freshly-matched-by-id> erase-flash
python -m esptool --chip esp32c6 --port <freshly-matched-by-id> --baud 460800 write-flash --flash-mode dio --flash-freq 80m --flash-size 16MB 0x0 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0xd000 build/ota_data_initial.bin 0x1e000 build/at_customize.bin 0x1f000 build/customized_partitions/mfg_nvs.bin 0x60000 build/esp-at.bin
```

Expected credential-free verification: reboot once; capture bounded UART0 boot
output; on UART1 send `AT\r\n` and `AT+GMR\r\n` at 115200 8N1 without flow
control; prove host TX to GPIO6 and GPIO7 to host RX. Do not join Wi-Fi, send
credentials, use MQTT, or automatically restore. Recovery would require a
separate authorization to write the complete verified backup and is not risk-free.
