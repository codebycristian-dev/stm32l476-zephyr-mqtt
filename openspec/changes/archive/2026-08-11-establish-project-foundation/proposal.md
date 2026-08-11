## Why

The repository needs a reproducible, independently buildable baseline before STM32-hosted transport and MQTT layers can be developed safely. Establishing the board, console, startup, tooling, and repository conventions now provides a measurable foundation for incremental spec-driven work without modifying the existing Zephyr workspace.

## What Changes

- Establish the repository as a standalone C Zephyr 4.4.0 application targeting `nucleo_l476rg` through the existing `/home/cristian/zephyrproject` workspace.
- Add a minimal startup application that emits a visible console message through the board's ST-LINK virtual COM port and exercises the user LED.
- Add the minimum `src/`, `include/`, `tests/`, `scripts/`, and `docs/` project structure.
- Add reproducible scripts for environment diagnostics, application build, hardware flashing, and serial monitoring.
- Add a project-specific Codex skill named `zephyr-build-l476rg` that documents the supported diagnostic, build, flash, and monitoring workflow.
- Define measurable acceptance criteria for build success, flash preparation/execution, startup logging, LED operation, and build memory reporting.
- Preserve USART2 for the Zephyr/ST-LINK diagnostic console and reserve the future USART1 path for direct CMSIS-register-based communication with an ESP32-C6 coprocessor.
- Exclude USART1, ESP-AT, Wi-Fi, TCP, MQTT, ESP32-C6 communication code, package installation, hardware flashing, and changes to `/home/cristian/zephyrproject` from this change.

## Capabilities

### New Capabilities

- `zephyr-application-foundation`: Standalone `nucleo_l476rg` application configuration, startup behavior, console visibility, user-LED exercise, and memory-reporting acceptance requirements.
- `project-workflows`: Reproducible environment diagnostics, build, flash, and serial-monitor workflows that use the existing Zephyr workspace without modifying it.
- `codex-zephyr-build-skill`: Project-local Codex guidance for consistently applying the supported NUCLEO-L476RG Zephyr workflow.

### Modified Capabilities

None.

## Impact

The change will introduce repository-local Zephyr application metadata and C source, structural placeholder/documentation files, shell scripts, project documentation, and a project-specific Codex skill. It will rely on the known Zephyr 4.4.0 workspace, Python virtual environment, West, Zephyr SDK/toolchain, STM32CubeProgrammer, and the board's existing ST-LINK interfaces, but will neither install dependencies nor alter external workspaces. No communication protocol implementation or firmware API is introduced in this foundation change.
