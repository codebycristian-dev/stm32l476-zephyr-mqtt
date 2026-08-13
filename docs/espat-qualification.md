# ESP-AT development-modem qualification

`scripts/qualify_espat.py` is the executable source of truth. It uses only the
Python standard library and never installs software, flashes or erases a modem,
runs `AT+RESTORE`, accesses a serial endpoint that was not first correlated to
an Espressif USB device, or implements STM32 USART1 or MQTT.

## Current hardware evidence and blocker

The attached target is a verified ESP32-C6 revision 0.2 with a reported 16 MB
flash. Esptool communicates through the WCH `1a86:55d3` interface exposed as
`/dev/ttyACM0`, confirming that path as usable for download/log access. It does
not establish that `/dev/ttyACM0` is an ESP-AT command endpoint.

Official ESP-AT uses UART0 for download/log and UART1 for AT commands on this
target: GPIO6 is UART1 RX and GPIO7 is UART1 TX. Both pins are physically
accessible, but no external USB-UART adapter is currently available. Therefore
the harmless probes previously sent through `/dev/ttyACM0` are inconclusive
about ESP-AT presence; they do not show that ESP-AT is absent or unresponsive.

Do not repeat baseline qualification against `/dev/ttyACM0` as though it were
the AT endpoint. Qualification is blocked until an external UART host can be
connected to GPIO6/GPIO7. The planned STM32L476RG USART1 transport may serve as
that host in its subsequent, separately authorized hardware stage. This change
remains active, and no remaining qualification task is complete on this basis.

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

## Future STM32 USART1 requirements

A later, separately authorized transport shall:

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

No USART1 pins, devicetree, Kconfig, driver, wiring, or MQTT behavior is part of
this qualification.
