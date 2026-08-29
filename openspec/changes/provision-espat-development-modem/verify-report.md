```yaml
schema: gentle-ai.verify-result/v1
evidence_revision: sha256:95299852b74a0bb017ead5e49d53cbdcad9a5c2c989702b32f5409e1d963725d
verdict: pass
blockers: 0
critical_findings: 0
requirements: 10/10
scenarios: 18/18
test_command: ./tests/run_host_tests.sh && ./tests/test_scripts.sh
test_exit_code: 0
test_output_hash: sha256:f87405cb945c3d244cbcfaf223fec5061f295d168a902cc16834c8c74504e5e0
build_command: /home/cristian/.espressif/python_env/idf5.4_py3.14_env/bin/python ./build.py build
build_exit_code: 0
build_output_hash: sha256:428c8e2d4ccf84cef6c2435aedf764f144ca14cfb0232926d63bc8c1c42df48e
```

# Verification Report: provision-espat-development-modem

## Verdict

**PASS — ready for archive.** All 16 tasks, 10 requirements, and 18 scenarios
have concrete supporting evidence. The retained provisioning evidence proves a
single authorized, verified 16 MiB factory-image write followed by healthy boot,
`AT`, `AT+GMR`, and bidirectional UART1 verification. No CRITICAL, WARNING, or
SUGGESTION issue remains.

Verification was read-only with respect to hardware. No device discovery,
flash, reset, monitor, serial, USB, Wi-Fi, TCP, MQTT, branch, merge, archive, or
commit operation was performed.

## Summary Scorecard

| Dimension | Status | Result |
|---|---|---|
| Completeness | PASS | 16/16 tasks complete; 10/10 requirements and 18/18 scenarios covered |
| Correctness | PASS | Host/script tests, a fresh ESP-AT build, post-build validation, and retained execution evidence agree |
| Coherence | PASS | Implementation follows the guarded read-only/preparation/final-authorization design and stays within ESP32-C6 provisioning scope |
| Strict validation | PASS | `openspec validate provision-espat-development-modem --strict` |
| Archive readiness | PASS | Verification introduces no unresolved blocker |

## Evidence Baseline

- Repository evidence revision: `a00c89391c6dce74ab236fac3cf1a980b6cb7cf7`
- ESP-AT source: tag `v4.1.1.0`, commit
  `7c092f9aee793d6dbfae31e7585637baa14fb4ce`
- ESP-IDF source: commit
  `8ad0d3d8f2faab752635bee36070313c47c07a13`
  (`v5.4.1-643-g8ad0d3d8f2f-dirty` in the retained generated source tree)
- Preserved full backup: 16,777,216 bytes, SHA-256
  `26ab342b257fe9ab5146beaa61a5b7d277457cdf49f1a899d7cdb799b22e2695`
- Filled factory image: 16,777,216 bytes, SHA-256
  `46fe184cc79dac002e55c278be878a7b863744dbdaf4691f2f4bea351a84e806`
- Retained private evidence was read only and remains ignored/untracked.

## Requirement and Scenario Coverage

