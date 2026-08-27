# Stockfish engine provenance

Framilton Chess does not infer engine identity from a filename. Every APK embeds `assets/BUILD-METADATA.txt` with the engine label, upstream reference, binary SHA-256, license SHA-256, and build date.

## Standalone offline engine reference

- Label: `Stockfish dev-20260810-5062aee5`
- Upstream commit: `5062aee519a1ba262d472d8ab139851ced56573e`
- Android ARM64 engine SHA-256: `acfb4dde7aa0c0d3ed9645c871ef733b471696635cec84fb5be8e8f1d38bbe02`
- `Copying.txt` SHA-256: `3972dc9744f6499f0f9b2dbf76696f2ae7ad8af9b23dde66d6af86c9dfb36986`

## GitHub and networked Fedora builds

The build workflow downloads the official `sf_18` Android ARMv8 tarball and verifies the complete tarball before extracting the engine:

- Release tag: `sf_18`
- Upstream commit: `cb3d4ee9b47d0c5aae855b12379378ea1439675c`
- Official tarball SHA-256: `e2eca54b0e3189ec7de338133c2b34fa8f5cdec3d2473519b414a5cb6815e768`

The workflow attaches that exact tarball to the GitHub release when stable Android signing secrets are configured, preserving GPLv3 source correspondence for the embedded engine.
