# Build and release guide

## Fedora local build

From the project root:

```bash
./scripts/setup-fedora.sh
```

The script installs the required Fedora packages and Android command-line SDK, verifies Stockfish, runs the native and renderer test suites, builds the ARM64 APK, signs it with the persistent local key, and writes:

```text
dist/CerelyticChess-v3-offline-arm64.apk
```

Install on an authorized USB-debugging device with:

```bash
./scripts/setup-fedora.sh --install
```

## Stable local signing

The default release key lives at:

```text
~/.local/share/cerelytic-chess/release.keystore
```

The default bundle credentials are intentionally documented for automation because the key itself is the sensitive object:

```text
alias: cerelyticlocal
store password: cerelytic-local-release
key password: cerelytic-local-release
```

Back up the keystore privately. Public source archives and repository synchronization explicitly exclude it.

## GitHub release

Authenticate once:

```bash
gh auth login
```

Then run:

```bash
./scripts/push-github.sh
```

The helper:

1. verifies the private release key;
2. configures the four Android signing secrets on the target repository;
3. synchronizes the clean source tree;
4. triggers `build.yml`;
5. waits for the workflow;
6. prints the `v3.0.0` release assets.

Use `--skip-signing-secrets` for artifact-only CI. Such builds use an ephemeral development key and are not suitable for future in-place upgrades.

## Release verification

```bash
./scripts/verify-apk.sh \
  dist/CerelyticChess-v3-offline-arm64.apk \
  stockfish-android-arm64-universal.zip
```

The verifier checks signature presence, archive integrity, package identity, app labels, SDK levels, absence of permissions, native payloads, embedded Stockfish identity, and 16 KiB alignment for the application shared library.
