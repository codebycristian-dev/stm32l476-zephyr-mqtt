## ADDED Requirements

### Requirement: Exact modem device and interface discovery
The qualification tooling SHALL identify the physical ESP32-C6 exposed to Linux using available USB and system metadata, enumerate every associated USB/serial interface, record stable identifiers when available, and select the AT communication serial device without relying solely on a transient `/dev/tty*` name.

#### Scenario: One unambiguous modem is attached
- **WHEN** discovery finds one ESP32-C6 device with sufficient interface metadata
- **THEN** the evidence records the device identity, USB topology, all interfaces, stable serial links if present, and the selected AT serial endpoint

#### Scenario: AT endpoint is ambiguous
- **WHEN** discovery cannot distinguish the AT endpoint from multiple candidates
- **THEN** the workflow reports the candidates and requires explicit safe selection or ends inconclusively without probing unrelated devices

### Requirement: Non-destructive firmware and serial qualification
The qualification workflow SHALL determine whether the selected endpoint responds as ESP-AT and SHALL determine and record the working baud rate, 8 data bits, parity, stop bits, flow control, and required command line termination using bounded harmless probes. It SHALL NOT flash, erase, run `AT+RESTORE`, or install packages.

#### Scenario: ESP-AT responds to a probe
- **WHEN** a bounded candidate serial configuration and line termination produce a valid `AT` response
- **THEN** the workflow records the confirmed endpoint and complete UART configuration and proceeds without changing modem firmware

#### Scenario: No probe succeeds
- **WHEN** no allowed candidate configuration produces a valid ESP-AT response
- **THEN** the workflow reports ESP-AT status and serial configuration as inconclusive and does not flash, erase, restore, or install software

### Requirement: Baseline ESP-AT command verification
The qualification workflow SHALL issue `AT`, `AT+GMR`, `AT+CWMODE?`, and `AT+CIPSTATUS` using the confirmed configuration and SHALL record redacted responses and individual outcomes.

#### Scenario: Baseline commands complete
- **WHEN** the selected endpoint is responsive
- **THEN** evidence contains the outcome and response for each required baseline command, including error or unsupported responses rather than omitting them

### Requirement: Version and build evidence
The qualification record SHALL capture the reported ESP-AT version, ESP-IDF version, chip/build information, selected serial interface, and confirmed UART parameters, and SHALL distinguish unavailable fields from successful observations.

#### Scenario: Identity information is reported
- **WHEN** `AT+GMR` and discovery responses contain version or build fields
- **THEN** the record preserves each field with its source command and does not infer missing values

### Requirement: Wi-Fi station capability verification
The workflow SHALL verify that station mode is supported without requiring credentials. When credentials are supplied through ephemeral, non-echoing input, it SHALL verify station mode, access-point association, and IP acquisition and SHALL record redacted outcomes. Wi-Fi association and IP acquisition SHALL pass for full qualification unless the user explicitly accepts a partial qualification.

#### Scenario: No credentials are supplied
- **WHEN** qualification runs without Wi-Fi credentials
- **THEN** station capability is checked and association and IP acquisition are marked `not-run`

#### Scenario: Valid credentials are supplied interactively
- **WHEN** the operator supplies credentials through the supported ephemeral input and the access point is reachable
- **THEN** the modem enters station mode, associates with the access point, acquires an IP address, and the evidence records success without revealing the credentials

#### Scenario: Association fails
- **WHEN** credential-dependent association or IP acquisition does not complete
- **THEN** the workflow records a redacted failure category and preserves enough non-secret diagnostics to distinguish association from IP failure

### Requirement: Credentials remain untracked and undisclosed
Qualification tooling SHALL accept Wi-Fi credentials only through ephemeral, non-echoing input and SHALL NOT store them in repository files, retained evidence, command-line arguments, or unredacted logs. Retained artifacts SHALL be reviewed for accidental secret inclusion.

#### Scenario: Credential-bearing command is transmitted
- **WHEN** the tool constructs or sends a Wi-Fi join command
- **THEN** operator-visible and persisted output replaces the SSID and passphrase with redacted markers and ephemeral credential storage is cleared at process exit

### Requirement: Basic TCP client verification
After successful network setup, the workflow SHALL verify a plain TCP client connection, data send, data receipt, and connection close independently of MQTT and SHALL NOT use ESP-AT MQTT commands. All four TCP stages SHALL pass for full qualification unless the user explicitly accepts a partial qualification.