| Requirement | Status | Concrete evidence |
|---|---|---|
| Positive target identification | PASS | `scripts/espat-device.sh:4-40` matches `1a86:55d3` plus serial `5B14063285`, requires one USB and one serial endpoint, and verifies the by-id link. `tests/test_scripts.sh:13-28` proves correct resolution and serial-mismatch rejection. `docs/espat-device-evidence.md:5-7` and the retained provisioning log record the selected identity, ESP32-C6 revision v0.2, and detected 16 MB flash. Both safe and unsafe scenarios are covered. |
| Non-destructive UART0 evidence | PASS | `docs/espat-read-only-workflow.md` specifies raw 115200 8N1, no flow control, bounded capture, safe reset/ROM entry, and identity rules. `docs/espat-device-evidence.md:8` records the 3,423-byte pre-provision capture and correctly identifies the original app as `hello_world`, not ESP-AT. `private/espat-evidence/uart0-20260814T220900Z-bounded.raw` is retained. Both identifiable and unknown-identity outcomes are explicitly handled. |
| Complete preserved flash backup | PASS | `scripts/espat-read-backup.sh:8-29` reads exactly `0x0..0x1000000`, checks 16,777,216 bytes, readability, reproducible SHA-256, and Git exclusion. The retained backup exists with exactly 16,777,216 bytes and reproduces digest `26ab342b...e2695`; `docs/espat-device-evidence.md:9-12` records its limits and non-semantic hash meaning. Pass and fail scenarios are fail-closed. |
| Supported 16 MB ESP-AT strategy | PASS | `docs/espat-provisioning-manifest.md:12-36` rejects inference from the official 4 MB release and pins a custom v4.1.1.0/ESP-IDF build. The source tag/commits and `IDF_VERSION` were rechecked. A fresh `build.py build` succeeded. `scripts/validate-espat-build.py:55-95` confirms target, DIO/80 MHz/16 MB, feature boundary, and exact generated flash map. |
| Defined UART1 AT interface | PASS | Repository configuration and generated `mfg_nvs.csv` specify UART1, TX 7, RX 6, 115200, CTS/RTS `-1`; `scripts/validate-espat-build.py:151-163` enforces those values. Retained boot evidence reports `uart1 tx:7 rx:6 cts:-1 rts:-1 baudrate:115200`, and the retained NUCLEO diagnostic records successful TX/RX for `AT` and `AT+GMR`. |
| Wi-Fi and TCP only modem scope | PASS | `firmware/espat-16mb/sdkconfig.append` disables MQTT, HTTP, OTA, mDNS, ping, signaling, SmartConfig, WPS, user, Bluetooth, BLE, and BluFi AT support. Generated `sdkconfig` enables base, UART, Wi-Fi, and network command groups and no forbidden group checked by `scripts/validate-espat-build.py:67-84`. Retained verification contains only `AT` and `AT+GMR`; no credentials, Wi-Fi, TCP, or MQTT command was exercised. |
| Reproducible firmware and flash manifest | PASS | `docs/espat-provisioning-manifest.md:10-122` records source/tool versions, configuration, partitions, UART, build commands, flash settings, hashes, and exact workflows. Fresh build outputs match all recorded binary sizes and hashes. `sdkconfig`, `download.config`, and `flasher_args.json` all state 16 MB and agree on the six offset/artifact pairs. `scripts/validate-espat-build.py:97-186` verifies bounds, non-overlap, alignment, partition decoding, OTA initialization, and factory composition. |
| Explicit final destructive authorization gate | PASS | `docs/espat-provisioning-manifest.md:3-8,100-122` records one executed authorization bound to the device, backup digest, image digest, offsets, and command, and requires fresh authorization for any later mutation. The retained log begins `FINAL GATE PASS`, records both bound digests, re-identifies revision v0.2 and 16 MB, and records exactly one factory-image write. No executable destructive command exists in the preparation/device scripts (`scripts/espat-validate-preparation.sh:11-19`). |
| Post-provisioning verification | PASS | Retained UART0 boot evidence reports ESP-IDF boot, DIO, 80 MHz, 16 MB, the expected partition table, OTA0 load, and UART1 mapping. `private/espat-evidence/qualification-at-gmr-pass-20260828.log` records `AT` TX 4/RX 11/result `OK` and `AT+GMR` TX 8/RX 198/result `OK`, with timeout, TX error, overflow, truncation, PE, FE, NE, ORE, and ring-overflow counters all zero. Failure handling is documented without automatic restore. |
| STM32 implementation remains unchanged | PASS | `git diff --name-status origin/main...HEAD` contains provisioning docs, firmware inputs, OpenSpec artifacts, scripts, `.gitignore`, and script tests only; no `src/`, `include/`, board, overlay, Kconfig, CMake, or STM32 USART1 implementation path changed. Host byte-ring tests still pass. |

## 16 MiB Evidence Chain

The physical-capacity and installed-layout conclusion is supported by mutually
consistent, independent evidence:

1. **Detected physical flash:** the retained authorized provisioning log reports
   ESP32-C6 revision v0.2 and `Detected flash size: 16MB`.
