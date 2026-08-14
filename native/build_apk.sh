#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
PROJECT="$(cd "$ROOT/.." && pwd)"
BUILD="$ROOT/apk-final"
STAGE="$BUILD/stage"

if [[ -n "${STOCKFISH_ARTIFACT:-}" ]]; then
  ARTIFACT="$STOCKFISH_ARTIFACT"
elif [[ -f "$PROJECT/stockfish-android-arm64-universal.zip" ]]; then
  ARTIFACT="$PROJECT/stockfish-android-arm64-universal.zip"
else
  ARTIFACT="$PROJECT/stockfish-android-arm64-universal.zip"
fi

OUT="${APK_OUT:-$PROJECT/CerelyticChess-v3-offline-arm64.apk}"
KEYSTORE="${KEYSTORE:-$BUILD/local-signing.keystore}"
STOREPASS="${STOREPASS:-cerelytic-local-release}"
KEYPASS="${KEYPASS:-$STOREPASS}"
ALIAS="${KEY_ALIAS:-cerelyticlocal}"
ENGINE_LABEL="${ENGINE_LABEL:-Stockfish (bundled)}"
ENGINE_REF="${ENGINE_REF:-}"
BUILD_DATE="${BUILD_DATE:-$(date -u +%F)}"
APKSIGNER_BIN="${APKSIGNER:-$(command -v apksigner 2>/dev/null || true)}"
REQUIRE_EXISTING_KEYSTORE="${REQUIRE_EXISTING_KEYSTORE:-0}"

if [[ ! -f "$ARTIFACT" ]]; then
  echo "Missing Stockfish artifact: $ARTIFACT" >&2
  echo "Place stockfish-android-arm64-universal.zip in the project root or set STOCKFISH_ARTIFACT." >&2
  exit 2
fi

for member in stockfish-android-arm64-universal Copying.txt; do
  # Do not use grep -q here: with pipefail enabled, grep exits as soon as it
  # finds a match and unzip can report SIGPIPE for archives with more entries.
  if ! unzip -Z1 "$ARTIFACT" | grep -Fx "$member" >/dev/null; then
    echo "Stockfish artifact is missing required member: $member" >&2
    exit 2
  fi
done

ENGINE_SHA256="$(unzip -p "$ARTIFACT" stockfish-android-arm64-universal | sha256sum | awk '{print $1}')"
ENGINE_LICENSE_SHA256="$(unzip -p "$ARTIFACT" Copying.txt | sha256sum | awk '{print $1}')"
if [[ -z "$ENGINE_REF" ]]; then
  ENGINE_REF="embedded engine SHA-256 ${ENGINE_SHA256}"
fi

mkdir -p "$(dirname "$OUT")" "$BUILD"
"$ROOT/build_native.sh"
python3 "$ROOT/patch_manifest.py" "$ROOT/template/AndroidManifest.xml" "$BUILD/AndroidManifest.xml"
python3 "$ROOT/patch_resources.py" "$ROOT/template/resources.arsc" "$BUILD/resources.arsc"

rm -rf "$STAGE"
mkdir -p "$STAGE/lib/arm64-v8a" "$STAGE/res/mipmap" "$STAGE/assets/brand"
cp "$BUILD/AndroidManifest.xml" "$STAGE/AndroidManifest.xml"
cp "$BUILD/resources.arsc" "$STAGE/resources.arsc"
cp "$ROOT/template/res/mipmap/icon.png" "$STAGE/res/mipmap/icon.png"
cp "$ROOT/build/libsf_chess.so" "$STAGE/lib/arm64-v8a/libsf_chess.so"
unzip -p "$ARTIFACT" stockfish-android-arm64-universal > "$STAGE/lib/arm64-v8a/libstockfish.so"
unzip -p "$ARTIFACT" Copying.txt > "$STAGE/assets/stockfish-COPYING.txt"
chmod 755 "$STAGE/lib/arm64-v8a/libstockfish.so"

if [[ -f "$PROJECT/brand/brand-guidelines.md" ]]; then
  cp "$PROJECT/brand/brand-guidelines.md" "$STAGE/assets/brand/brand-guidelines.md"
fi
if [[ -f "$PROJECT/brand/cerelytic-mark.svg" ]]; then
  cp "$PROJECT/brand/cerelytic-mark.svg" "$STAGE/assets/brand/cerelytic-mark.svg"
