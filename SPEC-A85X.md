# A85X1 format

A85X1 is an opt-in canonical Base85-derived envelope added by this fork.
Classic Ascii85 behavior remains the default.

## Envelope

```text
A85X1:<payload>:<pad>:<CRC32>
```

- `A85X1` is the exact version marker.
- `payload` consists only of the 85-character alphabet below and has a length divisible by 5.
- `pad` is exactly one digit from `0` through `3`.
- `CRC32` is exactly eight uppercase hexadecimal digits and is calculated over the original, unpadded bytes.
- No bytes may follow the eighth checksum digit. In particular, trailing whitespace and embedded/trailing NUL bytes are invalid.

## Alphabet

```text
0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#$%&()*+,-./=?@[]^_{}~
```

The characters `"`, `'`, `\`, `` ` ``, `<`, `>`, `:`, `;`, and `|` are excluded. `:` is reserved for framing.

## Block transform

1. Append zero bytes until the input length is divisible by 4, recording the number added as `pad`.
2. Interpret each 4-byte block as one unsigned 32-bit big-endian integer.
3. Convert that integer to exactly five radix-85 digits, most significant first.
4. Map the digits through the alphabet above.

Because `85^5 > 2^32`, a decoder MUST reject any five-character group that represents a value above `0xFFFFFFFF`.

## CRC-32

CRC-32/ISO-HDLC is calculated over the original unpadded bytes:

```text
poly    = 0x04C11DB7
refin   = true
refout  = true
init    = 0xFFFFFFFF
xorout  = 0xFFFFFFFF
check("123456789") = 0xCBF43926
```

The checksum is for accidental corruption detection only. It is not authentication and is not cryptographically secure.

## Canonical decoding

The decoder rejects:

- wrong or missing version marker,
- extra/missing envelope fields,
- payload lengths not divisible by 5,
- alphabet violations,
- radix-85 values above `UINT32_MAX`,
- invalid padding counts,
- non-zero bytes in declared padding,
- lowercase or malformed CRC fields,
- CRC mismatches,
- trailing bytes of any kind.

The reference CLI parses A85X input incrementally. It keeps only one decoded 4-byte block pending so final padding can be validated. Decoded bytes are written to a temporary verification stream and are copied to stdout only after CRC-32 validation succeeds. This avoids loading the encoded input into memory while preserving the no-unverified-output guarantee.
