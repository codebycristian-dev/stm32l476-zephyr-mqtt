# ESP32-C6 read-only preparation

This workflow applies only to the independently connected nanoESP32-C6. Never
open the ST-LINK endpoint or operate on the STM32L476RG. The required USB
identity is WCH `1a86:55d3`, serial `5B14063285`; a changing `ttyACM` number is
not identity. Every operation reruns `scripts/espat-device.sh` and requires the
stable `/dev/serial/by-id` link to resolve back to the matched USB device.

UART0 is internally connected to the WCH bridge (ESP32-C6 GPIO16 TX and GPIO17
RX according to Espressif's ESP32-C6 hardware guide). Logic is 3.3 V; grounds
must be common and 5 V must not be applied to GPIO. Capture at 115200 8N1, raw,
without flow control. A single manual EN/reset is permitted for one bounded
20-second capture; do not repeatedly reset. Preserve raw bytes locally under
`private/espat-evidence/`, record UTC start/end and settings, and treat firmware
identity as unknown unless the output explicitly names it.

For ROM download mode, hold BOOT (GPIO9 low), pulse EN once, then release BOOT.
This changes boot mode but does not erase or write flash. Re-enumerate USB and
rerun the resolver after the transition. Read-only identification uses esptool
`chip-id` and `flash-id`; stop unless the chip is ESP32-C6 revision 0.2 and the
detected physical flash is 16 MB.

The only approved acquisition command is wrapped by:

```sh
scripts/espat-read-backup.sh --read-only-backup
```

It explicitly reads offset `0x000000` for `0x1000000` bytes, then checks
readability, exact size 16,777,216, SHA-256 reproducibility, and Git exclusion.
Backups, logs, and raw UART output are local-only, potentially secret-bearing,
must not be printed, parsed for credentials, committed, or uploaded. The hash
establishes captured-artifact integrity, not semantic firmware validity.

Authoritative references: Espressif's [ESP32-C6 hardware connection guide](https://docs.espressif.com/projects/esp-at/en/latest/esp32c6/Get_Started/Hardware_connection.html),
[released firmware matrix](https://docs.espressif.com/projects/esp-at/en/latest/esp32c6/AT_Binary_Lists/esp_at_binaries.html),
and [download layout](https://docs.espressif.com/projects/esp-at/en/latest/esp32c6/Get_Started/Downloading_guide.html).