fi

cat > "$STAGE/assets/NOTICE.txt" <<EOF_NOTICE
Cerelytic Chess v3.0.0

A fully local Android chess product with profiles, optional local PIN lock,
Stockfish play, pass-and-play, offline puzzles, game history, replay,
on-device analysis, persistent settings, haptics, and checkmate feedback.

Chess engine: ${ENGINE_LABEL}
Upstream: official-stockfish/Stockfish
Reference: ${ENGINE_REF}
Engine license: GNU General Public License v3 (see stockfish-COPYING.txt)

All engine computation and application data stay on-device. The app requests
no network, account, contacts, storage, or vibration permission. Haptics use
Android view feedback. The exact Stockfish source corresponding to the packaged
engine is distributed with the release or Fedora bundle.
EOF_NOTICE

cat > "$STAGE/assets/BUILD-METADATA.txt" <<EOF_METADATA
product=Cerelytic Chess
version=3.0.0
package=com.cerelytic.knight
minimum_android=29
abi=arm64-v8a
engine=${ENGINE_LABEL}
engine_reference=${ENGINE_REF}
engine_sha256=${ENGINE_SHA256}
engine_license_sha256=${ENGINE_LICENSE_SHA256}
build_date=${BUILD_DATE}
EOF_METADATA

rm -f "$OUT"
python3 - "$STAGE" "$OUT" <<'PY'
import pathlib
import sys
import zipfile

stage = pathlib.Path(sys.argv[1])
out = pathlib.Path(sys.argv[2])
with zipfile.ZipFile(out, "w", allowZip64=True) as archive:
    for path in sorted(stage.rglob("*")):
        if not path.is_file():
            continue
        arc = path.relative_to(stage).as_posix()
        method = zipfile.ZIP_STORED if arc in {"AndroidManifest.xml", "resources.arsc"} else zipfile.ZIP_DEFLATED
        info = zipfile.ZipInfo(arc)
        info.date_time = (2026, 8, 14, 0, 0, 0)
        info.compress_type = method
        info.external_attr = (0o755 if arc.startswith("lib/") else 0o644) << 16
        archive.writestr(info, path.read_bytes(), compress_type=method, compresslevel=6)
PY

if [[ ! -f "$KEYSTORE" ]]; then
  if [[ "$REQUIRE_EXISTING_KEYSTORE" == "1" ]]; then
    echo "Signing keystore not found: $KEYSTORE" >&2
    echo "A stable release build must provide KEYSTORE, STOREPASS, KEYPASS, and KEY_ALIAS." >&2
    exit 3
  fi
  echo "WARNING: generating a local development signing key at $KEYSTORE" >&2
  echo "Keep this keystore to install future updates over this APK." >&2
  mkdir -p "$(dirname "$KEYSTORE")"
  keytool -genkeypair -noprompt \
    -keystore "$KEYSTORE" -storepass "$STOREPASS" -keypass "$KEYPASS" \
    -alias "$ALIAS" -keyalg RSA -keysize 2048 -validity 3650 \
    -dname "CN=Cerelytic Chess Local, OU=Android, O=Cerelytic, L=Local, ST=Local, C=IN" \
    >/dev/null 2>&1
fi

if [[ -n "$APKSIGNER_BIN" && -x "$APKSIGNER_BIN" ]]; then
  zipalign_bin="$(dirname "$APKSIGNER_BIN")/zipalign"
  if [[ -x "$zipalign_bin" ]]; then
    unaligned="${OUT}.unaligned"
    mv "$OUT" "$unaligned"
    "$zipalign_bin" -f 4 "$unaligned" "$OUT"
    rm -f "$unaligned"
  fi
  "$APKSIGNER_BIN" sign \
    --ks "$KEYSTORE" \
    --ks-key-alias "$ALIAS" \
    --ks-pass "pass:$STOREPASS" \
    --key-pass "pass:$KEYPASS" \
    --v1-signing-enabled true \
    --v2-signing-enabled true \
    --v3-signing-enabled true \
    "$OUT"
else
  jarsigner -keystore "$KEYSTORE" -storepass "$STOREPASS" -keypass "$KEYPASS" \
    -sigalg SHA256withRSA -digestalg SHA-256 "$OUT" "$ALIAS" >/dev/null
fi

printf '%s\n' "$OUT"
