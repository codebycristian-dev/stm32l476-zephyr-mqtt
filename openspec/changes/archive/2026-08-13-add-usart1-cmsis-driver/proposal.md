## Why

The application needs a deterministic, dependency-light USART1 transport on the NUCLEO-L476RG for future peripheral integration while retaining the existing ST-LINK console. Defining and verifying the low-level transport independently now provides a trustworthy foundation before any ESP-AT or network protocol work begins.

## What Changes

- Add USART1 operation on PA9 (TX) and PA10 (RX) at 115200 baud, 8 data bits, no parity, one stop bit, and no hardware flow control.
- Configure and operate USART1 through direct CMSIS register access without STM32 HAL, STM32 LL, or the Zephyr UART API.
- Provide polling transmission and interrupt-driven reception backed by a static fixed-size ring buffer with no dynamic allocation.
- Track USART1 receive and peripheral error conditions with inspectable counters.
- Preserve USART2 on PA2/PA3 as the unchanged ST-LINK console.
- Derive USART1 BRR from the actual peripheral clock represented by the active RCC/APB2 configuration, without relying on an unexplained hard-coded 80 MHz assumption.
- Add a project-local `stm32l476-usart-review` skill that reviews the implementation and tests without duplicating driver logic; repository implementation and tests remain authoritative.
- Document the future ESP-AT wiring handoff without connecting or communicating with the modem in this change.
- Add host tests for ring-buffer behavior and verification covering a pristine Zephyr 4.4.0 build, register review, and bounded physical PA9-to-PA10 loopback payloads with zero unexpected UART errors.

## Capabilities

### New Capabilities

- `usart1-cmsis-transport`: Direct-register USART1 transport behavior, buffering, error accounting, console coexistence, and verification requirements for the NUCLEO-L476RG.

### Modified Capabilities

- `zephyr-application-foundation`: Require the application foundation to integrate the new USART1 transport without changing the existing USART2 ST-LINK console.

## Impact

- Affects the Zephyr application source, STM32L476RG board configuration or devicetree overlays as needed to avoid ownership conflicts, interrupt configuration, host-side tests, project-local review skill, and concise handoff documentation.
- Adds no runtime heap use and no dependency on STM32 HAL, STM32 LL, or Zephyr's UART API for USART1.
- Keeps existing USART2 console behavior and pins unchanged.
- Documents only the future ESP-AT electrical signal mapping; it does not connect a modem or include ESP-AT parsing, ESP32-C6 communication, Wi-Fi, TCP, MQTT, DMA, or RTS/CTS.
