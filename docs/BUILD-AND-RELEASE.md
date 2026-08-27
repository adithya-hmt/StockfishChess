# Framilton Chess build and release guide

## Fedora local build

From the project root:

```bash
./scripts/setup-fedora.sh
```

The script installs the required Fedora packages and Android command-line SDK, verifies Stockfish, runs native and renderer tests, builds the ARM64 APK, signs it, and writes:

```text
dist/FramiltonChess-v3-offline-arm64.apk
```

Install on an authorized USB-debugging device with:

```bash
./scripts/setup-fedora.sh --install
```

## Stable local signing

The default release key lives at:

```text
~/.local/share/framilton-chess/release.keystore
```

Default local automation values are:

```text
alias: framiltonlocal
store password: framilton-local-release
key password: framilton-local-release
```

The keystore itself is the sensitive object. Back it up privately; public source synchronization excludes private signing keys.

## GitHub release

Authenticate once with `gh auth login`, then run:

```bash
./scripts/push-github.sh
```

The helper verifies the release key, configures Android signing secrets when requested, synchronizes source, triggers `build.yml`, waits for the workflow, and reports the `v3.0.0` assets.

Use `--skip-signing-secrets` for artifact-only CI. Such builds use a development key and are not suitable for future in-place upgrades.

## Release verification

```bash
./scripts/verify-apk.sh \
  dist/FramiltonChess-v3-offline-arm64.apk \
  stockfish-android-arm64-universal.zip
```

The verifier checks signature presence, archive integrity, `com.framilton.knight`, `Framilton Chess` labels, SDK levels, absence of permissions, native payloads, embedded Stockfish identity, and 16 KiB alignment for the application shared library.

Run `./scripts/check-framilton-identity.sh` before release to reject references to the former identity in tracked source or filenames.
