---
name: stm32l476-usart-review
description: Audit this repository's STM32L476RG USART1 direct-CMSIS transport, clock/BRR derivation, PA9/PA10 setup, IRQ/ring/error behavior, forbidden dependencies, and USART2 console preservation. Use for implementation reviews and pre-flash verification of the add-usart1-cmsis-driver change.
---

# STM32L476 USART review

Perform a read-only, evidence-based review. Treat repository source, generated
Zephyr files, tests, and build output as authoritative; do not reproduce or
invent driver logic in this skill.

1. Inspect `include/usart1_transport.h`, `include/byte_ring.h`,
   `src/usart1_transport.c`, `src/byte_ring.c`, `src/main.c`, the board overlay,
   `prj.conf`, host tests, and relevant OpenSpec artifacts.
2. Inspect the pristine build's `.config`, generated devicetree, map file, and
   size output. If the build is absent or stale, report that instead of assuming.
3. Report PASS/FAIL with file or generated-artifact evidence for:
   - RCC GPIOA and USART1 gates; live `USART1SEL`, SYSCLK/PLL, AHB, APB2 and
     compile-time clock verification; BRR rounding and resulting nominal baud;
   - PA9 TX and PA10 RX alternate mode, AF7, output type, speed, and pulls;
   - 115200 8N1, oversampling by 16, TE/RE, and absence of RTS/CTS;
   - Zephyr IRQ connection/enable, RX/error interrupt enables, stale-state
     clearing, and a minimal bounded non-blocking ISR;
   - fixed power-of-two static ring bounds, SPSC ordering, FIFO behavior,
     drop-newest preservation, and no allocation;
   - separate saturating parity, framing, noise, overrun, and ring-overflow
     counters plus coherent snapshots;
   - polling, binary-safe TX with no DMA, queue, or TX interrupt;
   - USART2 PA2/PA3 remaining the chosen Zephyr/ST-LINK console;
   - no STM32 HAL, STM32 LL, or Zephyr UART API operation for USART1;
   - no ESP-AT, ESP32-C6 communication, Wi-Fi, TCP, or MQTT implementation.
4. Run `./tests/run_host_tests.sh` and concise repository searches for forbidden
   calls and allocation. Do not classify ordinary Zephyr's USART2 console
   dependency as forbidden; the restriction is USART1 operation.
5. Confirm loopback mode is default-off and contains bounded short, binary, and
   physical-ring-wrap payloads. Never flash or claim physical results without
   explicit authorization and observed target evidence.
6. Finish with an overall PASS/FAIL, clock/BRR values, test/build evidence,
   findings by concern, and any remaining hardware gate.
