#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
APK="${1:-$ROOT/dist/CerelyticChess-v3-offline-arm64.apk}"
ENGINE_ARTIFACT="${2:-$ROOT/stockfish-android-arm64-universal.zip}"

[[ -f "$APK" ]] || { echo "APK not found: $APK" >&2; exit 2; }

if command -v apksigner >/dev/null 2>&1; then
  apksigner verify --verbose --print-certs "$APK"
else
  jarsigner -verify "$APK"
fi

PYTHONPATH="$ROOT/native" python3 - "$APK" "$ENGINE_ARTIFACT" <<'PY'
import hashlib
from pathlib import Path
import sys
import zipfile
from patch_manifest import inspect_manifest

apk = Path(sys.argv[1])
engine_artifact = Path(sys.argv[2])
with zipfile.ZipFile(apk) as archive:
    assert archive.testzip() is None
    names = set(archive.namelist())
    required = {
        "AndroidManifest.xml", "resources.arsc", "res/mipmap/icon.png",
        "lib/arm64-v8a/libsf_chess.so", "lib/arm64-v8a/libstockfish.so",
        "assets/NOTICE.txt", "assets/BUILD-METADATA.txt", "assets/stockfish-COPYING.txt",
    }
    assert required <= names, required - names
    manifest = inspect_manifest(archive.read("AndroidManifest.xml"))
    assert manifest["package"] == "com.cerelytic.knight", manifest
    assert manifest["application_label"] == "Cerelytic Chess", manifest
    assert manifest["activity_label"] == "Cerelytic Chess", manifest
    assert manifest["lib_name"] == "sf_chess", manifest
    assert manifest["min_sdk"] == 29 and manifest["target_sdk"] == 29, manifest
    assert manifest["permissions"] == [], manifest
    assert manifest["debuggable"] is False, manifest
    embedded = archive.read("lib/arm64-v8a/libstockfish.so")
    metadata_bytes = archive.read("assets/BUILD-METADATA.txt")
    assert b"version=3.0.0" in metadata_bytes
    metadata = {}
    for line in metadata_bytes.decode("utf-8").splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            metadata[key] = value
    embedded_sha = hashlib.sha256(embedded).hexdigest()
    assert metadata.get("engine_sha256") == embedded_sha, metadata
    assert metadata.get("engine"), metadata
    assert metadata.get("engine_reference"), metadata

if engine_artifact.is_file():
    with zipfile.ZipFile(engine_artifact) as engine_zip:
        upstream = engine_zip.read("stockfish-android-arm64-universal")
    assert hashlib.sha256(embedded).digest() == hashlib.sha256(upstream).digest()

print("Manifest:", manifest)
print("Engine metadata:", metadata["engine"], "|", metadata["engine_reference"])
print("Embedded Stockfish SHA-256:", embedded_sha)
print("APK structure and privacy verification: PASS")
PY

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
unzip -q "$APK" 'lib/arm64-v8a/*' -d "$work"
file "$work/lib/arm64-v8a/libsf_chess.so" "$work/lib/arm64-v8a/libstockfish.so"
readelf -lW "$work/lib/arm64-v8a/libsf_chess.so" | awk '
  $1 == "LOAD" { if ($NF != "0x4000") bad=1; count++ }
  END { if (bad || count < 3) exit 1; print "16 KiB ELF load alignment: PASS" }
'
sha256sum "$APK"
