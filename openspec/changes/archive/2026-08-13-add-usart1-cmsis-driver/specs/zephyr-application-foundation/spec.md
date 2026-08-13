## MODIFIED Requirements

### Requirement: Diagnostic console preservation
The application SHALL keep the Zephyr console routed through USART2 on PA2/PA3 over the NUCLEO board's ST-LINK virtual COM path and SHALL keep that diagnostic path independent of USART1 on PA9/PA10.

#### Scenario: Startup console output
- **WHEN** a successfully flashed board resets and the serial monitor is attached using the documented settings
- **THEN** the console displays an application startup message identifying the project within 5 seconds of reset through the unchanged USART2 PA2/PA3 ST-LINK virtual COM path

### Requirement: Foundation scope isolation
The foundation firmware SHALL limit USART1 behavior to the direct-register transport defined by the `usart1-cmsis-transport` capability and SHALL NOT implement ESP-AT parsing, Wi-Fi, TCP, MQTT, or ESP32-C6-specific communication behavior. It SHALL NOT use Arduino, Zephyr's MQTT library, or dynamic allocation for communication layers.

#### Scenario: Foundation source review
- **WHEN** the foundation implementation is reviewed
- **THEN** it contains startup, diagnostic-console, user-LED, and the bounded USART1 transport behavior, and none of the excluded protocol or coprocessor functionality
