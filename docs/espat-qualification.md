# ESP-AT development-modem qualification

`scripts/qualify_espat.py` is the executable source of truth. It uses only the
Python standard library and never installs software, flashes or erases a modem,
runs `AT+RESTORE`, accesses a serial endpoint that was not first correlated to
an Espressif USB device, or implements STM32 USART1 or MQTT.

## Current hardware evidence and STM32 diagnostic host

The attached target is a verified ESP32-C6 revision 0.2 with a reported 16 MB
flash. Esptool communicates through the WCH `1a86:55d3` interface exposed as
`/dev/ttyACM0`, confirming that path as usable for download/log access. It does
not establish that `/dev/ttyACM0` is an ESP-AT command endpoint.

Official ESP-AT uses UART0 for download/log and UART1 for AT commands on this
target: GPIO6 is UART1 RX and GPIO7 is UART1 TX. Both pins are physically
accessible. Therefore
the harmless probes previously sent through `/dev/ttyACM0` are inconclusive
about ESP-AT presence; they do not show that ESP-AT is absent or unresponsive.

Do not repeat baseline qualification against `/dev/ttyACM0` as though it were
the AT endpoint. The UART-host blocker is resolved: the STM32L476RG direct-CMSIS
USART1 transport has been physically verified at 115200 8N1 (69/69 loopback
bytes exact, all error counters zero), with PA9 TX, PA10 RX, polling transmit,
interrupt-driven receive, and a static 64-byte receive ring. USART2 remains the
ST-LINK diagnostic console.

The post-provision diagnostic sends `AT\r\n` first, then sends `AT+GMR\r\n`
only after a final `OK` without overflow. It retains fixed 512-byte transaction
snapshots, classifies command echo, `OK`, `ERROR`/`FAIL`, prompts, other
unsolicited lines, overflow, and timeout, and emits one bounded record through
USART2/ST-LINK. It performs no Wi-Fi, TCP, MQTT, or modem mutation.

### Initial STM32 UART1 probe evidence (2026-08-14)

The NUCLEO-L476RG was positively selected through ST-LINK serial
`066EFF515250898367012013`; its USART2 console was opened only through
`/dev/serial/by-id/usb-STMicroelectronics_STM32_STLink_066EFF515250898367012013-if02`.
The WCH `1a86:55d3` device, serial `5B14063285`, was excluded.

With PA9 connected to GPIO6, PA10 connected to GPIO7, and common ground, the
single diagnostic run transmitted exactly 4 bytes (`41 54 0D 0A`, `AT\r\n`).
It received 0 bytes in the bounded 1000 ms window: raw hex and printable
responses were both empty. Classification was `TIMEOUT`; echo, `OK`, error,
prompt, unsolicited-line, and overflow observations were all zero. USART1
parity, framing, noise, overrun, and ring-overflow counters were all zero.

This is an inconclusive initial probe. It establishes only that no valid ESP-AT
response was observed at 115200 8N1 on the tested default UART1 mapping. It is
not evidence that ESP-AT is absent, and no alternate baud, mapping, command, or
firmware was tried.

### Post-provision STM32 UART1 diagnostic evidence (2026-08-28)

The separately authorized NUCLEO-L476RG run used ST-LINK serial
`066EFF515250898367012013` (firmware `V2J30M19`, board `NUCLEO-L476RG`, device
`STM32L476`, ID `0x415`). It was flashed exactly once; approximately 27.56 KiB
was programmed and application start passed. The ESP32-C6 WCH `1a86:55d3`
device (serial `5B14063285`) was excluded from flashing and mutation.

With PA9 to GPIO6, PA10 from GPIO7, and common ground, the record started as
`ESPAT_EVIDENCE_BEGIN v=1 capacity=512` and ended with the observed
`ESPAT_EVIDENCE_END` marker. `AT` completed once: TX 4, RX 11, final `OK`, no
timeout, TX error, overflow, truncation, prompt, or USART/ring error. Its raw
bytes were `00 41 54 0D 0A 0D 0A 4F 4B 0D 0A` and printable form
`.AT....OK..`.

