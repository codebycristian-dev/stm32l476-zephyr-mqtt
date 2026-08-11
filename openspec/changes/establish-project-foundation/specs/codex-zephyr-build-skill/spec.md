## ADDED Requirements

### Requirement: Project-specific build skill
The repository SHALL contain a project-specific Codex skill named `zephyr-build-l476rg` with valid skill metadata and instructions for the supported environment diagnostics, build, flash, and serial-monitor workflows.

#### Scenario: Skill discovery
- **WHEN** Codex discovers skills from this project
- **THEN** it can identify `zephyr-build-l476rg` by name and determine when the skill applies from its description

### Requirement: Skill preserves operational boundaries
The skill SHALL direct Codex to use the existing project scripts and known Zephyr workspace while prohibiting dependency installation, Zephyr workspace modification, implicit hardware flashing, and substitution of a different target board.

#### Scenario: Build-only request
- **WHEN** the skill is used for a request to diagnose or build the application
- **THEN** its workflow does not flash hardware and does not write beneath `/home/cristian/zephyrproject`

#### Scenario: Explicit flash request
- **WHEN** the skill is used for an explicit request to flash a connected target
- **THEN** it first requires the expected build artifact and uses the project's controlled flash workflow for `nucleo_l476rg`

### Requirement: Skill provides verification guidance
The skill SHALL state how to verify build completion, memory reporting, successful flashing, startup-console output, and user-LED operation using measurable outcomes from the project specifications.

#### Scenario: Foundation acceptance verification
- **WHEN** Codex follows the skill to verify the completed foundation on available hardware
- **THEN** it reports separate pass or fail evidence for build, FLASH/ROM and RAM reporting, flash, startup log within 5 seconds, and observable LED state changes

