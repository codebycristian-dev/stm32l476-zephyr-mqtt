# Future ESP-AT USART1 wiring handoff

This change does not connect to or communicate with an ESP32-C6. For a future
integration, the intended common ground and crossed UART wiring is:

- `STM32 PA9 USART1_TX -> ESP32-C6 GPIO6 RX`
- `STM32 PA10 USART1_RX <- ESP32-C6 GPIO7 TX`
- `GND <-> GND`

GPIO6/GPIO7 are the official default ESP-AT UART1 mapping for ESP32-C6, but
customized ESP-AT firmware can select different pins. This change has not
established which UART pins the currently installed ESP-AT firmware uses.
