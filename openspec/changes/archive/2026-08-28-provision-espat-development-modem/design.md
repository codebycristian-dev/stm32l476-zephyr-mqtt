## Context

The target is a nanoESP32-C6 development board containing an ESP32-C6 revision 0.2 and 16 MB physical SPI flash. Its WCH USB serial adapter was previously observed as VID:PID `1a86:55d3`, serial `5B14063285`, at `/dev/ttyACM0`; device paths are not stable and therefore are not identities. Existing firmware produces UART0 output but is unidentified. A verified STM32L476RG host received no bytes after sending `AT\r\n` to the official default ESP-AT UART1 mapping (GPIO6 RX, GPIO7 TX, 115200 8N1), while all host USART1 error counters remained zero. ESP-AT is therefore unconfirmed.

The released official ESP32-C6 ESP-AT binary uses a 4 MB configuration. Flashing it onto this 16 MB board without a supported configuration would risk an invalid or irreproducible installation. Read-only discovery and backup can proceed independently, but every erase or write requires explicit user authorization.

## Goals / Non-Goals

**Goals:**

- Re-identify the exact WCH/ESP32-C6 target before every device operation.
- Preserve non-destructive UART0 evidence and a validated full-flash backup before mutation.
- Select or produce a supported, reproducible ESP-AT configuration for the physical 16 MB flash.
- Provision a Wi-Fi/TCP-only AT modem on UART1 GPIO6/GPIO7 at 115200 8N1 without flow control.
- Make the irreversible phase conspicuous, bounded, and dependent on fresh explicit authorization.
- Preserve enough provenance and hashes to reproduce and audit the installed firmware.

**Non-Goals:**

- Modifying the STM32 USART1 implementation.
- Implementing STM32 Wi-Fi, TCP, or MQTT behavior.
- Using or enabling ESP-AT MQTT functionality.
- Supplying network credentials or testing authenticated network access.
- Erasing or writing ESP32-C6 flash while preparing or implementing the workflow before authorization.

## Decisions

### Treat hardware identity as a precondition for every operation

Each operation will rediscover the serial endpoint and require the WCH VID:PID and unique serial to match. Operations that enter the ROM bootloader will additionally use esptool to confirm ESP32-C6 identity, revision, and detected flash size where the command supports it. The workflow will stop on missing, duplicate, or mismatched devices rather than trusting `/dev/ttyACM0`.

Alternative considered: pinning the device path. This is rejected because ACM numbering can change and does not protect another attached device.

### Separate read-only acquisition from mutation

UART0 capture, chip/flash identification, full-range flash read, hashing, size checking, and backup readability checks form a non-destructive preparation phase. The flash workflow is a separate final phase whose first actionable step is an explicit user authorization gate naming the device, selected firmware manifest, backup, and exact erase/write command. No erase/write command will be run merely because prior preparation was approved.

Alternative considered: one end-to-end provisioning script. This makes accidental continuation into destructive operations too easy and is rejected.

### Preserve the entire physical flash before selecting firmware

The backup will read exactly `0x1000000` bytes from address `0x0`, then record SHA-256 and byte count. Verification will require exact 16 MB size, successful independent file readability, and a repeatable integrity check against the recorded digest. The original backup remains immutable; any analysis uses a copy or read-only tooling.

### Resolve 16 MB support before generating a flash plan

The preferred strategy is an official ESP-AT release only if authoritative release metadata or Espressif configuration support establishes compatibility with this ESP32-C6 revision and 16 MB flash layout. Otherwise, ESP-AT will be built from a pinned source tag/commit with its compatible ESP-IDF version and an explicit 16 MB flash/partition configuration. The resulting manifest must describe every flashed offset and artifact and demonstrate that all regions fit the physical flash without overlapping.

Alternative considered: flash the released 4 MB image and rely on automatic flash-size detection. This is rejected because detected capacity does not establish that the binary's partition table and configuration are supported for this board.

### Make the AT interface and feature boundary explicit

The selected configuration will expose ESP-AT on UART1 with module RX on GPIO6, module TX on GPIO7, 115200 baud, 8 data bits, no parity, one stop bit, and no hardware flow control. ESP-AT MQTT support will not be relied upon; where configuration permits, unused MQTT commands/components will be disabled. The intended modem responsibility ends at Wi-Fi association and TCP transport.

### Capture a reproducibility manifest before authorization

The manifest will pin ESP-AT version, ESP-IDF version, upstream source/tag and commit, tool versions, configuration inputs, partition table, UART mapping, flash mode/frequency/size, build commands, output hashes, and the exact proposed flash command/workflow. The reviewed manifest and backup digest identify what the user authorizes; changing either invalidates the authorization.

### Verify without credentials

After an authorized flash and reboot, verification will check sane UART0 boot logs, `AT` response, `AT+GMR` identity, and bidirectional operation on the specified UART1 pins/settings. It will not join a network, transmit credentials, or exercise MQTT.

## Risks / Trade-offs

- [Wrong serial device is targeted] → Match VID:PID and unique serial before every operation, require exactly one match, and confirm the ESP32-C6 through esptool where possible.
- [Bootloader entry changes enumeration] → Rediscover and revalidate identity after each mode transition instead of retaining a stale device path.
- [UART0 capture is electrically or logically incorrect] → Document voltage, grounding, pin direction, baud attempts, raw capture timing, and preserve raw output separately from interpretation.
- [Backup is truncated or corrupt] → Read the full `0x1000000` range, enforce exact byte size, test readability, and record/recheck SHA-256 before authorization.
- [A 4 MB release is incompatible with 16 MB hardware] → Require affirmative compatibility evidence or build a pinned 16 MB configuration; capacity detection alone is insufficient.
- [Custom firmware becomes difficult to reproduce] → Pin sources and tools, preserve configuration and partition files, record exact commands, and hash every flash artifact.
- [Flash interruption leaves the modem unbootable] → Retain the verified full backup and a documented, separately authorized recovery workflow; do not claim rollback is risk-free.
- [Authorization becomes stale] → Bind authorization to device identity, backup digest, firmware manifest digest, and exact write plan; any change returns to review.

## Migration Plan

1. Perform read-only identity and UART0 capture.
2. Enter the ROM loader without erasing, re-identify the chip, and acquire and validate the complete 16 MB backup.
3. Establish the supported ESP-AT strategy and produce the reproducibility and flash manifests without connecting to the target for a write.
4. Review all evidence and present the exact destructive operation as the final authorization gate.
5. Only after fresh explicit authorization, re-identify the device, revalidate the manifests and backup digest, execute only the authorized erase/write workflow, and retain its log.
6. Reboot and perform credential-free UART0 and UART1 verification.

Rollback consists of a separately reviewed and explicitly authorized restoration of the verified full-flash backup. Restoration is destructive and is never automatic.

## Open Questions

- Which official ESP-AT release, if any, explicitly supports ESP32-C6 revision 0.2 with a 16 MB flash configuration?
- If a custom build is required, which pinned ESP-AT tag/commit and compatible ESP-IDF version provide the smallest supported Wi-Fi/TCP-only configuration?
- What board-specific boot/reset procedure reliably enters and exits the ESP32-C6 ROM loader while preserving positive USB identity?
- Which UART0 pins and capture settings on this nanoESP32-C6 variant are confirmed by board documentation or observation?
