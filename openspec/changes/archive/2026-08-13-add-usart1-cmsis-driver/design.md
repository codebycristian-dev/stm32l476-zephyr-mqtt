## Context

The target is the NUCLEO-L476RG running Zephyr 4.4.0. USART2 on PA2/PA3 already carries the Zephyr console through ST-LINK and must remain untouched. The new USART1 path is a deliberately small transport foundation: direct STM32L476 CMSIS register access, polling TX, interrupt-driven RX, bounded static storage, and observable error accounting. It must coexist with Zephyr's kernel and interrupt infrastructure without using the Zephyr UART API or STM32 HAL/LL for USART1.

## Goals / Non-Goals

**Goals:**

- Configure USART1 on PA9/PA10 as 115200 8N1 with no flow control using CMSIS register definitions.
- Provide bounded polling TX and interrupt-driven RX through a single-producer/single-consumer static ring buffer.
- Make parity, framing, noise, overrun, and software-buffer overflow events observable through counters.
- Preserve the existing USART2 ST-LINK console configuration and behavior.
- Make the low-level behavior testable on the host and verifiable through build, register inspection, and physical loopback.
- Provide a project-local review skill and a concise, non-operational ESP-AT wiring handoff.

**Non-Goals:**

- ESP-AT parsing or ESP32-C6 communication.
- Wi-Fi, TCP, or MQTT behavior.
- DMA-based UART transfer or RTS/CTS flow control.
- A general-purpose portable UART driver or replacement for the Zephyr console.
- Connecting to, configuring, or claiming the installed pin mapping of an ESP32-C6 modem.

## Decisions

### Isolate USART1 behind a small transport module

The application will expose a narrow USART1 transport interface for initialization, polling transmit, non-blocking receive, ring-buffer state, and counter snapshots. Hardware register manipulation stays in the target implementation, while ring-buffer mechanics remain hardware-independent for host testing. This prevents register details from spreading into application code.

An inline implementation in the application entry point was considered, but rejected because it would couple hardware bring-up to loopback logic and make the ring buffer harder to test independently.

### Configure GPIOA and USART1 exclusively through CMSIS registers

Initialization will directly enable GPIOA and USART1 clocks, select alternate function AF7 for PA9 and PA10, configure the pins for the USART role, and program USART1 for 115200 8N1, oversampling by 16, transmitter/receiver enabled, and hardware flow control disabled. The baud divisor will be derived from the actual USART1 peripheral clock represented by the active RCC clock-source and APB2 prescaler configuration. Compile-time board knowledge is acceptable only when the implementation verifies it against that actual RCC configuration; an unexplained hard-coded 80 MHz peripheral-clock constant is not acceptable.

The module will use STM32L476 CMSIS device definitions only for peripheral access. STM32 HAL, STM32 LL, and Zephyr's UART API were considered but rejected because direct register control is an explicit constraint. USART1 will be disabled in devicetree when necessary to prevent a Zephyr UART driver from claiming it; this does not affect USART2 console ownership.

### Use Zephyr only for interrupt wiring and execution context

The USART1 IRQ will be connected and enabled through Zephyr's interrupt facilities, while all USART status, data, enable, and clear operations remain direct CMSIS accesses. Initialization will clear stale USART status before enabling receive and error interrupts. The ISR will account for parity, framing, noise, and overrun flags, clear them using the STM32-defined interrupt-clear register, and service received data without blocking.

Pure bare-metal vector-table ownership was considered, but rejected because the application executes under Zephyr and must integrate safely with its interrupt model.

### Use a static single-producer/single-consumer RX ring

The RX ring will have a compile-time fixed, power-of-two capacity and monotonic or masked head/tail indices. The USART1 ISR is the sole producer and application context is the sole consumer. Producer publication and consumer observation will use compiler/architecture-safe ordering suitable for the target; no heap allocation, mutex, or blocking operation occurs in the ISR.

When the ring is full, the ISR will discard the newly received byte, retain already-buffered data, and increment a software-overflow counter. Overwriting unread bytes was considered, but rejected because preserving the earliest received sequence gives deterministic loss semantics.

### Poll TX with an explicit bounded API contract

