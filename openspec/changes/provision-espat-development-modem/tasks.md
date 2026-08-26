## 1. Read-Only Identification and Evidence

- [x] 1.1 Implement a fail-closed device resolver that uniquely matches WCH VID:PID `1a86:55d3` and serial `5B14063285`, returns the current path, and is invoked before every device operation.
- [x] 1.2 Document the board-specific UART0 wiring, electrical assumptions, reset/ROM-loader procedure, capture settings, and rules that keep discovery non-destructive.
- [x] 1.3 Capture and preserve raw UART0 boot/log output after fresh identification, recording timing, settings, and any evidence-backed firmware/build identity or explicitly recording it as unknown.
- [x] 1.4 In ROM-loader mode, re-identify the USB device and use read-only esptool queries to record ESP32-C6 revision, MAC where reported, physical flash identity/size, and esptool version.

## 2. Full-Flash Backup

- [x] 2.1 Add and review a read-only backup workflow that re-identifies the target and reads address `0x0` for exactly `0x1000000` bytes without erase or write commands.
- [x] 2.2 Run the approved read-only workflow to preserve the complete 16 MB flash backup and its command log in the documented evidence location.
- [x] 2.3 Record the backup byte count and SHA-256, verify exact size 16,777,216 bytes and independent readability, and recheck the digest from the preserved file.
- [x] 2.4 Add a fail-closed prerequisite check that prevents preparation of an executable flash operation when target identity or backup validation is missing or inconsistent.

## 3. Supported Firmware Preparation

- [x] 3.1 Research authoritative ESP-AT support for ESP32-C6 revision 0.2 with 16 MB flash and record whether the released 4 MB image is affirmatively compatible; do not infer compatibility from flash detection alone.
- [x] 3.2 Pin the selected strategy: either a compatibility-proven official release or a specific ESP-AT source tag/commit and compatible ESP-IDF version for a customized 16 MB build.
- [x] 3.3 Configure/build the selected firmware for UART1 GPIO6 RX, GPIO7 TX, 115200 8N1, no hardware flow control, and Wi-Fi/TCP-only use without relying on ESP-AT MQTT.
- [x] 3.4 Validate that the partition table and all offset/artifact pairs are non-overlapping and contained within 16 MB, then hash every firmware artifact.
- [x] 3.5 Create the reproducibility manifest containing tool and source versions, commits, configuration, partition layout, UART mapping, flash parameters, exact build commands, firmware hashes, and exact proposed erase/write workflow.
- [x] 3.6 Add credential-free post-provisioning checks for UART0 boot sanity, `AT`, `AT+GMR`, and bidirectional UART1 mapping, plus tests proving device mismatch and missing authorization fail closed.

## 4. Final Destructive Authorization Gate and Provisioning

- [x] 4.1 Present the freshly matched device identity, verified backup path/digest, firmware manifest/digest, exact erase/write commands, risks, and recovery limits, then stop and obtain explicit user authorization for this exact destructive operation; any changed input invalidates authorization.
- [ ] 4.2 Only after task 4.1 receives fresh explicit authorization, revalidate every bound input, execute only the authorized ESP32-C6 erase/write workflow, retain its complete log, reboot, and run the credential-free boot, `AT`, `AT+GMR`, and UART mapping verification without modifying STM32 USART1 or exercising Wi-Fi credentials or MQTT.
