# Contributing

Thanks for improving `ascii85`.

## Before opening a pull request

1. Build with warnings enabled: `make clean && make`.
2. Run the compatibility and regression suite: `make test`.
3. On a sanitizer-capable Unix system, run `make sanitize`.
4. For parser changes, run at least the smoke fuzzer: `make fuzz-smoke`.
5. Keep classic Ascii85 behavior compatible unless the old behavior accepts malformed or unsafe input.
6. Do not change the A85X1 wire format in place. A wire-incompatible change requires a new version marker.

## Style

The project is C11. Keep dependencies minimal and prefer bounded, explicit parsing over C-string assumptions for untrusted encoded input.

Run `make format` when `clang-format` is available.
