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
elif [[ -f "/mnt/data/stockfish-android-arm64-universal.zip" ]]; then
  ARTIFACT="/mnt/data/stockfish-android-arm64-universal.zip"
else
  ARTIFACT="$PROJECT/stockfish-android-arm64-universal.zip"
fi
OUT="${APK_OUT:-$PROJECT/StockfishChess-offline-arm64.apk}"
KEYSTORE="${KEYSTORE:-$BUILD/local-signing.keystore}"
STOREPASS="${STOREPASS:-stockfish-local}"
ALIAS="${KEY_ALIAS:-stockfishlocal}"

if [[ ! -f "$ARTIFACT" ]]; then
  echo "Missing official Stockfish artifact: $ARTIFACT" >&2
  exit 2
fi

"$ROOT/build_native.sh"
python3 "$ROOT/patch_manifest.py" "$ROOT/template/AndroidManifest.xml" "$BUILD/AndroidManifest.xml"
python3 "$ROOT/patch_resources.py" "$ROOT/template/resources.arsc" "$BUILD/resources.arsc"

rm -rf "$STAGE"
mkdir -p "$STAGE/lib/arm64-v8a" "$STAGE/res/mipmap" "$STAGE/assets"
cp "$BUILD/AndroidManifest.xml" "$STAGE/AndroidManifest.xml"
cp "$BUILD/resources.arsc" "$STAGE/resources.arsc"
cp "$ROOT/template/res/mipmap/icon.png" "$STAGE/res/mipmap/icon.png"
cp "$ROOT/build/libsf_chess.so" "$STAGE/lib/arm64-v8a/libsf_chess.so"
unzip -p "$ARTIFACT" stockfish-android-arm64-universal > "$STAGE/lib/arm64-v8a/libstockfish.so"
unzip -p "$ARTIFACT" Copying.txt > "$STAGE/assets/stockfish-COPYING.txt"
chmod 755 "$STAGE/lib/arm64-v8a/libstockfish.so"
cat > "$STAGE/assets/NOTICE.txt" <<'EOF'
Stockfish Chess local Android build

Chess engine: Stockfish 18
Upstream: official-stockfish/Stockfish
Tag: sf_18
License: GNU General Public License v3 (see stockfish-COPYING.txt)

This application runs the engine entirely on-device and requests no Android permissions.
The exact Stockfish source used for the packaged engine is distributed alongside this APK.
EOF

rm -f "$OUT"
python3 - "$STAGE" "$OUT" <<'PY'
import pathlib, sys, zipfile
stage = pathlib.Path(sys.argv[1])
out = pathlib.Path(sys.argv[2])
with zipfile.ZipFile(out, 'w', allowZip64=True) as z:
    for p in sorted(stage.rglob('*')):
        if not p.is_file():
            continue
        arc = p.relative_to(stage).as_posix()
        # Binary Android XML/resource table are stored; native binaries are deflated so
        # PackageManager extracts them into the app's executable native-library directory.
        method = zipfile.ZIP_STORED if arc in {'AndroidManifest.xml', 'resources.arsc'} else zipfile.ZIP_DEFLATED
        zi = zipfile.ZipInfo(arc)
        zi.date_time = (2026, 8, 13, 0, 0, 0)
        zi.compress_type = method
        zi.external_attr = (0o755 if arc.startswith('lib/') else 0o644) << 16
        z.writestr(zi, p.read_bytes(), compress_type=method, compresslevel=6)
PY

if [[ ! -f "$KEYSTORE" ]]; then
  keytool -genkeypair -noprompt \
    -keystore "$KEYSTORE" -storepass "$STOREPASS" -keypass "$STOREPASS" \
    -alias "$ALIAS" -keyalg RSA -keysize 2048 -validity 3650 \
    -dname "CN=Stockfish Chess Local, OU=Local, O=Local Build, L=Local, ST=Local, C=IN" >/dev/null 2>&1
fi
jarsigner -keystore "$KEYSTORE" -storepass "$STOREPASS" -keypass "$STOREPASS" \
  -sigalg SHA256withRSA -digestalg SHA-256 "$OUT" "$ALIAS" >/dev/null

echo "$OUT"
