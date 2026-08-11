## ADDED Requirements

### Requirement: Standalone Zephyr application
The repository SHALL contain a standalone firmware application written in C that can be built for `nucleo_l476rg` with Zephyr 4.4.0 from the existing `/home/cristian/zephyrproject` workspace, without writing to or modifying that workspace.

#### Scenario: Pristine application build
- **WHEN** the documented build workflow is run from a clean project build state with the known environment available
- **THEN** West completes successfully for board `nucleo_l476rg` and produces a flashable Zephyr image in a repository-local build directory

### Requirement: Diagnostic console preservation
The application SHALL keep the Zephyr console routed through the NUCLEO board's ST-LINK virtual COM path and SHALL reserve that diagnostic path independently of the future ESP32-C6 USART1 communication path.

#### Scenario: Startup console output
- **WHEN** a successfully flashed board resets and the serial monitor is attached using the documented settings
- **THEN** the console displays an application startup message identifying the project within 5 seconds of reset

### Requirement: User LED exercise
The startup application SHALL exercise the `nucleo_l476rg` board user LED using Zephyr's board description and GPIO facilities.

#### Scenario: Observable LED operation
- **WHEN** the application starts on the target board
- **THEN** the user LED repeatedly changes between illuminated and extinguished states with each state remaining observable for at least 250 milliseconds

#### Scenario: Missing LED alias
- **WHEN** the selected board configuration does not provide the expected user-LED devicetree alias or its GPIO device is not ready
- **THEN** the application reports a clear error through the Zephyr console and does not silently claim LED operation

### Requirement: Minimal repository structure
The project SHALL provide `src/`, `include/`, `tests/`, `scripts/`, and `docs/` paths with enough tracked content to communicate each path's intended role.

#### Scenario: Foundation layout inspection
- **WHEN** a developer checks out the completed change
- **THEN** all five required paths exist in version control and the application entry point is located under `src/`

### Requirement: Build memory reporting
The build workflow SHALL expose deterministic ROM and RAM utilization reported by the Zephyr build for the produced application image.

#### Scenario: Successful build reports memory
- **WHEN** the documented build completes successfully
- **THEN** its output includes used and available memory values for both FLASH/ROM and RAM, including units or percentages sufficient to evaluate footprint

### Requirement: Foundation scope isolation
The foundation firmware SHALL NOT implement USART1 communication, ESP-AT, Wi-Fi, TCP, MQTT, or ESP32-C6-specific communication behavior, and SHALL NOT use Arduino, Zephyr's MQTT library, or dynamic allocation for future communication layers.

#### Scenario: Foundation source review
- **WHEN** the foundation implementation is reviewed
- **THEN** it contains only startup, diagnostic-console, and user-LED behavior and none of the excluded protocol or coprocessor functionality
