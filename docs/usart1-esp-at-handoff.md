# ESP-AT USART1 diagnostic wiring handoff

The direct-CMSIS USART1 transport is implemented and physically verified. The
separately authorized post-provision diagnostic passed with this crossed UART
wiring:

- `STM32 PA9 USART1_TX -> ESP32-C6 GPIO6 RX`
- `STM32 PA10 USART1_RX <- ESP32-C6 GPIO7 TX`
- `GND <-> GND`

The retained run confirms that the installed ESP-AT firmware responds on this
GPIO6/GPIO7 UART1 path. The diagnostic sends `AT\r\n` first and sends
`AT+GMR\r\n` only after a final `OK` without overflow. It retains fixed,
512-byte transaction snapshots and emits one bounded record through the
unchanged USART2/ST-LINK console; it performs no Wi-Fi, TCP, MQTT, or modem
mutation.

## Post-provision diagnostic evidence (2026-08-28)

The NUCLEO-L476RG was identified through ST-LINK serial
`066EFF515250898367012013` (firmware `V2J30M19`, board `NUCLEO-L476RG`, device
`STM32L476`, ID `0x415`). It was flashed once, approximately 27.56 KiB was
programmed, and application start passed. The ESP32-C6 WCH device
`1a86:55d3`, serial `5B14063285`, was excluded from flashing and mutation.

The bounded record began with `ESPAT_EVIDENCE_BEGIN v=1 capacity=512` and its
`ESPAT_EVIDENCE_END` marker was observed. `AT` transmitted 4 bytes and received
11 bytes (`00 41 54 0D 0A 0D 0A 4F 4B 0D 0A`; printable `.AT....OK..`), ending
`OK` with no timeout, TX error, overflow, truncation, prompt, or USART/ring
error. `AT+GMR` then transmitted 8 bytes and received 198 bytes, ending `OK`
with no timeout, TX error, overflow, truncation, prompt, or USART/ring error.
Its bounded response reported ESP-AT `4.1.1.0(ba4dd0e - ESP32C6 - Jul 31 2025
08:37:48)`, ESP-IDF `v5.4.1-643-g8ad0d3d8f2f-dirty`, compile time
`Aug 26 2026 05:59:24`, and binary version `v4.1.1.0(ESP32C6-4MB)`.

No MPU fault, stack overflow, or Zephyr fatal error occurred. This physical run
resolves the previous synchronous-evidence stack-overflow regression.
