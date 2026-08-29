---
name: espat-development-modem-readiness
description: Safely discover and qualify a USB-connected ESP32-C6 or ESP-AT development modem with repository tooling. Use for modem readiness, harmless AT identity checks, serial configuration discovery, interactive Wi-Fi/TCP qualification, sanitized evidence, or diagnosis before any STM32 USART1 integration.
---

# Qualify an ESP-AT development modem

Treat `scripts/qualify_espat.py` as the executable source of truth. Do not copy
its serial protocol logic into ad hoc commands or this skill.

## Boundaries

- Never flash, erase, restore, replace firmware, alter partitions, install
  packages, or access an unrelated serial device.
- Never implement, configure, or connect STM32 USART1 and never use MQTT or
  ESP-AT MQTT commands.
- If ESP-AT is absent or harmless probes fail, preserve diagnostics and stop;
  request explicit authorization before any destructive recovery action.
- If device or interface identity is ambiguous, list it and stop instead of
  probing by guesswork.
- For the currently verified ESP32-C6 revision 0.2, treat WCH `1a86:55d3`
  `/dev/ttyACM0` as UART0 download/log, not as the UART1 AT endpoint. Do not
  infer ESP-AT absence from probes on that path. UART1 qualification requires
  an external host on GPIO6 (RX) and GPIO7 (TX); the later STM32L476RG USART1
  stage may supply it under separate authorization.
- Accept Wi-Fi credentials only through the tool's non-echoing prompts. Never
  put credentials in arguments, environment variables, files, logs, evidence,
  or chat.

## Workflow

1. Run `python3 scripts/qualify_espat.py discover` and review USB identity,
   topology, every serial interface, by-id links, and permissions.
2. Proceed only with one unambiguous, role-verified AT endpoint. Run
   `python3 scripts/qualify_espat.py baseline --output docs/espat-qualification-result.json`.
3. Review the bounded harmless probe and all four baseline command outcomes.
   Stop if `AT` does not identify working ESP-AT communication.
4. When credentials are available in an interactive terminal, run
   `python3 scripts/qualify_espat.py full --output docs/espat-qualification-result.json`.
   Select a deliberate plain TCP echo endpoint; do not use MQTT.
5. Run `python3 -m unittest tests/test_qualify_espat.py`, scan the evidence and
   repository diff for secrets, validate this skill, and run strict OpenSpec
   validation.

Interpret each evidence stage only as `pass`, `fail`, `inconclusive`, or
`not-run`. Missing prerequisites, permission denial, a busy port, ambiguous
devices, modem failure, Wi-Fi failure, and TCP endpoint failure are distinct.
Do not call the modem fully qualified or the change archive-ready until Wi-Fi
association, IP acquisition, and TCP connect/send/receive/close pass, unless
the user explicitly accepts a partial result.
