#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
ARTIFACT="${STOCKFISH_ARTIFACT:-$ROOT/stockfish-android-arm64-universal.zip}"
EXPECTED_ENGINE_SHA256="${EXPECTED_ENGINE_SHA256:-}"
EXPECTED_LICENSE_SHA256="${EXPECTED_LICENSE_SHA256:-}"

[[ -f "$ARTIFACT" ]] || {
  echo "Stockfish artifact not found: $ARTIFACT" >&2
  exit 2
}

for member in stockfish-android-arm64-universal Copying.txt; do
  unzip -Z1 "$ARTIFACT" | grep -Fx "$member" >/dev/null || {
    echo "Stockfish artifact is missing: $member" >&2
    exit 3
  }
done

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
unzip -p "$ARTIFACT" stockfish-android-arm64-universal > "$tmp/stockfish"
unzip -p "$ARTIFACT" Copying.txt > "$tmp/Copying.txt"
chmod 755 "$tmp/stockfish"

engine_sha="$(sha256sum "$tmp/stockfish" | awk '{print $1}')"
license_sha="$(sha256sum "$tmp/Copying.txt" | awk '{print $1}')"

if [[ -n "$EXPECTED_ENGINE_SHA256" && "$engine_sha" != "$EXPECTED_ENGINE_SHA256" ]]; then
  echo "Unexpected Stockfish engine SHA-256: $engine_sha" >&2
  echo "Expected: $EXPECTED_ENGINE_SHA256" >&2
  exit 4
fi
if [[ -n "$EXPECTED_LICENSE_SHA256" && "$license_sha" != "$EXPECTED_LICENSE_SHA256" ]]; then
  echo "Unexpected Stockfish license SHA-256: $license_sha" >&2
  echo "Expected: $EXPECTED_LICENSE_SHA256" >&2
  exit 5
fi

file "$tmp/stockfish" | grep -q 'ELF 64-bit.*ARM aarch64'
readelf -h "$tmp/stockfish" | grep -q 'Machine:.*AArch64'
readelf -h "$tmp/stockfish" | grep -Eq 'Type:.*(EXEC|DYN)'
grep -q 'GNU GENERAL PUBLIC LICENSE' "$tmp/Copying.txt"

embedded_ref="$(strings "$tmp/stockfish" | grep -E '^(5062aee5|cb3d4ee9)$' | head -n 1 || true)"

echo "Stockfish Android artifact verification: PASS"
echo "Engine SHA-256: $engine_sha"
echo "License SHA-256: $license_sha"
[[ -n "$embedded_ref" ]] && echo "Embedded engine reference: $embedded_ref"
