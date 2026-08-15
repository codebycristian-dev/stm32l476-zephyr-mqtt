# ESP-AT USART1 diagnostic wiring handoff

The direct-CMSIS USART1 transport is implemented and physically verified. The
next separately authorized diagnostic flash uses this crossed UART wiring:

- `STM32 PA9 USART1_TX -> ESP32-C6 GPIO6 RX`
- `STM32 PA10 USART1_RX <- ESP32-C6 GPIO7 TX`
- `GND <-> GND`

GPIO6/GPIO7 are the official default ESP-AT UART1 mapping for ESP32-C6, but
customized ESP-AT firmware can select different pins. This change has not yet
established which UART pins the currently installed firmware uses. The prepared
firmware sends only `AT\r\n`, waits up to 1000 ms for at most 64 bytes, and
reports its classification on USART2/ST-LINK; it performs no Wi-Fi or TCP work.
