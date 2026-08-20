#!/usr/bin/env python3
import os
import random
import subprocess
import sys

BIN = sys.argv[1] if len(sys.argv) > 1 else "./ascii85"
CORPUS = [
    b"A85X1::0:00000000",
    b"A85X1:Xk~0_ZvX%Q:3:3610A686",
    b"A85X1:Xk~0_Zy.MXa%[M(:1:0D4A1185",
]

rng = random.Random(0xF02285)

for _ in range(1500):
    data = bytearray(rng.choice(CORPUS))
    for _ in range(rng.randrange(1, 8)):
        op = rng.randrange(4)
        if op == 0 and data:
            data[rng.randrange(len(data))] = rng.randrange(256)
        elif op == 1:
            data.insert(rng.randrange(len(data) + 1), rng.randrange(256))
        elif op == 2 and data:
            del data[rng.randrange(len(data))]
        else:
            data.extend(os.urandom(rng.randrange(0, 4)))
    p = subprocess.run([BIN, "-x", "-d"], input=bytes(data),
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if p.returncode < 0:
        raise SystemExit(f"decoder terminated by signal {-p.returncode}")

print("ok - 1500 mutation cases completed without a signal crash")
