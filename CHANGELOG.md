# Changelog

## 2.1.0-a85x - 2026-08-20

- Reworked A85X decoding into a streaming input parser.
- Fixed canonical parsing around embedded NULs and trailing bytes.
- Added table-driven CRC-32 and A85X digit lookup.
- Removed the `getopt.h` dependency with a portable option parser.
- Added Windows binary stdio handling.
- Added CMake support and cross-platform CI.
- Added ASan/UBSan CI coverage.
- Added AFL++ fuzzing hooks and deterministic mutation smoke fuzzing.
- Added install/uninstall targets and a man page.
- Added tag-driven binary release automation.
- Expanded tests with CLI, malformed-envelope, and 1 MiB streaming cases.

## 2.0.0-a85x

- Added A85X1.
- Hardened classic Ascii85 decoding.
- Added automated compatibility and fuzz-style round-trip tests.
