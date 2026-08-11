---
name: zephyr-build-l476rg
description: Diagnose, build, prepare an explicitly authorized flash, or monitor this repository's Zephyr 4.4.0 application for nucleo_l476rg.
---

# Zephyr build workflow for NUCLEO-L476RG

Use this skill for project environment diagnostics, builds, memory reports,
explicitly requested hardware programming, or ST-LINK console monitoring.
The scripts in `scripts/` are authoritative; invoke them by repository path
rather than copying their internal commands.

## Boundaries

- Use only `nucleo_l476rg`, `/home/cristian/.venvs/zephyr`, and the existing
  `/home/cristian/zephyrproject` workspace.
- Never install or update dependencies and never write beneath the workspace.
- Diagnostics and builds never imply flashing. Run `scripts/flash.sh` without
  `--dry-run` only after an explicit hardware-programming request.
- Require the repository-local build artifact before any flash attempt.

## Workflow

1. Run `scripts/doctor.sh` for read-only diagnostics.
2. Run `scripts/build.sh` for a pristine repository-local build.
3. Record Zephyr's separate FLASH/ROM and RAM used/available output and verify
   the flashable image under `build/nucleo_l476rg/zephyr/`.
4. For preparation only, run `scripts/flash.sh --dry-run`. With explicit user
   authorization, run `scripts/flash.sh` and record its exit status and
   programmer success/restart evidence.
5. Run `scripts/monitor.sh --device DEVICE` for the 115200-baud 8-N-1 ST-LINK
   console; it does not change permissions.

Report independent PASS/FAIL evidence for: build completion; FLASH/ROM and RAM
reporting; authorized flash and MCU restart; the startup identifier appearing
within 5 seconds; and LED state changes with each state lasting at least 250 ms.
