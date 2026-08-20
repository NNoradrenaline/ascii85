#!/usr/bin/env python3
import base64
import os
import random
import subprocess
import sys
import tempfile
import zlib

BIN = sys.argv[1] if len(sys.argv) > 1 else "./ascii85"
ALPHABET = b"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#$%&()*+,-./=?@[]^_{}~"


def run(args, data=b"", ok=True):
    p = subprocess.run(
        [BIN, *args],
        input=data,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if ok and p.returncode != 0:
        raise AssertionError(
            f"command failed: {args}\n"
            f"stderr={p.stderr.decode(errors='replace')}"
        )
    if not ok and p.returncode == 0:
        raise AssertionError(
            f"command unexpectedly succeeded: {args}\nstdout={p.stdout!r}"
        )
    return p


def read_bytes(path):
    with open(path, "rb") as f:
        return f.read()


def a85x_ref(data: bytes) -> bytes:
    pad = (-len(data)) % 4
    padded = data + b"\x00" * pad
    out = bytearray(b"A85X1:")
    for i in range(0, len(padded), 4):
        n = int.from_bytes(padded[i:i + 4], "big")
        digits = [0] * 5
        for j in range(4, -1, -1):
            n, r = divmod(n, 85)
            digits[j] = ALPHABET[r]
        out.extend(digits)
    out.extend(f":{pad}:{zlib.crc32(data) & 0xffffffff:08X}".encode())
    return bytes(out)


def test_ascii85_vectors():
    vectors = [
        b"",
        b"hello",
        b"hello world",
        b"\0\0\0\0",
        b"    ",
        bytes(range(256)),
    ]
    for data in vectors:
        enc = run(["-n", "-w", "0"], data).stdout
        expected = base64.a85encode(
            data, adobe=False, foldspaces=False, wrapcol=0, pad=False
        )
        assert enc == expected, (data, enc, expected)
        assert run(["-d", "-n"], enc).stdout == data


def test_ascii85_abbreviations():
    assert run(["-n", "-w", "0"], b"\0\0\0\0").stdout == b"z"
    assert run(["-n", "-w", "0", "-y"], b"    ").stdout == b"y"
    assert run(["-d", "-n"], b"z").stdout == b"\0\0\0\0"
    assert run(["-d", "-n"], b"y").stdout == b"    "


def test_ascii85_strict_rejections():
    run(["-d", "-n"], b"!", ok=False)
    run(["-d", "-n"], b"uuuuu", ok=False)
    run(["-d", "-n"], b"!z!!!", ok=False)
    run([], b"a", ok=True)
    run(["-d"], b"<~!!!!~x", ok=False)
    p = subprocess.run([BIN, "one", "two"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    assert p.returncode != 0


def test_a85x_vectors():
    vectors = [
        b"",
        b"\0",
        b"\0\0\0\0",
        b"hello",
        b"hello world",
        bytes([0, 1, 2, 3]),
        b"\xff\xff\xff\xff",
        bytes(range(256)),
    ]
    for data in vectors:
        enc = run(["-x"], data).stdout
        assert enc == a85x_ref(data), (data, enc, a85x_ref(data))
        assert run(["-x", "-d"], enc).stdout == data


def test_a85x_corruption_is_atomic():
    data = b"this should not silently corrupt"
    enc = bytearray(run(["-x"], data).stdout)
    start = len(b"A85X1:")
    enc[start] = ord("1") if enc[start] != ord("1") else ord("2")
    p = run(["-x", "-d"], bytes(enc), ok=False)
    assert p.stdout == b"", "decoder released unverified plaintext"


def test_a85x_canonical_rules():
    valid_empty = b"A85X1::0:00000000"
    bad = [
        b"A85X1::1:00000000",
        b"A85X1:0:0:00000000",
        b"A85X1:~~~~~:0:00000000",
        b"A85X1::0:0000000a",
        b"A85X2::0:00000000",
        b"A85X1::0:00000000:EXTRA",
        valid_empty + b"\x00TRAILING",
        valid_empty + b"\n",
        b"A85X1::0:0000000",
        b"A85X1::0:000000000",
        b"A85X1::0:",
        b"A85X1::00:00000000",
        b"A85X1::0;00000000",
        b"A85X1:\x00:0:00000000",
    ]
    for token in bad:
        p = run(["-x", "-d"], token, ok=False)
        assert p.stdout == b"", token


def test_a85x_mutations():
    rng = random.Random(0xBAD85)
    for _ in range(250):
        n = rng.randrange(1, 513)
        data = bytes(rng.getrandbits(8) for _ in range(n))
        token = bytearray(run(["-x"], data).stdout)
        payload_start = len(b"A85X1:")
        payload_end = token.index(ord(":"), payload_start)
        pos = rng.randrange(payload_start, payload_end)
        old = token[pos]
        choices = [c for c in ALPHABET if c != old]
        token[pos] = rng.choice(choices)
        p = run(["-x", "-d"], bytes(token), ok=False)
        assert p.stdout == b""


def test_fuzz_round_trips():
    rng = random.Random(0xA85F00D)
    lengths = list(range(0, 260)) + [511, 512, 513, 1024, 4096, 65536]
    for n in lengths:
        data = bytes(rng.getrandbits(8) for _ in range(n))
        enc = run(["-x"], data).stdout
        assert enc == a85x_ref(data)
        assert run(["-x", "-d"], enc).stdout == data

    for _ in range(500):
        n = rng.randrange(0, 2049)
        data = bytes(rng.getrandbits(8) for _ in range(n))
        enc = run(["-n", "-w", "0"], data).stdout
        assert run(["-d", "-n"], enc).stdout == data


def test_large_streaming_round_trip():
    block = bytes(range(256))
    data = block * 4096
    enc = run(["-x"], data).stdout
    assert run(["-dx"], enc).stdout == data


def test_cli_portability_parser():
    assert run(["--version"]).stdout.startswith(b"ascii85 2.3.0")
    help_text = run(["--help"]).stdout
    assert b"A85X1" in help_text
    assert b"--text" in help_text
    assert b"--output" in help_text
    assert run(["-nw0"], b"hello").stdout == run(["-n", "-w", "0"], b"hello").stdout
    assert run(["--no-delims", "--wrap=0"], b"hello").stdout == run(["-n", "-w0"], b"hello").stdout
    run(["--does-not-exist"], ok=False)
    run(["-xw0"], ok=False)

    with tempfile.TemporaryDirectory() as td:
        path = os.path.join(td, "input.bin")
        data = b"file input\x00with binary\xff"
        with open(path, "wb") as f:
            f.write(data)
        token = subprocess.run(
            [BIN, "-x", path], stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True
        ).stdout
        assert run(["-dx"], token).stdout == data


def test_direct_text_input():
    assert run(["-x", "--text", "hello"]).stdout == a85x_ref(b"hello")
    assert run(["-x", "--text="]).stdout == a85x_ref(b"")
    expected = base64.a85encode(b"hello", adobe=False, foldspaces=False, wrapcol=0, pad=False)
    assert run(["-n", "-w0", "-t", "hello"]).stdout == expected
    run(["-d", "--text", "hello"], ok=False)
    run(["-x", "--text", "hello", "input.txt"], ok=False)


def test_output_files_are_staged():
    with tempfile.TemporaryDirectory() as td:
        out_path = os.path.join(td, "message.a85x")
        p = run(["-x", "--text", "hello", "-o", out_path])
        assert p.stdout == b""
        assert read_bytes(out_path) == a85x_ref(b"hello")

        restored = os.path.join(td, "restored.bin")
        p = run(["-dx", "-o", restored], read_bytes(out_path))
        assert p.stdout == b""
        assert read_bytes(restored) == b"hello"

        with open(restored, "wb") as f:
            f.write(b"KEEP-EXISTING-DATA")
        broken = bytearray(a85x_ref(b"hello"))
        payload_start = len(b"A85X1:")
        broken[payload_start] = ord("1") if broken[payload_start] != ord("1") else ord("2")
        run(["-dx", "-o", restored], bytes(broken), ok=False)
        assert read_bytes(restored) == b"KEEP-EXISTING-DATA"

        same_path = os.path.join(td, "same-file.bin")
        data = bytes(range(256))
        with open(same_path, "wb") as f:
            f.write(data)
        run(["-x", same_path, "-o", same_path])
        assert read_bytes(same_path) == a85x_ref(data)
        run(["-dx", same_path, "-o", same_path])
        assert read_bytes(same_path) == data


def test_binary_stdio_regression():
    data = bytes(range(256))
    classic = run(["-n", "-w0"], data).stdout
    assert classic == base64.a85encode(
        data, adobe=False, foldspaces=False, wrapcol=0, pad=False
    )
    assert run(["-dn"], classic).stdout == data

    token = run(["-x"], data).stdout
    assert token == a85x_ref(data)
    assert run(["-dx"], token).stdout == data


def main():
    tests = [
        test_ascii85_vectors,
        test_ascii85_abbreviations,
        test_ascii85_strict_rejections,
        test_a85x_vectors,
        test_a85x_corruption_is_atomic,
        test_a85x_canonical_rules,
        test_a85x_mutations,
        test_fuzz_round_trips,
        test_large_streaming_round_trip,
        test_cli_portability_parser,
        test_direct_text_input,
        test_output_files_are_staged,
        test_binary_stdio_regression,
    ]
    for test in tests:
        test()
        print(f"ok - {test.__name__}")
    print(f"\n{len(tests)} test groups passed")


if __name__ == "__main__":
    main()
