# `ascii85` A85X fork

This is a source fork of [roukaour/ascii85](https://github.com/roukaour/ascii85), based on upstream commit `069f585cbe7642a3f0f08de71ced639ce48375ba`.

The original program is a compact C command-line Ascii85 encoder/decoder. This fork keeps classic Ascii85 as the default so existing command lines remain useful, while tightening malformed-input handling and adding an optional canonical **A85X1** mode.

## What changed

- Keeps standard Ascii85 encode/decode behavior and `<~ ~>` delimiters.
- Keeps `z`, optional `y`, wrapping, delimiter-free mode, and garbage-skipping mode.
- Fixes the upstream extra-operand check.
- Rejects impossible 5-digit Ascii85 values above `UINT32_MAX` instead of allowing 32-bit arithmetic wraparound.
- Rejects one-character final tuples and `z`/`y` inside partial tuples.
- Uses safer character classification and stricter option parsing.
- Detects read/write failures.
- Adds `--version`.
- Adds `-x` / `--a85x` for A85X1.
- A85X1 adds explicit final padding, a fixed safer alphabet, canonical framing, and CRC-32 accidental-corruption detection.
- A85X decoding verifies the entire checksum before releasing decoded bytes to stdout.
- Adds automated compatibility, malformed-input, known-vector, corruption, and randomized round-trip tests.

## Build

```sh
make
```

## Test

```sh
make test
```

For AddressSanitizer + UndefinedBehaviorSanitizer:

```sh
make sanitize
```

## Classic Ascii85

```sh
printf 'hello world' | ./ascii85
printf '<~BOu!rD]j7BEbo7~>' | ./ascii85 -d
```

Delimiter-free:

```sh
printf 'hello world' | ./ascii85 -n -w 0
```

## A85X1

```sh
printf 'hello world' | ./ascii85 --a85x
```

Output:

```text
A85X1:Xk~0_Zy.MXa%[M(:1:0D4A1185
```

Decode:

```sh
printf 'A85X1:Xk~0_Zy.MXa%[M(:1:0D4A1185' | ./ascii85 --a85x --decode
```

See [`SPEC-A85X.md`](SPEC-A85X.md) for the format definition.

## Compatibility notes

A85X1 is a separate opt-in format. It is not wire-compatible with Adobe Ascii85 and does not claim to be a standard.

The default Ascii85 decoder is intentionally stricter about malformed encodings than the original upstream program. `--ignore-garbage` still skips invalid non-whitespace characters, but it does not make structurally invalid tuples legal.

## Provenance and license

Original `ascii85` by Remy Oukaour, Copyright (C) 2012-2016, MIT licensed.

This fork retains the upstream MIT license and copyright notice. See [`LICENSE`](LICENSE).
