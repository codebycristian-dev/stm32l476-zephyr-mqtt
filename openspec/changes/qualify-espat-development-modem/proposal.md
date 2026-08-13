## Why

The available ESP32-C6 must be qualified as a known-good ESP-AT development modem before STM32 USART1 integration begins. Doing this independently establishes reproducible evidence and a portable ESP-AT contract without making the future STM32 MQTT stack depend on ESP32-C6-specific behavior.

## What Changes

- Add a repeatable, Linux-hosted qualification workflow that identifies the attached ESP32-C6 and its USB serial interfaces, selects the AT interface, detects whether ESP-AT is installed, and determines the working serial configuration without flashing, erasing, restoring, or installing packages.
- Verify and record basic ESP-AT identity and status, then require Wi-Fi association, IP acquisition, and a plain TCP connect/send/receive/close exchange for full qualification. Credentials are accepted only through ephemeral, non-echoing input.
- Define the smallest observed common ESP-AT command and response subset suitable for a future modem-neutral STM32 engine, distinguishing generic ESP-AT behavior from ESP32-C6-specific observations.
- Derive measurable USART1 transport requirements for a later change while leaving STM32 USART1, MQTT, wiring, and ESP-AT MQTT commands out of scope.
- Add a project-local `espat-development-modem-readiness` skill whose single repository qualification tool is the executable source of truth and whose workflow does not persist credentials.

## Capabilities

### New Capabilities

- `espat-modem-qualification`: Covers safe device discovery, serial and firmware qualification, Wi-Fi station and TCP verification, evidence capture, the portable ESP-AT subset, generic-versus-device-specific classification, and future STM32 transport requirements.
- `espat-development-modem-readiness-skill`: Covers a project-local, script-backed, repeatable and secret-safe operator workflow for development-modem qualification.

### Modified Capabilities

None.

## Impact

The change adds OpenSpec requirements, one small host-side qualification tool with minimal tests, minimum non-secret evidence, and a repository-local Codex skill. It may read Linux USB/serial metadata and communicate with the explicitly selected modem serial port during implementation. A partial result may be retained while credentials are unavailable, but the change remains active until Wi-Fi/TCP qualification passes or the user explicitly accepts partial qualification. It does not alter the verified STM32 application foundation, `/home/cristian/zephyrproject`, STM32 USART1 code, MQTT code, or modem firmware; destructive modem commands and firmware flashing remain separately authorized actions.