2. **Preserved recovery artifact:** the backup is readable, exactly 16,777,216
   bytes, and reproduces SHA-256 `26ab342b...e2695`.
3. **Build configuration:** generated root `sdkconfig` contains ESP32-C6, DIO,
   80 MHz, `CONFIG_ESPTOOLPY_FLASHSIZE="16MB"`, and the custom partition CSV.
4. **Download metadata:** generated `download.config` contains
   `--flash_size 16MB`; `flasher_args.json` reports `flash_size: 16MB` and the
   exact six offset/artifact pairs.
5. **Factory image:** the fresh build regenerated a filled image of exactly
   16,777,216 bytes with SHA-256 `46fe184c...e806`.
6. **Partition layout:** decoded generated partitions are aligned,
   non-overlapping, and end exclusively at `0x1000000` through OTA1
   (`0x830000 + 0x7d0000`).
7. **Verified write:** the retained authorized log reports
   `Wrote 16777216 bytes`, `Hash of data verified`, and a hard reset.
8. **Observed boot:** retained post-write UART0 reports
   `SPI Flash Size : 16MB`, the expected partition offsets/sizes, and a healthy
   OTA0 boot.

### `ESP32C6-4MB` is non-blocking metadata

`AT+GMR` reports `Bin version:v4.1.1.0(ESP32C6-4MB)`. This string is **not used
as physical-capacity evidence**. Source inspection shows `AT+GMR` formats the
parenthetical value through `esp_at_get_current_module_name()`, while the
compiled ESP32-C6 module-name table contains `ESP32C6-4MB`. Generated factory
NVS independently records the requested custom name `ESP32C6-16MB-TCP`, but
that string is not a supported compiled module-table entry and therefore does
not replace the runtime fallback label. The manifest correctly retains this as
a provenance discrepancy instead of renaming firmware or treating it as a
flash probe. The eight direct capacity/layout signals above remain decisive.

## Completed Task Evidence

| Task | Status | Supporting evidence |
|---|---|---|
| 1.1 | PASS | Unique identity resolver plus mismatch test (`scripts/espat-device.sh`, `tests/test_scripts.sh`). |
| 1.2 | PASS | Board UART0 electrical/reset/capture/discovery rules (`docs/espat-read-only-workflow.md`). |
| 1.3 | PASS | Retained raw UART0 capture and recorded `hello_world` identity (`docs/espat-device-evidence.md`). |
| 1.4 | PASS | Retained esptool v5.3.1 evidence for ESP32-C6 v0.2, MAC, manufacturer/device IDs, and 16 MB. |
| 2.1 | PASS | Exact guarded full-range read workflow (`scripts/espat-read-backup.sh`). |
| 2.2 | PASS | Preserved ignored full backup and recorded read command/path (`docs/espat-device-evidence.md`). |
| 2.3 | PASS | Fresh stat and SHA-256 reproduction: 16,777,216 bytes and `26ab342b...e2695`. |
| 2.4 | PASS | Preparation validator rejects tracked private evidence or destructive device/backup scripts (`scripts/espat-validate-preparation.sh`). |
| 3.1 | PASS | Manifest records lack of affirmative 4 MB release compatibility and refuses capacity inference. |
| 3.2 | PASS | v4.1.1.0 commit and ESP-IDF commit are pinned and present in the retained source tree. |
| 3.3 | PASS | Fresh ESP-AT build passed; generated config and NVS prove UART/feature settings. |
| 3.4 | PASS | Post-build validator decoded and checked partitions, bounds, alignments, files, and hashes. |
| 3.5 | PASS | Manifest contains reproducibility, generated metadata, artifact hashes, and exact workflows. |
| 3.6 | PASS | Credential-free check plan and negative authorization/identity tests exist and pass. |
| 4.1 | PASS | Manifest plus retained `FINAL GATE PASS` log bind identity, backup, image, command, risks, and recovery limits. |
| 4.2 | PASS | Retained log proves one verified 16,777,216-byte write/reset; retained boot and NUCLEO logs prove post-provision acceptance without Wi-Fi/MQTT or STM32 changes. |

