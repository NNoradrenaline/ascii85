# Test results

Validated locally on 2026-08-20 after the 2.1.0-a85x hardening update.

## GCC / default compiler

`make test`

- 9 test groups passed
- includes classic Ascii85 compatibility vectors
- A85X known vectors
- malformed/canonical envelope rejection
- embedded NUL and trailing-byte rejection
- CRC corruption rejection with no unverified stdout
- 1 MiB+ streaming round trip
- randomized round trips

## Sanitizers

`make sanitize`

- 9 test groups passed under AddressSanitizer + UndefinedBehaviorSanitizer
- no sanitizer findings

## Fuzz smoke

`make fuzz-smoke`

- 1,500 deterministic mutation cases
- no signal crashes

## CMake / CTest

Release CMake build completed successfully and `ctest` passed 1/1 tests.

## Clang warnings

Built with:

```sh
clang -std=c11 -Wall -Wextra -Wconversion -Wshadow -pedantic -Werror -O2
```

The build completed with zero warnings, and all 9 test groups passed.
