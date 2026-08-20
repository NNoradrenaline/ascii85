# `ascii85` A85X fork

A modernized fork of [`roukaour/ascii85`](https://github.com/roukaour/ascii85), based on upstream commit `069f585cbe7642a3f0f08de71ced639ce48375ba`.

Classic Ascii85 remains the default. The fork adds a strict, canonical **A85X1** mode plus parser hardening, portability work, CI, fuzzing hooks, installation support, and release automation.

## Highlights

- Classic Ascii85 compatibility, including `<~ ~>`, `z`, optional `y`, wrapping, delimiter-free mode, and garbage skipping.
- A85X1 via `-x` / `--a85x`.
- Streaming A85X input parser with constant memory usage apart from the verified-output temporary file.
- Strict rejection of trailing bytes and embedded NULs.
- CRC-32 verification before any decoded A85X bytes are released to stdout.
- Table-driven CRC-32 and A85X character decoding.
- Impossible radix-85 values above `UINT32_MAX` are rejected.
- Portable built-in option parser, removing the `getopt.h` dependency.
- Windows stdin/stdout binary mode handling.
- Make and CMake builds.
- Linux, macOS, and Windows GitHub Actions CI.
- ASan/UBSan testing.
- AFL++ fuzz target and deterministic mutation smoke fuzzing.
- `make install`, `make uninstall`, and a man page.
- Tag-driven release workflow for downloadable binaries.

## Build

### Make

```sh
make
make test
make sanitize
```

### CMake

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

## Install

```sh
sudo make install
```

Use `PREFIX` or `DESTDIR` when packaging:

```sh
make install PREFIX=/usr
make install DESTDIR="$PWD/pkg"
```

## Classic Ascii85

```sh
printf 'hello world' | ./ascii85
printf '<~BOu!rD]j7BEbo7~>' | ./ascii85 -d
printf 'hello world' | ./ascii85 -n -w 0
```

## A85X1

```sh
printf 'hello world' | ./ascii85 --a85x
```

Produces:

```text
A85X1:Xk~0_Zy.MXa%[M(:1:0D4A1185
```

Decode:

```sh
printf 'A85X1:Xk~0_Zy.MXa%[M(:1:0D4A1185' | ./ascii85 --a85x --decode
```

See [`SPEC-A85X.md`](SPEC-A85X.md).

## Fuzzing

Quick deterministic mutation smoke test:

```sh
make fuzz-smoke
```

AFL++:

```sh
make fuzz-afl
afl-fuzz -i fuzz/corpus -o fuzz/findings -- ./ascii85-afl -x -d
```

See [`fuzz/README.md`](fuzz/README.md).

## CI and releases

`.github/workflows/ci.yml` builds and tests on Linux, macOS, and Windows and runs sanitizer coverage on Linux.

`.github/workflows/release.yml` packages platform binaries when a `v*` tag is pushed and creates a GitHub release.

## Security model

A85X is an encoding, not encryption. CRC-32 detects accidental corruption only. It is not a MAC or digital signature.

Security issues should be reported using GitHub's private vulnerability reporting / security advisory feature when available. See [`SECURITY.md`](SECURITY.md).

## Provenance and license

Original `ascii85` by Remy Oukaour, Copyright (C) 2012-2016, MIT licensed.

This fork retains the upstream MIT license and copyright notice. See [`LICENSE`](LICENSE) and [`UPSTREAM.md`](UPSTREAM.md).
