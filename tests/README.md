# Tests

Repository-owned static and host-side workflow checks live here. Firmware and
hardware acceptance procedures are documented separately in `docs/verification.md`.
Run the allocation-free RX ring host tests with:

```sh
./tests/run_host_tests.sh
```

They cover empty/full boundaries, FIFO order, index wraparound, drop-newest
overflow preservation, and saturating counters.
