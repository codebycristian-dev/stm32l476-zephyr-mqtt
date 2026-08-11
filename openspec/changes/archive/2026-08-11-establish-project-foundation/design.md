## Context

This repository is becoming the standalone firmware project for an STM32L476RG host MCU on `nucleo_l476rg`. Zephyr 4.4.0 and its dependencies already exist in `/home/cristian/zephyrproject`, the supported Python environment is `/home/cristian/.venvs/zephyr`, and the known board/toolchain combination has built and flashed Zephyr's blinky sample. The foundation must prove that application ownership, board output, and developer workflows live in this repository while the external Zephyr workspace remains unchanged.

The ST-LINK virtual COM console (USART2 on the board) is a permanent diagnostic channel. A later ESP32-C6 link will use USART1 with CMSIS register definitions and statically bounded buffers, but no part of that communication architecture is implemented now.

## Goals / Non-Goals

**Goals:**

- Create the smallest conventional standalone Zephyr C application for `nucleo_l476rg`.
- Provide visible and independently verifiable console and LED startup behavior.
- Make diagnostics, build, flash, and monitoring repeatable through repository-owned scripts.
- Capture project structure and a Codex workflow that future spec-driven changes can extend.
- Keep generated artifacts and all implementation changes inside this repository.

**Non-Goals:**

- Implement USART1 or configure the ESP32-C6 communication path.
- Implement ESP-AT, Wi-Fi, TCP, MQTT, or any ESP32-C6-specific behavior.
- Use Zephyr UART APIs for the future USART1 link, Zephyr's MQTT library, ESP-AT MQTT commands, Arduino, STM32 HAL, or STM32 LL for that communication implementation.
- Install or update software, modify `/home/cristian/zephyrproject`, or flash hardware without explicit user authorization after a successful build.
- Design the later protocol layers beyond preserving their stated architectural boundaries.

## Decisions

### Use the repository as a Zephyr application, not a new workspace

The root will contain standard Zephyr application metadata, configuration, and `src/`, and builds will be launched with the existing workspace's West installation while placing the build directory inside this repository. This avoids duplicating Zephyr modules or coupling project source to the external workspace. Creating a new West workspace or copying the application into the Zephyr tree was rejected because either adds maintenance or violates the no-modification constraint.

### Use Zephyr-native facilities only for foundation behavior

The application will use Zephyr logging/console support and the board's devicetree LED alias through the GPIO API. This is appropriate for startup diagnostics and LED proof and does not weaken the later requirement that the USART1 ESP32-C6 path use direct CMSIS registers. Direct register manipulation for the LED and console was rejected because it would duplicate board configuration and add unnecessary foundation complexity.

### Keep startup behavior deterministic and failure-visible

Startup will emit a fixed project-identifying message and then toggle the user LED at an observable fixed interval. Missing LED configuration or device readiness will produce a console error rather than silently degrading. The console remains independently useful if LED initialization fails.

### Make scripts location-independent and narrowly scoped

Each Bash script will derive the repository root from its own location, use explicit known defaults, accept only useful overrides such as build directory or serial device, fail on errors, and print the effective operation. Diagnostics remain read-only. Build writes only to a repository-local output directory. Diagnostics, static checks, and builds may run automatically, but hardware flashing never does. After implementation and a successful build, the workflow stops and requests explicit user authorization before hardware programming.

The build script will rely on standard Zephyr build output for FLASH/ROM and RAM utilization and will preserve that output in the console. A separate binary-size implementation is unnecessary unless the standard output proves insufficient during implementation.

### Treat flash as a separate, explicit operation

Build and flash will be separate scripts so a successful build never implies programming hardware. The flash script will validate the expected artifact and tool availability before invoking the board-compatible runner backed by STM32CubeProgrammer. Once the user explicitly authorizes flashing, physical verification is required to complete this same change: detect the NUCLEO-L476RG through ST-LINK, program the repository-built firmware, restart the MCU, observe the project startup message through the ST-LINK virtual COM port within 5 seconds, and observe the user LED toggling with each state lasting at least 250 milliseconds. The change is neither fully verified nor ready to archive until all five checks pass.

### Encode project knowledge in a local Codex skill

The `zephyr-build-l476rg` skill will point to the scripts rather than duplicate their commands, define when each operation is appropriate, and make hardware-changing behavior conditional on an explicit request. Keeping workflow mechanics in scripts and guidance in the skill prevents the two from becoming competing implementations.

### Keep structural directories intentionally minimal

`include/`, `tests/`, and `docs/` will receive concise tracked documentation or placeholders describing their intended use; speculative modules and test harnesses will not be created. `scripts/` contains only the four required operational workflows, and `src/` contains the minimal startup entry point.

## Risks / Trade-offs

- [Host paths are machine-specific] → Centralize known defaults in scripts, print them during diagnostics, and permit narrowly scoped overrides where portability is useful.
- [ST-LINK serial device names vary] → Require or allow an explicit device argument and document the expected console framing instead of guessing silently.
- [Hardware verification changes target state] → Enforce a hard authorization gate after a successful build, run no flash implicitly, and require recorded success for every physical acceptance check before treating the change as fully verified or ready to archive.
- [Zephyr output formatting may change] → Accept standard Zephyr 4.4.0 memory reporting semantically rather than parsing fragile decorative formatting.
- [Future direct-register USART1 code could conflict with console configuration] → State and test the USART2/ST-LINK reservation before communication work begins; defer USART1 pin and clock design to its own change.
- [A local skill can drift from scripts] → Make scripts authoritative and have the skill invoke and explain them rather than replicate command bodies.

## Migration Plan

1. Add the standalone Zephyr metadata, configuration, directory skeleton, and minimal application.
2. Add and statically verify the four scripts against known environment paths and safety boundaries.
3. Add the project-local skill and documentation.
4. Run diagnostics, static checks, and a pristine build; confirm generated outputs remain repository-local and memory usage is reported.
5. Stop and request explicit user authorization before hardware programming.
6. Once authorized, detect the NUCLEO-L476RG through ST-LINK, program the repository-built firmware, restart the MCU, observe the startup message through the ST-LINK virtual COM port within 5 seconds, and observe LED toggling with states lasting at least 250 milliseconds.
7. Treat the change as fully verified and ready to archive only after every physical acceptance check passes.

Rollback consists of reverting the repository-local foundation files and build outputs; the external Zephyr workspace requires no rollback because it is never modified.

## Open Questions

None for the foundation. The exact USART1 pins, clocking, interrupt/DMA strategy, ESP-AT framing, and bounded-buffer sizes belong to later scoped changes.
