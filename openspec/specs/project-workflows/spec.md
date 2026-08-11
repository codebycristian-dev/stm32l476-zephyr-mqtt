## Purpose

Define safe, repeatable repository workflows for environment diagnostics, application builds, explicitly authorized flashing, and ST-LINK serial monitoring.

## Requirements

### Requirement: Environment diagnostics workflow
The project SHALL provide a non-installing diagnostics script that validates the required host paths and reports the detected Python, West, Zephyr, SDK/toolchain, board, programmer, and serial-device prerequisites without modifying `/home/cristian/zephyrproject`.

#### Scenario: Known environment is ready
- **WHEN** diagnostics run on the documented Ubuntu environment with all known prerequisites present
- **THEN** the script exits successfully and reports each required prerequisite as available, including Zephyr 4.4.0, West 1.5.0, the `/home/cristian/.venvs/zephyr` environment, and `nucleo_l476rg` board support

#### Scenario: Required prerequisite is absent
- **WHEN** diagnostics cannot find or validate a required prerequisite
- **THEN** the script exits nonzero and identifies the missing or mismatched prerequisite without installing or changing anything

### Requirement: Reproducible build workflow
The project SHALL provide a build script that activates or directly uses `/home/cristian/.venvs/zephyr`, invokes West against `/home/cristian/zephyrproject`, explicitly selects `nucleo_l476rg`, and keeps all generated application artifacts inside the project repository.

#### Scenario: Build from an unrelated working directory
- **WHEN** the build script is invoked by its path from outside the repository root
- **THEN** it resolves project and workspace paths deterministically and builds the same application target successfully

### Requirement: Controlled flash workflow
The project SHALL provide a flash script that uses the successfully built project image and the supported Zephyr/STM32CubeProgrammer runner path, fails clearly if required artifacts or tools are absent, and requires an explicit invocation to program hardware.

#### Scenario: Flash command preparation
- **WHEN** the flash script is inspected or invoked without a valid build artifact
- **THEN** its target is unambiguously `nucleo_l476rg`, it does not rebuild or install software implicitly, and it exits without attempting a flash

#### Scenario: Successful authorized flash
- **WHEN** a valid application artifact and connected NUCLEO-L476RG are present and a developer explicitly runs the flash script
- **THEN** the programming command completes with exit status zero and reports successful target programming

### Requirement: Reproducible serial monitoring workflow
The project SHALL provide a serial-monitor script for the ST-LINK virtual COM port with documented default console settings and an explicit override for the serial device.

#### Scenario: Monitor explicit serial device
- **WHEN** a developer supplies the ST-LINK virtual COM device to the monitor script
- **THEN** the script opens that device using the documented baud rate and 8-N-1 framing without reconfiguring USART1

#### Scenario: Serial device unavailable
- **WHEN** the selected serial device does not exist or is inaccessible
- **THEN** the script exits nonzero with an actionable diagnostic and does not modify device permissions

### Requirement: Workflow safety boundaries
All workflow scripts SHALL avoid software installation, package updates, and writes beneath `/home/cristian/zephyrproject`.

#### Scenario: Workflow side-effect review
- **WHEN** the diagnostics, build, flash, and monitor scripts are reviewed
- **THEN** their writes are confined to repository-local build outputs and the explicitly requested hardware interfaces
