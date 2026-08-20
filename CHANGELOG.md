# Changelog

## 2.3.0-a85x - 2026-08-20

### Usability

- Added `-t` / `--text` for encoding short command-line strings without piping or creating an input file.
- Added `-o` / `--output` for writing encoded or decoded output directly to a file.
- Output-file writes are staged in a temporary stream and only committed after the codec operation succeeds, so a malformed A85X input does not overwrite an existing destination.
- On Windows, launching `ascii85.exe` interactively with no arguments now shows help instead of immediately beginning a classic Ascii85 stream. Piped stdin remains unchanged; use `ascii85.exe -` to request interactive stdin explicitly.

### Portability and regression coverage

- Kept Windows stdin/stdout in explicit binary mode for raw byte preservation.
- Expanded the regression suite with direct-text tests, staged-output tests, same-file transformations, and an explicit `00..FF` binary pipe round trip covering every possible byte value.
- Routed codec output through explicit `FILE *` streams so stdout and staged file destinations share the same encoding/decoding paths.

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
