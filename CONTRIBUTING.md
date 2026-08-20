# Contributing

Contributions are welcome.

Before opening a pull request:

1. Build with warnings enabled.
2. Run `make test`.
3. Run `make sanitize` on a platform with ASan/UBSan.
4. Add regression coverage for parser or encoding changes.
5. Keep A85X1 canonical behavior backward compatible. Format-breaking changes require a new version marker.

For cross-platform work, prefer C11 and avoid platform-specific APIs unless they are isolated behind a small compatibility block.
