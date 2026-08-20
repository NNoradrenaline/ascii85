# Fuzzing

## Smoke fuzzing

`make fuzz-smoke` mutates a small A85X corpus and checks that malformed input does not crash the process.

## AFL++

Install AFL++ and run:

```sh
make fuzz-afl
afl-fuzz -i fuzz/corpus -o fuzz/findings -- ./ascii85-afl -x -d
```

Malformed A85X inputs are expected to exit non-zero. Sanitizer findings, signals, hangs, or unexpected successful decoding of malformed canonical envelopes are the interesting cases.
