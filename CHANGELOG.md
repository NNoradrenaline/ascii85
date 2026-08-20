# Changelog

## 2.2.0-a85x - 2026-08-20

### Security and correctness

- Replaced the A85X C-string/read-all parser with an incremental state machine.
- Rejects embedded NUL bytes, whitespace, newlines, and all trailing data after the checksum.
- Preserves atomic A85X decoding: checksum failures release no decoded bytes to stdout.
- Keeps only the final decoded block pending so padding can be validated without retaining encoded input in memory.
- Added table-based CRC-32 and A85X digit lookup.

### Portability

- Removed the `getopt.h` dependency and added a portable built-in CLI parser.
- Added Windows binary-mode stdin/stdout handling.
- Added MSVC build/testing to CI.

### Testing and tooling

- Expanded the regression suite from 7 to 10 groups.
- Added embedded-NUL/trailing-data tests, mutation tests, a 1 MiB streaming round trip, and CLI portability tests.
- Added a libFuzzer harness and corpus plus ASan/UBSan smoke fuzzing.
- Added Linux/macOS/Windows CI, CodeQL, and Dependabot for GitHub Actions.

### Packaging

- Added `make install` / `make uninstall` with `PREFIX` and `DESTDIR` support.
- Added a `ascii85(1)` manual page.
- Added automated tagged-release builds for Linux, macOS, and Windows.
- Added security and contribution documentation.

## 2.1.0-a85x - 2026-08-20

- Published the first streaming-parser and cross-platform hardening pass with CMake, CI, installation support, and initial fuzzing scaffolding.

## 2.0.0-a85x - 2026-08-20

- Added A85X1 mode, strict Base85 range validation, CRC-32 verification, expanded tests, and documentation on top of `roukaour/ascii85`.
