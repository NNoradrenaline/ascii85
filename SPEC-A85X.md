# A85X1 format specification

**Status:** experimental, frozen v1 wire format

A85X1 is an opt-in Base85-derived binary-to-text format. It keeps the 4-byte to 5-character density of Ascii85 while defining one canonical representation and adding accidental-corruption detection.

## Security model

A85X1 is an encoding, not encryption. CRC-32 detects accidental corruption. It is not a MAC, signature, password hash, or defense against an attacker who can modify the encoded text.

## Alphabet

The exact 85-character alphabet, in digit order 0 through 84, is:

```text
0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#$%&()*+,-./=?@[]^_{}~
```

The payload alphabet excludes quotes, backslash, backtick, angle brackets, colon, semicolon, pipe, and whitespace. Colon is reserved as the envelope field separator.

## Block encoding

1. Treat input as raw bytes.
2. Append 0 to 3 zero bytes so the length is divisible by 4.
3. Split into 4-byte blocks.
4. Interpret each block as an unsigned 32-bit big-endian integer.
5. Convert that integer to exactly five radix-85 digits, most-significant digit first.
6. Map the digits through the A85X1 alphabet and concatenate the groups.

Because `85^5 > 2^32`, some five-character strings do not represent a 32-bit block. Decoders MUST reject any group whose value exceeds `0xFFFFFFFF`.

A85X1 has no shorthand forms such as Ascii85 `z` or `y`.

## Envelope

The canonical representation is:

```text
A85X1:<payload>:<pad>:<crc32>
```

Where:

- `A85X1` is the exact version marker.
- `<payload>` contains zero or more alphabet characters and its length is divisible by five.
- `<pad>` is exactly one ASCII digit: `0`, `1`, `2`, or `3`.
- `<crc32>` is exactly eight uppercase hexadecimal digits.

No leading/trailing whitespace, embedded NUL, newline, or other bytes are permitted. The checksum is the final byte of the envelope.

## Padding

`pad` is the number of zero bytes appended to the final 4-byte block before Base85 conversion.

A decoder MUST:

- reject nonzero padding for an empty payload,
- verify that each declared padding byte is zero,
- remove exactly `pad` bytes from the decoded final block.

## Checksum

The checksum is CRC-32/ISO-HDLC over the original **unpadded** bytes:

```text
width   = 32
poly    = 0x04C11DB7
refin   = true
refout  = true
init    = 0xFFFFFFFF
xorout  = 0xFFFFFFFF
check("123456789") = 0xCBF43926
```

It is serialized as eight uppercase hexadecimal digits.

## Strict decoding requirements

A conforming decoder MUST reject an envelope if any of the following is true:

1. The version/prefix is not exactly `A85X1:`.
2. The payload contains a byte outside the fixed alphabet.
3. The payload length is not a multiple of five.
4. Any five-character group exceeds `0xFFFFFFFF`.
5. The pad field is not exactly one digit in `0..3` followed by `:`.
6. An empty payload declares nonzero padding.
7. Any declared padding byte is nonzero.
8. The CRC field is not exactly eight uppercase hexadecimal digits.
9. Any data follows the eighth checksum digit.
10. The computed CRC differs from the encoded CRC.

These rules ensure that each byte string has exactly one canonical A85X1 representation.

## Streaming implementation

The envelope is suitable for incremental parsing. A decoder needs only one pending 4-byte block because padding applies solely to the final block. Implementations that promise atomic verification should buffer decoded output in a temporary stream (or equivalent) until CRC-32 validation succeeds.

The reference implementation follows this strategy: input parsing is constant-memory with respect to encoded input size, while verified output is staged in a temporary stream until checksum validation succeeds.

## Test vectors

| Input | A85X1 |
| --- | --- |
| empty | `A85X1::0:00000000` |
| `00` | `A85X1:00000:3:D202EF8D` |
| `00000000` | `A85X1:00000:0:2144DF1C` |
| `hello` | `A85X1:Xk~0_ZvX%Q:3:3610A686` |
| `hello world` | `A85X1:Xk~0_Zy.MXa%[M(:1:0D4A1185` |
| `00010203` | `A85X1:009C6:0:8BB98613` |
| `FFFFFFFF` | `A85X1:{NsC0:0:FFFFFFFF` |