Task evidence coverage is **16/16**. No completed checkbox lacks supporting
implementation, generated-build, or retained-execution evidence.

## Manifest Claim Audit

Every material claim in `docs/espat-provisioning-manifest.md` was checked
against at least one of: pinned Git metadata, generated build metadata, fresh
build output, independent post-build validation, file size/hash calculation,
retained provisioning output, retained UART0 output, or retained NUCLEO
qualification output. Recorded offsets, sizes, hashes, UART mapping, source
revisions, feature boundary, write scope, and post-provision results agree.

No unsupported success, physical-capacity, rollback, compatibility, or scope
claim remains. The manifest correctly limits hashes to integrity evidence,
states that recovery is destructive and not risk-free, and retains the
`ESP32C6-4MB` label as non-capacity metadata.

## Commands and Results

| Command | Result |
|---|---|
| `./tests/run_host_tests.sh` | PASS — byte-ring empty/full, FIFO, wraparound, overflow preservation, saturation |
| `./tests/test_scripts.sh` | PASS — shell syntax, target mismatch rejection, authorization rejection, non-destructive preparation, and repository safety checks |
| `/home/cristian/.espressif/python_env/idf5.4_py3.14_env/bin/python ./build.py build` in the retained ESP-AT source | PASS — fresh Ninja build; bootloader/app size checks; NVS and 16 MiB filled/unfilled factory images regenerated |
| `scripts/validate-espat-build.py private/espat-source/esp-at-v4.1.1.0` after the build | PASS — generated configuration, files, partitions, OTA initialization, factory composition, UART mapping, sizes, and hashes coherent |
| `openspec validate provision-espat-development-modem --strict` | PASS — change is valid |
| `git diff --check` | PASS |

The required primary test command
`./tests/run_host_tests.sh && ./tests/test_scripts.sh` therefore passed in full.

## Strict TDD Verification

### TDD Compliance

| Check | Result | Details |
|---|---|---|
| TDD evidence reported | ✅ | No separate apply-progress artifact exists; the orchestrator explicitly bound verification to proposal/spec/design/tasks plus retained execution evidence. |
| All tasks have evidence | ✅ | 16/16 tasks map to implementation, generated build, or retained physical-execution evidence. |
| RED confirmed | ➖ | Historical RED output is not retained; final verification did not infer it. |
| GREEN confirmed | ✅ | The exact primary test command passed with exit code 0. |
| Triangulation adequate | ✅ | Identity mismatch, missing authorization, non-destructive safety, generated-layout, build, and retained positive-device paths are independently covered. |
| Safety net | ✅ | The complete host/script suite passed against the final candidate. |

**TDD compliance**: final GREEN and safety-net evidence passed; historical RED evidence is unavailable and is not claimed.

### Test Layer Distribution

| Layer | Tests | Files | Tools |
|---|---:|---:|---|
| Unit | 1 compiled host suite plus script assertions | 2 | C host runner and Bash |
| Integration | Retained offline provisioning/build validators | 2 | Bash and Python |
| E2E | Retained authorized provisioning and UART qualification evidence | 3 evidence records | Hardware evidence captured previously; not rerun |

### Changed File Coverage

Coverage analysis skipped — no coverage tool is configured for the shell-script, documentation, generated-firmware, and retained-evidence workflow.

### Assertion Quality

**Assertion quality**: ✅ All executable assertions call production scripts or compiled production code and verify observable results; no tautology, ghost-loop, type-only, or smoke-only assertion was found.

### Quality Metrics

**Linter**: ✅ `bash -n` passed for all related shell scripts through the primary suite.
**Type Checker**: ➖ Not available for the mixed Bash/Python/firmware-evidence change.

## Issues by Severity

### CRITICAL

None.

### WARNING

None.

### SUGGESTION

None.

## Gaps and Blockers

None. No check was skipped. Hardware actions were intentionally prohibited and
were not needed because complete retained execution evidence was available and
cross-checked against a fresh local ESP-AT build.

## Archive Readiness

**YES.** The change is complete, correct, coherent, strictly valid, and has no
unresolved verification issue. It may proceed to the archive phase.