`AT+GMR` then completed once: TX 8, RX 198, final `OK`, one echo, four
unsolicited lines, and no timeout, TX error, overflow, truncation, prompt, or
USART/ring error. The bounded response reported `AT version:4.1.1.0(ba4dd0e -
ESP32C6 - Jul 31 2025 08:37:48)`, `SDK version:v5.4.1-643-g8ad0d3d8f2f-dirty`,
`compile time(7c092f9):Aug 26 2026 05:59:24`, and
`Bin version:v4.1.1.0(ESP32C6-4MB)`, followed by final `OK`. The complete raw
`AT+GMR` capture is intentionally not reproduced here; this retained summary is
traceable to the bounded transaction counts and observed fields.

No MPU fault, stack overflow, or Zephyr fatal error occurred. The prior
synchronous-evidence stack-overflow regression is physically resolved by this
run.

Run non-secret discovery first:

```sh
python3 scripts/qualify_espat.py discover
```

If discovery reports exactly one physical Espressif device and one serial
interface, run the harmless serial probe and baseline checks:

```sh
python3 scripts/qualify_espat.py baseline \
  --output docs/espat-qualification-result.json
```

If multiple serial interfaces are listed, stop. Select `--device` only after
the operator has established which listed interface is the AT endpoint; never
probe candidates by guesswork. The tool probes only documented baud rates at
8-N-1 with no flow control, using `AT` with CRLF or CR. It then runs `AT`,
`AT+GMR`, `AT+CWMODE?`, and `AT+CIPSTATUS`.

After ESP-AT is confirmed, run the secret-dependent stages in an interactive
terminal:

```sh
python3 scripts/qualify_espat.py full \
  --output docs/espat-qualification-result.json
```

The SSID and passphrase prompts do not echo. Never supply credentials through
arguments, environment variables, redirected input, tracked files, chat, or
logs. The TCP host, port, and payload must name a deliberately selected plain
echo service and are non-secret. The workflow uses no MQTT commands.

## Portable handoff contract

The future modem engine retains this modem-neutral API: `modem_init`,
`modem_join_ap`, `modem_tcp_connect`, `modem_send`, `modem_receive`, and
`modem_close`.

The smallest candidate ESP-AT runtime subset is `AT` for health,
`AT+CWMODE=1` for station mode, `AT+CWJAP` for association, `AT+CIFSR` and
`AT+CIPSTATUS` for network state, `AT+CIPSTART` for one TCP connection,
`AT+CIPSEND` plus its `>` prompt and length-delimited payload for send,
`+IPD` for received data, and `AT+CIPCLOSE` for close. `AT+GMR` is
qualification-only. This subset remains provisional until the corresponding
physical stages pass.

Generic ESP-AT behavior comprises command echo tolerance, `OK`/`ERROR` final
results, the send prompt, `SEND OK`/`SEND FAIL`, asynchronous status lines, and
`+IPD` receive framing. USB VID/PID, topology, serial strings, chip name,
version/build strings, exact baud and observed timing are device/build-specific
or uncertain observations and stay outside the logical API.

## STM32 USART1 transport and future modem-engine requirements

The verified transport supplies the fixed 115200 8N1, no-flow-control physical
path and bounded 64-byte interrupt receive ring. A later modem engine shall:

- configure the measured baud, 8 data bits, measured parity/stop bits/flow
  control, and measured line termination;
- accept echo on or off, parse prompts and final results separately, and route
  unsolicited lines and length-delimited `+IPD` data while a command is active;
- buffer fragmented lines and payloads across reads and handle multiple events
  in one receive burst without assuming USB-specific packet boundaries;
- use command-specific bounded timeouts (short health/status, longer join and
  connect), bounded retries only for safe/idempotent operations, and propagate
  timeout, modem, association, network, send, and close errors distinctly;
- size command, response, URC, and payload buffers from retained measured maxima.
  Until physical evidence supplies those bounds, treat them as unresolved and
  prevent silent truncation rather than inventing constants.

The diagnostic reuses the transport without Zephyr UART, STM32 HAL/LL, dynamic
allocation, Wi-Fi, TCP, or MQTT behavior.
