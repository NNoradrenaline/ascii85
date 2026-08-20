# `ascii85` A85X fork

A modernized, security-focused fork of [`roukaour/ascii85`](https://github.com/roukaour/ascii85), based on upstream commit `069f585cbe7642a3f0f08de71ced639ce48375ba`.

The original project is a compact C command-line Ascii85 encoder/decoder. This fork preserves classic Ascii85 as the default while adding stricter validation, cross-platform behavior, automated testing, fuzzing, and the optional canonical **A85X1** format.

## Highlights

- Classic Ascii85 remains the default, including `<~ ~>` delimiters, `z`, optional `y`, wrapping, delimiter-free mode, and garbage skipping.
- Adds `-x` / `--a85x` for canonical A85X1 envelopes with explicit padding and CRC-32 accidental-corruption detection.
- A85X decoding verifies CRC-32 before releasing decoded bytes.
- Adds `-t` / `--text` for encoding short text directly from the command line.
- Adds `-o` / `--output` for writing results directly to a file.
- Output-file writes are staged and committed only after the operation succeeds, so a failed A85X decode does not overwrite an existing destination.
- Uses binary stdin/stdout mode on Windows so arbitrary byte values are preserved exactly.
- Rejects impossible Base85 tuples above `UINT32_MAX`, one-character final tuples, embedded NULs in A85X envelopes, trailing A85X bytes, and other malformed forms.
- Includes compatibility tests, malformed-input tests, deterministic mutations, all-byte `00..FF` regressions, large streaming tests, ASan/UBSan, libFuzzer, CodeQL, and cross-platform CI.
- Tagged releases automatically build Linux, macOS, and Windows binaries.

## Download and quick start

The easiest way to use `ascii85` is to download a prebuilt binary from the [GitHub Releases page](https://github.com/NNoradrenaline/ascii85/releases/latest).

Choose the archive for your operating system:

- **Windows:** `ascii85-windows-x86_64.zip`
- **Linux:** `ascii85-linux-x86_64.tar.gz`
- **macOS:** `ascii85-macos.tar.gz`

Extract the archive somewhere convenient. On Windows, open Command Prompt or PowerShell in the extracted folder and verify the program:

```bat
ascii85.exe --version
```

Current version:

```text
ascii85 2.3.0-a85x
```

### Fastest way to encode text

A85X:

```bat
ascii85.exe -x --text "hello world"
```

Short form:

```bat
ascii85.exe -x -t "hello world"
```

That immediately prints an `A85X1:...` token. Unlike `echo`, `--text` does not add a newline to the encoded input.

`--text` encodes the command-line bytes supplied by the operating system/runtime. For arbitrary binary data or exact Unicode file bytes, use a file or stdin instead.

### Encode a file with A85X

Using `-o` is the easiest form:

```bat
ascii85.exe -x input.txt -o output.a85x
```

Linux/macOS:

```sh
./ascii85 -x input.txt -o output.a85x
```

Shell redirection still works:

```bat
ascii85.exe -x input.txt > output.a85x
```

The encoded file uses the canonical A85X1 envelope:

```text
A85X1:<payload>:<pad>:<CRC32>
```

### Decode an A85X file

Windows:

```bat
ascii85.exe -x -d output.a85x -o restored.txt
```

Linux/macOS:

```sh
./ascii85 -x -d output.a85x -o restored.txt
```

The decoder verifies the CRC-32 before releasing decoded data. When `-o` is used, output is staged first, so malformed A85X input does not replace an existing destination file.

### Binary files work too

The tool is not limited to text. Images, archives, executables, audio, encrypted blobs, and other binary data are handled byte-for-byte.

```bat
ascii85.exe -x photo.png -o photo.a85x
ascii85.exe -x -d photo.a85x -o restored-photo.png
```

On Windows, stdin and stdout are explicitly switched to binary mode. The regression suite includes a round trip containing every byte value from `0x00` through `0xFF`.

### Classic Ascii85 mode

Leave off `-x` to use classic Ascii85:

```bat
ascii85.exe input.txt -o output.ascii85
ascii85.exe -d output.ascii85 -o restored.txt
```

Direct text also works in classic mode:

```bat
ascii85.exe -n -w0 --text "hello world"
```

### Windows double-click behavior

On Windows, launching `ascii85.exe` interactively with no arguments now shows the help screen instead of immediately printing `<~` and waiting for input.

Piped input is unchanged:

```bat
type input.bin | ascii85.exe -x
```

If you intentionally want interactive stdin, use `-` as the input operand:

```bat
ascii85.exe -x -
```

For all options:

```bat
ascii85.exe --help
```

## Build

### Unix-like systems

```sh
make
```

### Windows with MSVC

From a Visual Studio Developer Command Prompt:

```bat
cl /nologo /std:c11 /W4 /O2 ascii85.c /Fe:ascii85.exe
```

The source automatically switches stdin/stdout to binary mode on Windows.

## Test

```sh
make test
```

AddressSanitizer + UndefinedBehaviorSanitizer:

```sh
make sanitize
```

A short libFuzzer run:

```sh
make fuzz-smoke
```

For a longer fuzzing session:

```sh
make fuzz
./fuzz_a85x fuzz/corpus -max_total_time=3600
```

## Install

```sh
sudo make install
```

Customize the prefix or package into a staging root:

```sh
make install PREFIX=/usr
make install DESTDIR=/tmp/package-root PREFIX=/usr
```

Remove installed files with:

```sh
sudo make uninstall
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

Encode:

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

A85X1 is intentionally strict. Whitespace, NUL bytes, or any other trailing content after the checksum are rejected. A checksum failure produces no decoded output.

See [`SPEC-A85X.md`](SPEC-A85X.md) for the wire-format definition.

## Continuous integration and releases

- `ci.yml` builds/tests with GCC and Clang on Linux, Clang on macOS, and MSVC on Windows.
- Sanitizer and short libFuzzer jobs run on Linux.
- `codeql.yml` performs C/C++ static analysis on pushes, pull requests, and a weekly schedule.
- Dependabot watches GitHub Actions dependencies.
- Pushing a tag matching `v*` triggers `release.yml`, which builds platform binaries and creates a GitHub Release with SHA-256 checksums.

## Compatibility notes

A85X1 is a separate opt-in format. It is not wire-compatible with Adobe Ascii85 and does not claim to be an external standard.

The default Ascii85 decoder is intentionally stricter about malformed encodings than the original upstream program. `--ignore-garbage` skips invalid non-whitespace characters, but it does not make structurally invalid tuples legal.

## Security

CRC-32 is for accidental corruption detection only. It does not provide authentication, secrecy, or protection from a malicious party who can modify both the payload and checksum.

See [`SECURITY.md`](SECURITY.md) for reporting security issues.

## Provenance and license

Original `ascii85` by Remy Oukaour, Copyright (C) 2012-2016, MIT licensed.

This fork retains the upstream MIT license and copyright notice. See [`LICENSE`](LICENSE) and [`UPSTREAM.md`](UPSTREAM.md).
