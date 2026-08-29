# ESP32-C6 preparation evidence

Timestamp: 2026-08-15 03:45:53 UTC (host timezone America/Bogota).

- Selected bridge: WCH USB Single Serial `1a86:55d3`, serial `5B14063285`, stable path `/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B14063285-if00`, resolving to `/dev/ttyACM1` at capture time.
- Isolation: ST-LINK `0483:374b` was separately enumerated at `/dev/ttyACM0` and was not opened.
- esptool 5.3.1: ESP32-C6 QFN40 revision v0.2, 40 MHz crystal; flash manufacturer `0x68`, device `0x4018`, detected physical size 16 MB.
- UART0: 115200 8N1, raw, no flow control, one bounded 20-second listen; 3,423 bytes captured locally. ROM `esp32c6-20220919`; reset reason `SW_CPU`; SPI fast-flash boot; existing app `hello_world` version `1`, built July 11 2026 with ESP-IDF v6.0.2; DIO 16 MB; partitions NVS at `0x9000` size `0x6000`, PHY at `0xf000` size `0x1000`, factory app at `0x10000` size `0x100000`. This evidence establishes that the app is hello_world, not ESP-AT.
- Backup: `private/espat-backups/esp32c6-20260815T035358Z-full-16MiB.bin`; exact size 16,777,216 bytes; SHA-256 `26ab342b257fe9ab5146beaa61a5b7d277457cdf49f1a899d7cdb799b22e2695`. The file is readable, the digest rechecked successfully, and the path is ignored/untracked. It may contain secrets and must not be inspected, committed, printed, parsed for credentials, or uploaded.
- Exact read command: `python -m esptool --chip esp32c6 --port <matched-by-id> --baud 921600 read-flash 0x000000 0x1000000 esp32c6-20260815T035358Z-full-16MiB.bin`.

The SHA-256 proves integrity of the preserved captured artifact; it does not prove semantic firmware validity. An earlier interrupted partial read is retained only in the ignored private directory and is not an accepted backup.
