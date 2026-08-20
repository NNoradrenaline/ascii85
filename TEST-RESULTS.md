# Verification results for 2.2.0-a85x

Local verification was performed on 2026-08-20 before publication.

## Functional regression suite

`make test` passed all 10 groups:

- classic Ascii85 compatibility vectors against Python's `base64.a85encode`,
- `z` / optional `y` abbreviation behavior,
- strict malformed classic Ascii85 rejection,
- A85X1 known vectors,
- checksum-corruption atomicity (no unverified stdout),
- canonical A85X1 rejection rules including embedded NUL/trailing bytes,
- 250 randomized payload mutation rejection cases,
- deterministic randomized round trips,
- a 1 MiB A85X streaming round trip,
- portable CLI parsing and file-input behavior.

## Compilers

The full 10-group suite passed with both GCC-compatible `cc` and Clang using C11 warnings enabled.

## Sanitizers

`make sanitize` passed all 10 groups under AddressSanitizer + UndefinedBehaviorSanitizer with leak detection enabled.

## Fuzzing

The libFuzzer smoke target completed roughly 4.9 million executions in a 10-second run under AddressSanitizer + UndefinedBehaviorSanitizer with no crash or sanitizer finding.

The repository also retains the process-level fuzz smoke harness and AFL++ build target for complementary parser testing.

## Build and packaging

- CMake configure/build/CTest completed successfully.
- `make install DESTDIR=... PREFIX=/usr` staged both `usr/bin/ascii85` and `usr/share/man/man1/ascii85.1` successfully.
- The staged binary reported `ascii85 2.2.0-a85x`.

## CI coverage after publication

GitHub Actions is configured to test CMake builds on Linux, macOS, and Windows, explicit GCC/Clang builds, sanitizers, libFuzzer smoke runs, and CodeQL analysis. Tagged releases build platform archives and SHA-256 checksums.
