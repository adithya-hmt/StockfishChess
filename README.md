# Framilton Chess v3 for Android

**Framilton Chess** is a complete, local-first Android chess app powered by a packaged Stockfish engine. It includes device-local profiles, optional PIN locking, Stockfish play, pass-and-play, offline puzzles, game history, replay, local analysis, persistent settings, and dedicated check/checkmate feedback.

## Product experience

The app has five persistent destinations: Home, Play, Puzzles, History, and Profile. Games, replay, analysis, onboarding, profile creation, unlock, and settings open as focused full-screen destinations.

Gameplay includes tap-to-move and drag-to-move, legal-move markers, hints, board flip, undo, promotion choices, clocks, rematch, resign, local export, castling, en passant, stalemate, timeout, insufficient-material handling, and persistent resume after process recreation.

## Privacy and platform

- Android 10+ (`minSdkVersion 29`).
- ARM64 (`arm64-v8a`) only.
- Fully offline.
- No Android permissions.
- No ads, analytics, telemetry, or account server.
- Stockfish runs as a packaged local UCI process.
- Local state stays inside the app sandbox.
- Haptics use Android view feedback and do not request vibration permission.
- Android package ID: `com.framilton.knight`.

The Framilton package namespace is intentionally a clean identity break from earlier builds. Existing installs under another package ID are treated as separate apps and do not share local application data.

## Framilton identity

The project presents **FRAMILTON** / **Framilton Chess** in user-facing surfaces, Android metadata, release artifacts, signing defaults, and packaging metadata. The launcher and release assets are maintained independently from any previous company identity.

## Architecture

The release uses a freestanding NativeActivity architecture:

- `native/activity.c` — screens, navigation, input, rendering orchestration, clocks, workers, and app behavior.
- `native/ui.c` / `native/ui.h` — drawing primitives, typography, icons, avatars, pieces, and identity rendering.
- `native/core.c` / `native/core.h` — board representation, FEN, move application, undo, check detection, and history helpers.
- `native/engine.c` / `native/engine.h` — local Stockfish UCI process, legal moves, hints, best moves, and analysis.
- `native/state.c` / `native/state.h` — profiles, settings, active-game resume, statistics, puzzles, and history persistence.
- `native/app_model.c` / `native/app_model.h` — isolated model contract and corruption tests.
- `native/build_native.sh` — reproducible ARM64 native build.
- `native/build_apk.sh` — manifest/resource patching, APK assembly, metadata, and signing.

The project does not require Gradle or Android Studio to compile the app.

## Fedora setup, build, and optional install

```bash
./scripts/setup-fedora.sh
```

Build and install on an authorized USB-debugging device:

```bash
./scripts/setup-fedora.sh --install
```

Select a device explicitly:

```bash
./scripts/setup-fedora.sh --install --serial DEVICE_ID
```

The verified APK is written to:

```text
dist/FramiltonChess-v3-offline-arm64.apk
```

The default persistent signing key lives at:

```text
~/.local/share/framilton-chess/release.keystore
```

Back that key up privately if you intend to ship in-place updates to the Framilton package.

## Manual build

Place `stockfish-android-arm64-universal.zip` in the project root or set `STOCKFISH_ARTIFACT`, then run:

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

## Tests

```bash
clang -std=c11 -Wall -Wextra -Werror native/tests/core_test.c native/core.c -o /tmp/core-test
/tmp/core-test

clang -std=c11 -Wall -Wextra -Werror native/tests/state_test.c native/state.c -o /tmp/state-test
/tmp/state-test

clang -std=c11 -Wall -Wextra -Werror -pthread native/tests/engine_parser_test.c native/engine.c native/core.c -o /tmp/engine-test
/tmp/engine-test

clang -std=c11 -Wall -Wextra -Werror native/tests/app_model_test.c native/app_model.c -o /tmp/model-test
/tmp/model-test

python3 native/tests/test_patch_manifest.py
python3 native/tests/test_patch_resources.py
native/tests/render_preview_smoke.sh /tmp/framilton-previews
bash -n native/build_native.sh native/build_apk.sh scripts/setup-fedora.sh scripts/push-github.sh scripts/verify-apk.sh
python3 -m py_compile native/patch_manifest.py native/patch_resources.py
```

## GitHub Actions and stable signing

The included workflow builds and tests pushes to `main`, uploads the APK as a workflow artifact, and publishes `v3.0.0` only when these repository secrets exist:

```text
ANDROID_KEYSTORE_BASE64
ANDROID_KEYSTORE_PASSWORD
ANDROID_KEY_ALIAS
ANDROID_KEY_PASSWORD
```

## Licensing

This source distribution is GPLv3. Stockfish is also GPLv3. The APK includes the Stockfish license, and each distributed APK is accompanied by the exact Stockfish source archive corresponding to its embedded engine.

Exact engine identities and hashes are recorded in `docs/ENGINE-PROVENANCE.md` and inside every APK at `assets/BUILD-METADATA.txt`.
