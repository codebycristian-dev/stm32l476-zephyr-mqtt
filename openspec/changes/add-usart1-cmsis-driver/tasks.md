## 1. Transport Contract and Configuration

- [ ] 1.1 Define the USART1 transport API, fixed RX capacity, return semantics, and error-counter snapshot type.
- [ ] 1.2 Add build configuration that prevents Zephyr UART ownership of USART1 while preserving the USART2 console selection.
- [ ] 1.3 Define how the actual USART1 clock is derived from RCC/APB2 state and how compile-time board knowledge is verified against it.

## 2. Static Receive Ring

- [ ] 2.1 Implement the allocation-free single-producer/single-consumer byte ring with drop-newest overflow behavior.
- [ ] 2.2 Implement saturating counter helpers used by ring overflow and USART error accounting.
- [ ] 2.3 Add host tests for empty/full boundaries, FIFO ordering, wraparound, overflow preservation, and counter saturation.

## 3. USART1 CMSIS Implementation

- [ ] 3.1 Implement direct RCC and GPIOA register setup for USART1 on PA9/PA10 alternate function AF7.
- [ ] 3.2 Implement direct USART1 setup for 115200 8N1 with BRR derived from verified RCC/APB2 state, oversampling by 16, and no flow control.
- [ ] 3.3 Implement synchronous polling TX with argument validation and documented completion semantics.
- [ ] 3.4 Implement USART1 interrupt connection, enablement, and initialization-time stale-status clearing.
- [ ] 3.5 Implement ISR-driven RX insertion and parity, framing, noise, overrun, and ring-overflow accounting.
- [ ] 3.6 Implement non-blocking RX retrieval and coherent saturating error-counter snapshots.

## 4. Application Integration

- [ ] 4.1 Integrate USART1 initialization without changing USART2 PA2/PA3 console startup or LED behavior.
- [ ] 4.2 Add bounded PA9-to-PA10 loopback payloads covering short, binary/non-printable, and ring-wraparound cases, with results reported through USART2.

## 5. Verification and Documentation

- [ ] 5.1 Add concise future ESP-AT wiring handoff documentation with the GPIO6/GPIO7 default-mapping and installed-firmware caveats.
- [ ] 5.2 Add the project-local `stm32l476-usart-review` skill covering every specified review concern without duplicating driver code.
- [ ] 5.3 Run all host ring-buffer tests and record passing results.
- [ ] 5.4 Run a pristine Zephyr 4.4.0 build for `nucleo_l476rg` and confirm no forbidden USART1 API dependencies.
- [ ] 5.5 Run `stm32l476-usart-review` and confirm RCC/APB2/BRR, GPIO, frame, IRQ/ISR, ring, errors, console preservation, and prohibited-use findings against repository evidence.
- [ ] 5.6 With explicit flash authorization, run all bounded physical PA9-to-PA10 loopback payloads and confirm exact data plus zero unexpected UART errors.