#### Scenario: TCP echo-style exchange succeeds
- **WHEN** a configured reachable TCP test endpoint and payload are provided after IP acquisition
- **THEN** the modem connects, sends the payload, receives a verifiable response, closes the connection, and evidence records each stage

#### Scenario: TCP endpoint is unavailable
- **WHEN** the remote endpoint, DNS, or network prevents the exchange
- **THEN** the result identifies the failed stage and does not claim that MQTT or the STM32 transport was tested

### Requirement: Portable ESP-AT command subset
The qualification output SHALL define the smallest command and response subset supported by evidence for initialization/health, station configuration and join, network status, one TCP connection, send, receive, and close. Qualification-only identity commands SHALL be labeled separately from commands required by the future runtime modem engine.

#### Scenario: Subset is derived from completed evidence
- **WHEN** command qualification is reviewed
- **THEN** every runtime command maps to at least one logical modem operation and unnecessary or ESP32-C6-only commands are excluded

### Requirement: Generic and device-specific behavior classification
The qualification record SHALL classify observed commands, final result codes, unsolicited result codes, prompts, and data framing as generic ESP-AT behavior or ESP32-C6-specific/uncertain behavior, with the basis for each classification.

#### Scenario: Chip-specific observation occurs
- **WHEN** a response contains ESP32-C6 chip, build, USB, or other device-specific information
- **THEN** it is labeled as qualification evidence and excluded from the portable runtime contract unless later compatibility evidence justifies it

### Requirement: Future STM32 transport requirements
The change SHALL produce testable requirements for a later STM32 USART1 transport covering confirmed serial framing and line termination, command/response and prompt handling, unsolicited receive handling, buffering, timeouts, retries, error propagation, and observed size limits. It SHALL require a modem-neutral API containing `modem_init`, `modem_join_ap`, `modem_tcp_connect`, `modem_send`, `modem_receive`, and `modem_close`, without implementing USART1.

#### Scenario: Transport requirements are derived
- **WHEN** modem qualification evidence is complete or explicitly marks unavailable observations
- **THEN** a future USART1 implementer can trace each transport requirement to measured evidence or a clearly labeled unresolved bound

#### Scenario: MQTT consumes modem services later
- **WHEN** a future MQTT implementation is designed
- **THEN** its modem dependency is expressed through logical modem operations and exposes no ESP32-C6-specific abstraction

### Requirement: Qualification result integrity
The workflow SHALL retain only the minimum evidence needed to support the result, SHALL report each stage as pass, fail, inconclusive, or not-run, and SHALL not claim verification for stages that were not executed. The OpenSpec change SHALL remain active until Wi-Fi association, IP acquisition, and TCP connect/send/receive/close pass or the user explicitly accepts partial qualification.

#### Scenario: Partial qualification is saved
- **WHEN** qualification stops before all required stages complete
- **THEN** completed evidence remains usable, every omitted stage is explicitly marked inconclusive or not-run, and the change remains active unless the user explicitly accepts the partial result

#### Scenario: Full qualification is ready to archive
- **WHEN** baseline commands, Wi-Fi association, IP acquisition, and TCP connect/send/receive/close have passed
- **THEN** the result is fully qualified and may proceed to OpenSpec verification and archival readiness

### Requirement: Bounded STM32 UART1 diagnostic
The qualification firmware SHALL reuse the verified direct-CMSIS USART1 transport, send exactly `AT\r\n` as its first runtime modem command at 115200 8N1 with no flow control, and send exactly `AT+GMR\r\n` only after the first command completes with a final `OK` and no overflow. Each response SHALL use fixed bounded storage and a bounded deadline, preserve raw/printable evidence and exact TX/RX counts, classify `OK`, error, echo, unsolicited lines, prompts, overflow, and timeout, report USART1 parity, framing, noise, overrun, and ring-overflow counters through USART2/ST-LINK, and extract reported ESP-AT and ESP-IDF identity from `AT+GMR` when available. It SHALL NOT use Zephyr UART, STM32 HAL/LL, dynamic allocation, network commands, or MQTT.

#### Scenario: Diagnostic is prepared but not flashed
- **WHEN** host tests, target build, transport review, and static validation pass
- **THEN** the firmware is ready for the explicit wiring and flash step but no runtime modem qualification task is marked complete

#### Scenario: Initial health command does not pass
- **WHEN** `AT\r\n` ends in error, timeout, or overflow rather than final `OK`
- **THEN** the diagnostic reports bounded evidence and counters and does not transmit `AT+GMR\r\n`
