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
