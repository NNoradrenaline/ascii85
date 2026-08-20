# Security Policy

## Supported version

The actively maintained line is the current `master` branch and the latest tagged release.

## Reporting a vulnerability

Please use GitHub private vulnerability reporting / a private security advisory for this repository when available. Avoid opening a public issue for a vulnerability that could put users at risk before a fix is ready.

Useful reports include:

- the affected commit or release,
- operating system and compiler,
- a minimal reproducer,
- expected vs. actual behavior,
- sanitizer output or crash logs if available.

## Security notes

A85X is an encoding format. It does not provide confidentiality or cryptographic authentication. Its CRC-32 field detects accidental corruption, not malicious tampering.
