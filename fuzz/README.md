# Fuzzing

The repository has three complementary fuzzing paths.

## libFuzzer + ASan/UBSan

This exercises the real incremental A85X parser directly, without spawning a process for every input:

```sh
make fuzz
./fuzz_a85x fuzz/corpus -max_total_time=3600
```

For a short checked smoke run that copies the seed corpus to a temporary directory:

```sh
make fuzz-smoke
```

## Process-level mutation fuzzing

The original black-box smoke harness remains useful because it exercises CLI process behavior, stdout atomicity, and exit codes:

```sh
make fuzz-process
```

## AFL++

Install AFL++ and run:

```sh
make fuzz-afl
afl-fuzz -i fuzz/corpus -o fuzz/findings -- ./ascii85-afl -x -d
```

Malformed A85X inputs are expected to exit non-zero. Crashes, sanitizer findings, hangs, or successful decoding of malformed canonical envelopes are bugs.
