# Cerelytic Chess v3 for Android

Cerelytic Chess is a complete, local-first Android chess product powered by a packaged Stockfish engine. It combines a premium original interface with device-local profiles, optional local PIN locking, pass-and-play, offline puzzles, game history, replay, analysis, persistent settings, and a dedicated red checkmate experience.

![Cerelytic Chess v3](brand/release-banner.svg)

## Product experience

### Home and navigation

The app uses five persistent destinations:

- **Home** — resume the active game, challenge Stockfish, start pass-and-play, open puzzles, and view local progress.
- **Play** — choose Stockfish or pass-and-play, side, strength, clock, hints, and legal-move markers.
- **Puzzles** — five packaged tactical positions that work without a connection.
- **History** — saved local results with replay and analysis entry points.
- **Profile** — rating, record, puzzle progress, profile switching, local lock, and settings.

Games, replay, analysis, onboarding, profile creation, unlock, and settings open as focused full-screen destinations.

### Gameplay

- Correct original Staunton-style vector pieces.
- Tap-to-move and drag-to-move interaction.
- Selected-square, last-move, legal-move, capture, hint, check, and checkmate states.
- Smooth piece animation with a reduced-motion option.
- Easy, Medium, and Hard Stockfish strengths.
- No-clock, 3-minute, 5-minute, and 10-minute games.
- Undo, hint, board flip, restart, resign, rematch, analysis, and local move export.
- Castling, en passant, all four promotion choices, stalemate, timeout, and insufficient-material handling.
- Full-turn persistence and resume after process recreation.

### Check and checkmate feedback

- Check uses amber king-square feedback.
- Checkmate uses a red king square, thin red board frame, `CHECKMATE` status, result sheet, and restrained haptic feedback.
- Result actions include Rematch, Analyze, Export, and Home.

### Local profiles and lock

Profiles and the optional four-digit PIN are device-local. This is deliberately not cloud authentication:

- No email account.
- No remote password database.
- No OAuth token.
- No backend session.
- No network permission.

The PIN is a convenience lock for separating local profiles, not a replacement for Android device encryption or a secure password manager. Resetting local data removes profiles, settings, active games, and history. The complete boundary is documented in `docs/SECURITY-AND-PRIVACY.md`.

## Privacy and platform

- Android 10+ (`minSdkVersion 29`).
- ARM64 (`arm64-v8a`) only.
- Fully offline.
- No Android permissions.
- No ads, analytics, telemetry, or account server.
- Stockfish runs as a packaged local UCI process.
- Local state is stored inside the app sandbox.
- Haptics use Android view feedback and do not request vibration permission.
- V3 uses the new package ID `com.cerelytic.knight`, so it installs beside the legacy prototype instead of failing on its unrelated signing key.

## Brand assets

The `brand/` directory contains the complete original Cerelytic visual system:

- Geometric knight-and-C mark and wordmark.
- Adaptive launcher foreground and monochrome icon artwork.
- Splash lockup and release banner.
- Five abstract local avatar illustrations.
- Twelve original Staunton piece SVGs.
- Palette sheet and brand guidelines.

The Android launcher icon is generated from this system and is packaged under `native/template/res/mipmap/icon.png`.

## Architecture

This release intentionally stays with the proven freestanding NativeActivity architecture instead of importing a large mobile framework merely to move pawns around:

- `native/activity.c` — screen state, navigation, input, rendering orchestration, clocks, worker lifecycle, and app behavior.
- `native/ui.c` / `native/ui.h` — anti-aliased drawing primitives, typography, icons, avatars, pieces, and brand rendering.
- `native/core.c` / `native/core.h` — board representation, FEN, move application, undo, check detection, and history helpers.
- `native/engine.c` / `native/engine.h` — local Stockfish UCI process, legal moves, hints, best moves, and analysis.
- `native/state.c` / `native/state.h` — profiles, settings, active-game resume, statistics, puzzles, and history persistence.
- `native/app_model.c` / `native/app_model.h` — isolated model contract and corruption tests.
- `native/build_native.sh` — reproducible ARM64 Android shared-library build.
- `native/build_apk.sh` — manifest/resource patching, APK assembly, branding metadata, and signing.

The project does not require Gradle or Android Studio to compile the app. Android SDK Build-Tools are used for modern APK alignment and signing when available.

The reasons this implementation retained NativeActivity instead of following the original Compose proposal are recorded in `docs/ARCHITECTURE-DECISION.md`, including the explicit v3.0 product limits.

## One-command Fedora setup, build, and optional install

