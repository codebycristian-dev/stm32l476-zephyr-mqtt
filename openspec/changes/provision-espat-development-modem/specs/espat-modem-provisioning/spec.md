## ADDED Requirements

### Requirement: Positive target identification
The provisioning workflow SHALL positively identify the target before every device operation by matching WCH USB VID:PID `1a86:55d3` and serial `5B14063285`, and SHALL reject missing, ambiguous, or mismatched devices. It SHALL additionally confirm an ESP32-C6 revision 0.2 and 16 MB detected flash through esptool for operations in ROM-loader mode where those properties are available.

#### Scenario: Expected device is uniquely present
- **WHEN** a device operation is requested and exactly one enumerated device matches the required USB identity
- **THEN** the workflow resolves its current device path and proceeds only after any operation-specific ESP32-C6 checks pass

#### Scenario: Device identity is unsafe
- **WHEN** the required device is absent, multiple devices match, or any required identity property differs
- **THEN** the workflow stops before communicating with or changing a device

### Requirement: Non-destructive UART0 evidence
The workflow SHALL capture and preserve raw UART0 boot or log output without erasing or writing ESP32-C6 flash and SHALL record capture settings, timing, and any firmware or build identity derived from the output.

#### Scenario: Existing firmware emits identifiable output
- **WHEN** the board is reset and UART0 emits boot or application output
- **THEN** the raw output and capture metadata are saved and any observed firmware/build identity is recorded with its evidence

#### Scenario: Existing firmware remains unidentified
- **WHEN** captured UART0 output contains no reliable firmware/build identity
- **THEN** the workflow records the capture and explicitly reports the identity as unknown without inferring ESP-AT

### Requirement: Complete preserved flash backup
Before any erase or write, the workflow SHALL read flash address `0x0` through exactly `0x1000000` bytes into a preserved backup, record its SHA-256 and byte size, and verify that the file is readable and exactly 16,777,216 bytes.

#### Scenario: Backup passes validation
- **WHEN** esptool completes the full-range read and the resulting file is readable, has the exact required size, and reproduces its recorded SHA-256
- **THEN** the backup is accepted as the prerequisite recovery artifact and retained without modification

#### Scenario: Backup fails validation
- **WHEN** the read fails, the file is unreadable, its size differs, or its digest does not reproduce
- **THEN** the workflow rejects the backup and prohibits every erase or write operation

### Requirement: Supported 16 MB ESP-AT strategy
The workflow SHALL establish a supported firmware strategy for the physical 16 MB flash before creating an executable write plan. It SHALL use an official released binary only with affirmative compatibility evidence; otherwise it SHALL produce a locally built ESP-AT image from pinned sources with an explicit compatible 16 MB flash and partition configuration.

#### Scenario: Official image compatibility is established
- **WHEN** authoritative release or configuration evidence establishes that a selected official ESP32-C6 ESP-AT image supports the target revision and 16 MB layout
- **THEN** the workflow may select that image and records the supporting evidence

#### Scenario: Released 4 MB image lacks compatibility evidence
- **WHEN** the available official image is configured for 4 MB and supported compatibility with this 16 MB board has not been established
- **THEN** the workflow SHALL NOT flash it and instead prepares a supported customized build or stops

### Requirement: Defined UART1 AT interface
The provisioned modem SHALL expose its AT command interface on UART1 with ESP32-C6 GPIO6 as RX, GPIO7 as TX, 115200 baud, 8 data bits, no parity, one stop bit, and no hardware flow control.

#### Scenario: Host communicates on the intended mapping
- **WHEN** a host sends `AT\r\n` to GPIO6 using 115200 8N1 without flow control and receives from GPIO7
- **THEN** the modem returns a valid ESP-AT response on that interface

### Requirement: Wi-Fi and TCP only modem scope
The provisioned modem SHALL be intended for Wi-Fi and TCP transport and SHALL NOT require or exercise ESP-AT MQTT functionality during provisioning or verification.

#### Scenario: Provisioning is verified without MQTT or credentials
- **WHEN** the post-provisioning verification is performed
- **THEN** it uses only credential-free identity and AT-interface checks and sends neither MQTT commands nor network credentials

### Requirement: Reproducible firmware and flash manifest
Before authorization, the workflow SHALL record the ESP-AT version, ESP-IDF version, source tag and commit, configuration, partition layout, UART mapping, flash parameters, build commands, tool versions, firmware artifact hashes, and exact proposed flash command or workflow. The partition and offset manifest SHALL fit within 16 MB without overlap.

#### Scenario: Firmware package is ready for review
- **WHEN** firmware selection or build completes
- **THEN** an auditor can reproduce the artifacts and determine exactly which hashed bytes will be written at each flash offset

#### Scenario: Flash layout is invalid
- **WHEN** an artifact exceeds flash bounds, overlaps another region, or lacks a recorded hash or offset
- **THEN** the workflow rejects the package before authorization

### Requirement: Explicit final destructive authorization gate
The workflow SHALL treat every ESP32-C6 erase or write as destructive and SHALL require fresh, explicit user authorization immediately before execution. The request SHALL identify the matched device, verified backup digest, firmware manifest digest, exact erase/write workflow, and consequences. Preparation approval SHALL NOT count as flash authorization.

#### Scenario: Destructive action is not authorized
- **WHEN** preparation is complete but fresh explicit authorization for the identified write plan has not been given
- **THEN** no erase or write command is executed

#### Scenario: Authorized inputs change
- **WHEN** the device identity, backup digest, firmware manifest, or exact write plan changes after authorization
- **THEN** the authorization is invalidated and the workflow returns to the final gate

#### Scenario: Destructive operation is explicitly authorized
- **WHEN** the user explicitly authorizes the fully identified final write plan and all prerequisites still validate
- **THEN** the workflow may execute only that authorized operation and records its complete result

### Requirement: Post-provisioning verification
After an authorized provisioning operation, the workflow SHALL verify boot/log sanity, a valid response to `AT`, firmware identity through `AT+GMR`, and bidirectional UART1 operation on GPIO6/GPIO7 at 115200 8N1 without hardware flow control.

#### Scenario: Provisioned modem passes verification
- **WHEN** the modem reboots after provisioning
- **THEN** UART0 logs are sane, `AT` and `AT+GMR` return valid responses, and the documented UART1 mapping works bidirectionally

#### Scenario: Verification fails
- **WHEN** any required boot, identity, command-response, or UART mapping check fails
- **THEN** the workflow records the failure and does not claim successful provisioning or automatically restore flash

### Requirement: STM32 implementation remains unchanged
The change SHALL NOT modify the STM32L476RG USART1 implementation or add application Wi-Fi, TCP, or MQTT behavior.

#### Scenario: Change scope is audited
- **WHEN** implementation changes are reviewed
- **THEN** only ESP32-C6 provisioning workflow, configuration, documentation, and evidence-handling files are affected
