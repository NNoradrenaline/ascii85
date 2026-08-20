# Security policy

## Supported version

Security fixes are applied to the current `master` branch and the newest tagged release.

## Reporting a vulnerability

Please do **not** publish exploitable details in a public issue before a fix is available.

Use GitHub's private vulnerability reporting feature for this repository when available. Include:

- the affected version or commit,
- a minimal reproducer or malformed input,
- expected versus observed behavior,
- compiler/OS details,
- sanitizer output or crash trace when relevant.

For parser bugs, please mention whether the issue affects classic Ascii85, A85X1, or both.

## Security boundaries

A85X1 CRC-32 is accidental-corruption detection only. It does not provide cryptographic integrity, authentication, or secrecy. An attacker who can alter an A85X1 payload can also recompute its CRC.
