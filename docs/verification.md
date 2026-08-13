# Foundation verification

Record PASS or FAIL and evidence separately for every item:

1. **Build:** `scripts/build.sh` exits zero for `nucleo_l476rg`, and
   `build/nucleo_l476rg/zephyr/zephyr.elf` plus a flashable `.bin` or `.hex`
   exists inside this repository.
2. **Memory:** build output reports used and available values (with units or a
   percentage) separately for FLASH/ROM and RAM.
3. **Flash (authorization required):** only after explicit approval,
   `scripts/flash.sh` exits zero and STM32CubeProgrammer reports successful
   programming and MCU restart through the connected ST-LINK.
4. **Startup log:** after reset with the monitor already attached, the exact
   project identifier `stm32l476-zephyr-mqtt foundation starting` appears on
   the 115200-baud 8-N-1 ST-LINK console within 5 seconds.
5. **LED:** observe multiple transitions and confirm illuminated and
   extinguished states each last at least 250 ms (the firmware target is 500
   ms per state).

Build and memory evidence do not imply hardware acceptance. Flash, restart,
console timing, and LED evidence remain pending until separately authorized.

## USART1 physical loopback — 2026-08-13

Task 5.6 of `add-usart1-cmsis-driver` passed with PA9 (`USART1_TX`)
physically connected to PA10 (`USART1_RX`). The only programmed target was the
NUCLEO-L476RG ST-LINK probe `066EFF515250898367012013`; USART2 output was
captured from its stable ST-LINK virtual COM endpoint
`usb-STMicroelectronics_STM32_STLink_066EFF515250898367012013-if02` at
115200 8-N-1. The WCH USB serial device `1a86:55d3`, serial `5B14063285`, was
excluded and not opened or programmed.

STM32CubeProgrammer identified `NUCLEO-L476RG`, completed the download, and
reported a successful application start. The USART2 console reported:

- short payload: PASS, 5 bytes transmitted and 5 received;
- binary payload containing `0x00` and non-printable bytes: PASS, 8 transmitted
  and 8 received;
- RX-ring-wrap payload: PASS, 56 transmitted and 56 received;
- total: 69 transmitted and 69 received, with exact binary equality and FIFO
  ordering checked by the target firmware;
- final parity, framing, noise, hardware-overrun, and ring-overflow counters:
  all zero.

The physical loopback suite completed once without an intentional overrun.
Loopback mode was restored to its default-off repository configuration after
the acceptance run.
