## 1. Standalone Application Structure

- [x] 1.1 Add the root Zephyr application metadata and configuration for a C application targeting `nucleo_l476rg`, preserving the ST-LINK/USART2 Zephyr console.
- [x] 1.2 Create tracked `src/`, `include/`, `tests/`, `scripts/`, and `docs/` paths with concise role documentation and no speculative communication modules.
- [x] 1.3 Implement the minimal `src/` startup entry point with a fixed project-identifying log and devicetree-alias-based user LED initialization.
- [x] 1.4 Implement an observable fixed-period LED toggle loop and clear console error handling for a missing alias or unavailable GPIO device.

## 2. Reproducible Project Workflows

- [x] 2.1 Implement a read-only environment diagnostics script that checks and reports the known Python environment, West, Zephyr 4.4.0 workspace, SDK/toolchain, `nucleo_l476rg` support, STM32CubeProgrammer, and serial prerequisites.
- [x] 2.2 Implement a location-independent build script that uses `/home/cristian/.venvs/zephyr` and `/home/cristian/zephyrproject`, selects `nucleo_l476rg`, and writes to a repository-local build directory.
- [x] 2.3 Ensure the build workflow visibly reports FLASH/ROM and RAM used and available values from the Zephyr build output.
- [x] 2.4 Implement a separate explicit flash script that validates the existing build artifact and programmer tooling before invoking the supported Zephyr runner, without installing, rebuilding, or flashing during implementation of this change.
- [x] 2.5 Implement a serial-monitor script with documented ST-LINK console defaults, an explicit serial-device override, and actionable failures without permission changes.
- [x] 2.6 Add shell-level validation for script path resolution, prerequisite failure behavior, dry-run/static flash safety, and the invariant that scripts never write beneath `/home/cristian/zephyrproject`.

## 3. Project Documentation and Codex Skill

- [x] 3.1 Document repository layout, known environment assumptions, console settings, build outputs, and separate diagnostic/build/flash/monitor commands.
- [x] 3.2 Document measurable acceptance procedures for successful build, memory reporting, authorized flash success, startup log within 5 seconds, and LED state changes of at least 250 milliseconds.
- [x] 3.3 Create the project-specific `zephyr-build-l476rg` Codex skill with valid metadata, clear triggers, script-authoritative workflows, and explicit no-install/no-workspace-write/no-implicit-flash boundaries.
- [x] 3.4 Add skill verification guidance that records separate pass/fail evidence for build, memory, flash, startup log, and LED acceptance.

## 4. Foundation Verification

- [x] 4.1 Run the diagnostics workflow in the known environment and confirm it reports all prerequisites without changing or installing anything.
- [x] 4.2 Perform a pristine `nucleo_l476rg` build and confirm a flashable image is produced inside the repository with FLASH/ROM and RAM utilization shown.
- [x] 4.3 Inspect the implementation and build configuration to confirm it contains no USART1, ESP-AT, Wi-Fi, TCP, MQTT, ESP32-C6 communication, Arduino, STM32 HAL/LL communication, Zephyr MQTT, or dynamic-allocation implementation.
- [x] 4.4 Verify no implementation or verification step changed files beneath `/home/cristian/zephyrproject`.
- [x] 4.5 After implementation and a successful build, stop and request explicit user authorization before hardware programming; once authorized, detect the NUCLEO-L476RG through ST-LINK, successfully program the repository-built firmware, successfully restart the MCU, observe the project startup message through the ST-LINK virtual COM port within 5 seconds, and observe the user LED toggling with each state lasting at least 250 milliseconds; do not consider the change fully verified or ready to archive until every check passes.
