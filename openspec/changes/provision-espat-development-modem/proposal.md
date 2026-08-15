## Why

The nanoESP32-C6 board currently has unidentified firmware and does not answer on the expected ESP-AT UART1 interface. It must be backed up and provisioned reproducibly without assuming that Espressif's released 4 MB ESP32-C6 image is compatible with the board's physical 16 MB flash.

## What Changes

- Add a guarded workflow that positively identifies the ESP32-C6/WCH device before every device operation.
- Capture UART0 boot output and preserve a verified, complete 16 MB flash backup with size and SHA-256 evidence before provisioning.
- Establish and document a supported ESP-AT firmware strategy for the 16 MB board, using an official release only when compatibility is demonstrated or a reproducible customized build otherwise.
- Define the modem interface as UART1 on GPIO6 RX and GPIO7 TX at 115200 8N1 without hardware flow control, for Wi-Fi and TCP transport only.
- Record firmware provenance, configuration, partitioning, flash parameters, hashes, and exact commands, then verify boot logs, `AT`, `AT+GMR`, and the UART mapping.
- Require separate, explicit user authorization immediately before any ESP32-C6 erase or write operation; proposal and preparation remain read-only.

## Capabilities

### New Capabilities

- `espat-modem-provisioning`: Safe identification, backup, firmware selection/build, authorization-gated provisioning, provenance recording, and post-flash verification for the nanoESP32-C6 development modem.

### Modified Capabilities

None.

## Impact

This change adds operational documentation, scripts or configuration needed to prepare and verify the ESP32-C6 modem and its evidence artifacts. It may introduce pinned ESP-AT/ESP-IDF build dependencies, but does not modify the STM32 USART1 implementation, implement Wi-Fi/TCP behavior, use ESP-AT MQTT, implement application MQTT, or transmit credentials. The only hardware mutation is the separately authorized final ESP32-C6 flash operation.