```bash
./scripts/setup-fedora.sh
```

Build and install on one authorized USB-debugging device:

```bash
./scripts/setup-fedora.sh --install
```

Select a device explicitly:

```bash
./scripts/setup-fedora.sh --install --serial DEVICE_ID
```

The script installs the Fedora toolchain and official Android command-line tools, downloads and verifies stable Stockfish 18 when no packaged engine is present, identifies bundled development engines accurately, runs every test suite, renders UI smoke previews, builds the APK, verifies its archive and signature, and writes:

```text
dist/CerelyticChess-v3-offline-arm64.apk
```

A persistent local signing key is retained at:

```text
~/.local/share/cerelytic-chess/release.keystore
```

Back that file up privately. Android updates require every future APK to use the same key. Computers are exceptionally literal about this.

## Manual build

Place `stockfish-android-arm64-universal.zip` in the project root or set `STOCKFISH_ARTIFACT`, then run. The APK metadata records the exact embedded engine SHA-256 and the supplied engine label/reference rather than relying on wishful filenames.

```bash
chmod +x native/build_native.sh native/build_apk.sh
./native/build_apk.sh
```

Useful signing variables:

```text
KEYSTORE=/private/path/release.keystore
STOREPASS=...
KEYPASS=...
KEY_ALIAS=...
REQUIRE_EXISTING_KEYSTORE=1
APKSIGNER=/path/to/android-sdk/build-tools/36.0.0/apksigner
```

Without `APKSIGNER`, the script uses `jarsigner` as an Android-10-compatible local fallback.

## Tests

```bash
clang -std=c11 -Wall -Wextra -Werror \
  native/tests/core_test.c native/core.c -o /tmp/core-test
/tmp/core-test

clang -std=c11 -Wall -Wextra -Werror \
  native/tests/state_test.c native/state.c -o /tmp/state-test
/tmp/state-test

clang -std=c11 -Wall -Wextra -Werror -pthread \
  native/tests/engine_parser_test.c native/engine.c native/core.c -o /tmp/engine-test
/tmp/engine-test

clang -std=c11 -Wall -Wextra -Werror \
  native/tests/app_model_test.c native/app_model.c -o /tmp/model-test
/tmp/model-test

python3 native/tests/test_patch_manifest.py
python3 native/tests/test_patch_resources.py
STOCKFISH_ARTIFACT=/path/to/stockfish-android-arm64-universal.zip \
  native/tests/test_stockfish_artifact.sh
STOCKFISH_HOST_BIN=/path/to/host/stockfish \
  native/tests/test_stockfish_integration.sh
native/tests/render_preview_smoke.sh /tmp/cerelytic-previews
bash -n native/build_native.sh native/build_apk.sh scripts/setup-fedora.sh scripts/push-github.sh scripts/verify-apk.sh native/tests/test_stockfish_artifact.sh native/tests/test_stockfish_integration.sh
python3 -m py_compile native/patch_manifest.py native/patch_resources.py
```

## GitHub Actions and stable signing

The included workflow builds and tests every push to `main`, uploads the APK as a workflow artifact, and publishes `v3.0.0` only when these repository secrets exist:

```text
ANDROID_KEYSTORE_BASE64
ANDROID_KEYSTORE_PASSWORD
ANDROID_KEY_ALIAS
ANDROID_KEY_PASSWORD
```

The Fedora publishing helper configures those four repository secrets from the private local release key, synchronizes the clean source tree, triggers the workflow, waits for verification, and prints the release assets:

```bash
gh auth login
./scripts/push-github.sh
```

Use `--skip-signing-secrets` only when an artifact-only CI build is acceptable. When stable secrets are absent, CI still produces a test APK artifact with a temporary development key but deliberately does not replace the public release, because shipping randomly signed updates is how one manufactures uninstall instructions.

## Licensing

This source distribution is GPLv3. Stockfish is also GPLv3. The APK includes the Stockfish license, and each distributed APK is accompanied by the exact Stockfish source archive corresponding to its embedded engine. GitHub release builds pin stable Stockfish 18; the offline Fedora bundle may carry a fully identified development build together with its exact source when a network-free build is required.

The NativeActivity manifest/resource template is derived from the public `cnlohr/rawdrawandroid` sample under its stated MIT/X11 or NewBSD licensing choice. Original Cerelytic branding, avatar art, piece art, and interface assets are included in this repository and distributed under the project license.

Exact engine identities and hashes are recorded in `docs/ENGINE-PROVENANCE.md` and inside every APK at `assets/BUILD-METADATA.txt`.
