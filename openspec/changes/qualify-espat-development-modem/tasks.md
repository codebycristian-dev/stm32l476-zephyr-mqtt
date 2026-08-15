## 1. Minimal Host Qualification Tool

- [x] 1.1 Inspect repository conventions and select paths for one small qualification tool, its minimal tests, the readiness skill, and concise non-secret results.
- [x] 1.2 Implement safe Linux discovery of the physical ESP32-C6, its USB/serial interfaces, stable identifiers, permissions, and unambiguous AT endpoint selection.
- [x] 1.3 Implement bounded harmless `AT` probing to detect existing ESP-AT and confirm baud rate, 8 data bits, parity, stop bits, flow control, and line termination.
- [x] 1.4 Implement serial command parsing for echo, prompts, final results, unsolicited responses, received data, and timeouts needed by the required checks.
- [x] 1.5 Implement `AT`, `AT+GMR`, `AT+CWMODE?`, and `AT+CIPSTATUS` checks and concise capture of versions, chip/build information, endpoint, and UART settings.
- [x] 1.6 Implement ephemeral, non-echoing Wi-Fi credential input and redaction that keeps credentials out of arguments, files, logs, and retained evidence.
- [x] 1.7 Implement station association, IP acquisition, and plain TCP connect/send/receive/close checks without MQTT or ESP-AT MQTT commands.
- [x] 1.8 Add minimal host-side tests for response parsing and credential redaction, including failure cases.

## 2. Hardware Qualification

- [x] 2.1 Run discovery and record the exact attached ESP32-C6, enumerated interfaces, and safely selected Linux AT serial endpoint.
- [x] 2.2 Run the non-destructive probe and record whether ESP-AT is installed and the confirmed serial configuration.
- [ ] 2.3 Run the four baseline commands and retain only the required version, build, endpoint, UART, response, and outcome evidence.
- [ ] 2.4 With credentials supplied interactively, verify station mode, AP association, and IP acquisition without retaining secrets.
- [ ] 2.5 Run and record a successful plain TCP connect/send/receive/close exchange; if credentials are unavailable, record a partial result and keep the change active unless the user explicitly accepts it.

## 3. Portable Handoff

- [ ] 3.1 Derive the smallest evidence-backed ESP-AT runtime command subset and distinguish generic behavior from ESP32-C6-specific or uncertain observations.
- [ ] 3.2 Document the verified STM32 USART1 transport baseline plus future modem-engine requirements for line termination, prompts, responses, unsolicited data, buffering, timeouts, retries, errors, and observed limits.
- [ ] 3.3 Preserve `modem_init`, `modem_join_ap`, `modem_tcp_connect`, `modem_send`, `modem_receive`, and `modem_close` as the modem-neutral future API, with no ESP32-C6 detail exposed to MQTT.
- [ ] 3.4 Create the small project-local `espat-development-modem-readiness` skill, referencing the repository tool as source of truth and duplicating no protocol implementation.
- [ ] 3.5 Run minimal tool tests, secret review, skill validation, and strict OpenSpec validation; declare archival readiness only after full Wi-Fi/TCP qualification or explicit user acceptance of a partial result.

## 4. STM32 UART1 Diagnostic Host

- [x] 4.1 Record that the verified direct-CMSIS USART1 transport resolves the external UART-host blocker and document the GPIO6/GPIO7 wiring.
- [x] 4.2 Add the bounded single-command `AT\r\n` diagnostic without redesigning USART1 or adding network/MQTT behavior.
- [x] 4.3 Add fixed-storage response classification for OK, error, echo, unsolicited lines, prompts, overflow, and timeout with host tests.
- [x] 4.4 Run host tests, a pristine nucleo_l476rg build, USART review, strict OpenSpec validation, and git diff checking without flashing.
