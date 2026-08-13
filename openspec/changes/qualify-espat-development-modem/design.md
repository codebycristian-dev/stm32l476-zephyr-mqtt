## Context

The STM32L476RG application foundation is complete, but neither USART1 nor MQTT exists. An ESP32-C6 is available in place of the originally intended ESP8266, so it must first be qualified independently on Linux as an ESP-AT modem. The resulting contract must remain portable across compatible ESP-AT devices and must not leak credentials or require an unapproved firmware mutation.

Qualification covers Linux USB discovery, serial command exchange, Wi-Fi association, TCP testing, concise evidence, future transport requirements, and a project-local operator skill. Hardware identity, installed firmware, port topology, serial settings, network access, and exact responses are runtime observations rather than facts to assume in planning.

## Goals / Non-Goals

**Goals:**

- Produce repeatable, hardware-verifiable evidence identifying the ESP32-C6, its USB interfaces, its AT serial endpoint, installed ESP-AT and ESP-IDF versions, chip/build information, and working UART framing and line termination.
- Verify baseline AT commands, Wi-Fi association, IP acquisition, and a basic non-MQTT TCP client exchange before declaring full qualification.
- Define a minimal portable ESP-AT command/response contract and classify deviations or observations specific to the ESP32-C6.
- Translate host qualification observations into requirements for a future STM32 USART1 transport and logical modem API.
- Provide a repository-local readiness skill backed by the repository qualification tool, with secrets accepted only ephemerally.

**Non-Goals:**

- Implement, configure, wire, or test STM32 USART1.
- Implement MQTT or use ESP-AT MQTT commands.
- Flash, erase, restore, or otherwise replace modem firmware without separate explicit authorization.
- Install host packages without separate explicit authorization.
- Guarantee compatibility with an unavailable ESP8266; this change defines a conservative subset to qualify later.
- Modify the verified STM32 foundation or `/home/cristian/zephyrproject`.

## Decisions

### Use staged, non-destructive qualification

Qualification will proceed through discovery, passive serial probing, baseline commands, Wi-Fi checks, and TCP checks. Credentials may be deferred for a partial result, but Wi-Fi/TCP checks are required for full qualification unless the user explicitly accepts the partial result. Every selected device and configuration will be shown in evidence, and mutation commands such as `AT+RESTORE` and firmware tools will not be part of the workflow.

Alternative: flash a known ESP-AT image first. Rejected because it destroys evidence about the installed state, introduces board/partition risk, and requires authorization that this change does not grant.

### Use one small repository tool as the executable source of truth

One small host-side qualification tool will own discovery, serial transactions, redaction, qualification sequencing, and concise result output. Minimal host-side tests will cover parser and redaction behavior. The project-local skill will invoke and explain the tool rather than duplicate protocol logic in prose. The tool will check prerequisites and fail with actionable messages; it will not install them.

Alternative: document ad hoc terminal commands only. Rejected because timing, line termination, port selection, response capture, and redaction would be difficult to repeat and audit.

### Separate public qualification from secret-dependent checks

Baseline qualification will run without credentials. Association checks will accept SSID and passphrase only from a non-echoing interactive prompt, keep them out of command arguments and files, redact command transcripts, and never write raw credentials to retained evidence. TCP test endpoints and payloads will be non-secret configuration.

Alternative: use a checked-in local configuration template populated by the operator. Rejected because it increases accidental secret-commit risk.

### Retain only minimum qualification evidence

The workflow will record only the device and endpoint identity, confirmed UART settings, required command results and version/build fields, Wi-Fi/TCP outcomes, generic-versus-device-specific findings, and information needed for the future command subset and USART1 requirements. A partial result may mark credential-dependent checks `not-run`, but full qualification and archival require Wi-Fi association, IP acquisition, and TCP connect/send/receive/close to pass unless the user explicitly accepts the partial result.

### Define portability at the logical-operation boundary

The future STM32 stack will expose `modem_init`, `modem_join_ap`, `modem_tcp_connect`, `modem_send`, `modem_receive`, and `modem_close`. The MQTT layer will consume these operations and never ESP32-C6 identifiers, USB details, or chip-specific commands. The initial command subset will be limited to commands actually needed and observed for reset-free initialization/health, station operation, IP/status inspection, one TCP connection, send, receive indication/data handling, and close. Identity commands used only for qualification need not become runtime dependencies.

Alternative: expose raw AT commands directly to MQTT. Rejected because it couples protocol logic to modem syntax and prevents replacement or targeted qualification.

### Derive USART1 requirements without implementing the transport

The evidence will state baud, data bits, parity, stop bits, flow control, line ending, response termination, unsolicited-result handling, receive buffering, command timeout/retry needs, and maximum observed or declared transaction sizes. These become acceptance inputs to a separate USART1 change; no devicetree, pin, Kconfig, driver, or application code changes are made here.

## Risks / Trade-offs

- [Multiple USB serial interfaces or unstable `/dev/tty*` names] → Correlate udev/sysfs metadata and stable `/dev/serial/by-id` links, enumerate all candidates, and require an unambiguous selected endpoint.
- [Unknown installed firmware or baud rate] → Probe a bounded, documented set of common configurations using harmless `AT` requests and report inconclusive results without flashing.
- [Serial probing disrupts another attached device] → Filter by discovered physical USB identity, display the target, detect busy ports, and require explicit device selection when ambiguous.
- [Credentials leak through process arguments, logs, shell history, or evidence] → Use non-echoing/ephemeral input, redact transmitted join commands and responses, scan generated evidence, and document cleanup behavior.
- [TCP test depends on an external endpoint] → Make endpoint selection explicit, record it, use a deterministic plain TCP exchange, and distinguish modem capability failure from DNS/network/remote-endpoint failure.
- [ESP32-C6 behavior is mistaken for portable ESP-AT behavior] → Label every command/response classification with evidence and keep chip/build identity outside the runtime abstraction.
- [A minimal subset is still version-dependent] → Record exact ESP-AT/ESP-IDF versions and leave other modem families unqualified until the same suite passes.

## Migration Plan

This is an additive qualification change. Implement the single tool, minimal tests, and skill; run the baseline stages; then complete Wi-Fi and TCP checks with credentials supplied interactively. Review the minimum evidence and validate the OpenSpec change. If credentials are temporarily unavailable, retain a partial result and keep the change active unless the user explicitly accepts partial qualification. Rollback consists of removing the new qualification assets; no target firmware or existing STM32 behavior is migrated.

## Open Questions

- What exact ESP32-C6 board, USB bridge/topology, serial endpoint, firmware version, and UART configuration will discovery establish?
- Are credentials and a suitable access point available during implementation for association/IP checks?
- Which deterministic TCP endpoint is reachable from that network, or should the operator provide a local test server?
- What maximum response bursts, unsolicited messages, and send sizes will be observed and therefore carried into the later USART1 acceptance criteria?
