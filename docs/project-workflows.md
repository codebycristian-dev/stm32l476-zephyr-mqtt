# Project workflows

This repository is a standalone Zephyr 4.4.0 C application for
`nucleo_l476rg`. `src/` owns application code, `include/` project headers,
`tests/` host/static checks, `scripts/` operational workflows, and `docs/`
project guidance. Generated files stay under `build/nucleo_l476rg/`.
Zephyr's compiler-capability cache is redirected to `build/.zephyr-cache/`;
the external workspace and host compiler cache are not written by the build.

The supported host uses `/home/cristian/.venvs/zephyr` and the existing,
read-only-input workspace `/home/cristian/zephyrproject`. The board-defined
USART2 console remains on the ST-LINK virtual COM port at 115200 baud, 8 data
bits, no parity, and 1 stop bit. USART1 is outside this foundation.

Run these separate commands from any working directory:

```sh
/path/to/repository/scripts/doctor.sh
/path/to/repository/scripts/build.sh
/path/to/repository/scripts/flash.sh --dry-run
/path/to/repository/scripts/monitor.sh --device /dev/ttyACM0
```

`doctor.sh` only reads prerequisites. `build.sh` always performs a pristine
build and prints Zephyr's FLASH and RAM report. `flash.sh` requires an existing
repository-local image and only programs hardware when explicitly invoked
without `--dry-run`. `monitor.sh` never changes serial-device permissions.