Transmit operations will wait for the transmit-data-empty indication before each byte and, when the API promises completed wire transmission, wait for transmission-complete after the final byte. The API will reject invalid buffer arguments and will document that it is synchronous and unsuitable for ISR context. No TX queue or allocation is introduced.

### Keep counters saturating and snapshot-friendly

Separate counters will represent parity, framing, noise, hardware overrun, and ring-buffer overflow events. Counters will saturate at their maximum value rather than wrap, and application code will read a coherent snapshot through the transport interface. Successful loopback verification requires no unexpected increments.

### Add a project-local review skill without duplicating the driver

The repository will include a project-local skill named `stm32l476-usart-review`. It will guide review of RCC clock enable and source assumptions, GPIOA PA9/PA10 configuration, AF7 selection, USART1 frame configuration, BRR derivation, interrupt configuration, ISR boundedness, RX ring-buffer bounds, error accounting, USART2/ST-LINK preservation, and the absence of STM32 HAL, STM32 LL, Zephyr UART API use for USART1, and dynamic allocation.

The skill will inspect and report on the repository implementation and tests, which remain the source of truth. It will not contain or duplicate driver implementation logic.

### Document the future ESP-AT wiring as a handoff only

Concise documentation will record the future connection as `STM32 PA9 USART1_TX -> ESP32-C6 GPIO6 RX`, `STM32 PA10 USART1_RX <- ESP32-C6 GPIO7 TX`, and `GND <-> GND`. GPIO6/GPIO7 will be identified as the official default ESP-AT UART1 mapping on ESP32-C6 and as customizable. The documentation will explicitly avoid claiming that the currently installed ESP-AT firmware uses those pins, and this change will not connect or communicate with the modem.

### Verify in layers

Host tests will exercise ring wraparound, FIFO ordering, empty/full boundaries, overflow policy, and counter saturation without target hardware. A pristine Zephyr build will establish integration. Register review will confirm clock gates and sources, the active APB2 relationship, GPIO AF7 routing, frame format, derived baud divisor, interrupt enables, and preservation of USART2. The final target test will physically bridge PA9 to PA10 and exercise several bounded payload sizes: a short payload, a payload containing `0x00` and non-printable bytes, and a payload spanning ring-buffer wraparound. Each payload must be retrieved exactly and in order with zero unexpected error counters. Hardware verification will not intentionally provoke uncontrolled peripheral overruns; overflow semantics remain primarily host-tested.

## Risks / Trade-offs

- **[Peripheral-clock assumptions produce the wrong baud]** → Derive the USART1 clock from the active RCC source and APB2 configuration, verify any compile-time board knowledge against those registers, and include the derivation and BRR in review.
- **[Zephyr or devicetree also claims USART1]** → Disable USART1's Zephyr device instance as needed and review the generated configuration while leaving USART2 chosen as the console.
- **[ISR/application races corrupt ring state]** → Keep strict single-producer/single-consumer ownership, use fixed-width atomic index accesses and required ordering, and stress wraparound in host tests.
- **[Polling TX can block indefinitely after hardware failure]** → Keep the intended synchronous contract explicit and structure status waits so a bounded timeout can be added without changing RX buffering; loopback verification detects basic liveness failures.
- **[Error-flag clearing loses a received byte or double-counts an event]** → Follow the STM32L476 USART clear sequence, capture status once per ISR pass, drain RX deterministically, and review the generated register operations.
- **[Direct-register code is target-specific]** → Confine it to the STM32L476 USART1 module and keep portable ring logic separate.

## Migration Plan

1. Add and host-test the standalone ring-buffer component.
2. Add the USART1 CMSIS transport and target configuration without changing the application behavior that uses USART2 console output.
3. Integrate a bounded loopback verification path, run a pristine build, and inspect generated configuration and registers.
4. Flash only during explicitly authorized hardware verification, connect PA9 to PA10, and execute the loopback acceptance test.

Rollback consists of removing the USART1 module and its USART1-specific board configuration; USART2 console files and settings remain unchanged throughout.

## Open Questions

- The concrete fixed RX ring capacity will be selected during implementation based on the existing memory budget and recorded in the public module contract.
- Whether the physical loopback runs automatically at startup or through an explicit test build/configuration will be chosen to avoid altering normal application startup behavior.
