## Purpose

Define the direct-CMSIS USART1 transport, buffering, error accounting, console coexistence, and verification requirements for the STM32L476RG application.

## Requirements

### Requirement: USART1 direct-register configuration
The application SHALL configure STM32L476RG USART1 through CMSIS register access for PA9 transmit and PA10 receive using alternate function AF7 at 115200 baud, 8 data bits, no parity, one stop bit, oversampling by 16, and no hardware flow control. The USART1 implementation SHALL NOT use STM32 HAL, STM32 LL, or the Zephyr UART API.

USART1 BRR SHALL be derived from the actual USART1 peripheral clock represented by the active RCC clock-source and APB2 configuration. Compile-time board knowledge SHALL be verified against that configuration, and the implementation SHALL NOT rely on an unexplained hard-coded 80 MHz USART1 peripheral-clock constant.

#### Scenario: USART1 register review
- **WHEN** the initialized target's clock, GPIOA, and USART1 registers are reviewed
- **THEN** USART1 and GPIOA clocks are enabled, PA9 and PA10 select AF7, the configured frame is 115200 8N1 without flow control, BRR is derived from the actual USART1 clock and APB2 configuration, and transmit, receive, receive interrupt, and USART1 are enabled

#### Scenario: Forbidden dependency review
- **WHEN** the USART1 implementation and build dependencies are inspected
- **THEN** no USART1 operation calls STM32 HAL, STM32 LL, or the Zephyr UART API

### Requirement: Polling USART1 transmission
The USART1 transport SHALL transmit bytes synchronously by polling the USART1 transmit status and writing the USART1 transmit data register, without a transmit interrupt, DMA, dynamic allocation, or a software transmit queue.

#### Scenario: Transmit a byte sequence
- **WHEN** application context requests transmission of a valid non-empty byte sequence
- **THEN** the transport writes every byte in order as USART1 becomes ready and reports completion according to its synchronous contract

### Requirement: Interrupt-driven buffered reception
The USART1 transport SHALL receive bytes through the USART1 interrupt into a statically allocated fixed-size ring buffer and SHALL provide application context with non-blocking FIFO retrieval without dynamic allocation.

#### Scenario: Receive ordered bytes
- **WHEN** USART1 receives bytes while free ring-buffer capacity is available
- **THEN** the interrupt handler stores the bytes and application retrieval returns them once in arrival order

#### Scenario: Read an empty ring
- **WHEN** application context requests a byte while the receive ring is empty
- **THEN** the transport reports that no byte is available without blocking or returning stale data

#### Scenario: Receive while ring is full
- **WHEN** a byte arrives while the receive ring has no free capacity
- **THEN** the new byte is discarded, unread buffered bytes remain in FIFO order, and the ring-overflow counter increments

### Requirement: USART1 error accounting
The USART1 transport SHALL maintain separate fixed-width saturating counters for parity, framing, noise, hardware overrun, and receive-ring overflow events and SHALL expose a way for application context to obtain a coherent counter snapshot.

#### Scenario: Account for peripheral errors
- **WHEN** the USART1 interrupt observes one or more asserted parity, framing, noise, or overrun error conditions
- **THEN** the handler increments each corresponding counter once for the observed event and clears the handled peripheral condition according to STM32L476RG requirements

#### Scenario: Counter reaches its maximum
- **WHEN** an error occurs after its corresponding counter has reached its maximum representable value
- **THEN** the counter remains at that maximum instead of wrapping to zero

### Requirement: USART1 loopback acceptance
The target verification SHALL support a physical PA9-to-PA10 loopback that exercises polling transmit, interrupt-driven receive, ring-buffer retrieval, and error accounting with several bounded payload sizes, including a short payload, a payload containing `0x00` and non-printable bytes, and a payload spanning ring-buffer wraparound. Verification SHALL NOT intentionally create uncontrolled hardware overruns.

#### Scenario: Physical loopback succeeds
- **WHEN** PA9 is physically connected to PA10 and the loopback verification transmits each required bounded payload
- **THEN** every payload is retrieved with binary-safe exact equality and in order through the receive ring, with zero unexpected parity, framing, noise, hardware-overrun, or ring-overflow counts

### Requirement: Project-local USART review skill
The repository SHALL provide a project-local skill named `stm32l476-usart-review` that reviews RCC clock enable and source assumptions, GPIOA PA9/PA10 configuration, AF7 selection, USART1 frame configuration, BRR derivation, interrupt configuration, ISR boundedness, RX ring-buffer bounds, error accounting, USART2/ST-LINK preservation, and the absence of STM32 HAL, STM32 LL, Zephyr UART API use for USART1, and dynamic allocation. Repository implementation and tests SHALL remain the source of truth, and the skill SHALL NOT duplicate driver implementation.

#### Scenario: Run the project-local review
- **WHEN** a reviewer invokes `stm32l476-usart-review` against the completed change
- **THEN** the skill evaluates every required review concern from repository evidence and reports findings without supplying a separate driver implementation

### Requirement: Future ESP-AT wiring handoff
The documentation SHALL concisely record the future handoff wiring as `STM32 PA9 USART1_TX -> ESP32-C6 GPIO6 RX`, `STM32 PA10 USART1_RX <- ESP32-C6 GPIO7 TX`, and `GND <-> GND`. It SHALL state that GPIO6/GPIO7 are the official default ESP-AT UART1 mapping on ESP32-C6, that customized ESP-AT firmware may use different pins, and that the currently installed firmware's pin mapping has not been established by this change.

#### Scenario: Review the future modem handoff
- **WHEN** a developer reads the USART1 handoff documentation
- **THEN** the three connections, default-versus-customized mapping caveat, and absence of any claim about the installed firmware are clear, without instructions that connect or communicate with the modem in this change

### Requirement: USART1 transport verification
The change SHALL include host tests for ring-buffer behavior and SHALL build successfully from a pristine state for `nucleo_l476rg` with Zephyr 4.4.0.

#### Scenario: Host ring-buffer tests
- **WHEN** the host test suite runs
- **THEN** it verifies FIFO order, empty and full boundaries, wraparound, overflow behavior, and counter saturation without target hardware

#### Scenario: Pristine target build
- **WHEN** the documented pristine build is run for `nucleo_l476rg` with Zephyr 4.4.0
- **THEN** it succeeds and produces the target firmware image without forbidden USART1 dependencies
