# Stockfish Chess Android (offline, ARM64)

A small `android.app.NativeActivity` chess app that runs Stockfish entirely on the phone. It contains no Java/Kotlin bytecode and requests no Android permissions.

## What it builds

- Android 10+ (`minSdkVersion 29`), ARM64 only.
- Human plays White against Stockfish.
- Easy, Medium, and Hard engine settings.
- Tap-to-move legal chess with castling, en passant, promotion-to-queen, and New Game.
- Stockfish runs as a local UCI process packaged inside the APK.

The exact engine used by the provided APK is **Stockfish dev-20260810-5062aee5**, commit `5062aee519a1ba262d472d8ab139851ced56573e`.

## Build

Requirements: `clang` with LLD, Python 3, `unzip`, a JDK containing `keytool` and `jarsigner`, and the exact Stockfish Android artifact ZIP distributed alongside this source.

Place `stockfish-android-arm64-universal.zip` beside this README's parent project directory, or set `STOCKFISH_ARTIFACT` to its path. Then run:

```bash
./native/build_apk.sh
```

The script compiles the ARM64 NativeActivity library without an Android SDK, packages the official Android 29 Stockfish executable, creates a local signing key if necessary, and emits `StockfishChess-offline-arm64.apk`.

This sideload build deliberately targets API 29 so the locally available JAR/v1 signing path remains valid. It is not a Google Play submission build.

## Tests

```bash
clang -std=c11 -Wall -Wextra -Werror native/tests/core_test.c native/core.c -o /tmp/sf-core-test
/tmp/sf-core-test
python3 native/tests/test_patch_manifest.py
python3 native/tests/test_patch_resources.py
bash -n native/build_native.sh native/build_apk.sh
python3 -m py_compile native/patch_manifest.py native/patch_resources.py
```

## Licensing and template attribution

Stockfish is GPLv3. `assets/stockfish-COPYING.txt` in the APK contains its license, and the exact upstream artifact distributed alongside the APK contains the full corresponding Stockfish source.

The tiny binary Android manifest/resource-table template was adapted from the public `cnlohr/rawdrawandroid` NativeActivity sample build. Its Makefile states the project is available under MIT/X11 or NewBSD at the user's choice. No rawdrawandroid native application code is included here.
