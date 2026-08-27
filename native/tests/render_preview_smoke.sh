#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT_DIR="${1:-$(mktemp -d)}"
BIN="${TMPDIR:-/tmp}/framilton-v3-preview-test"
mkdir -p "$OUT_DIR"

clang -std=c11 -O2 -Wall -Wextra -Werror -pthread \
  -DSF_UI_PREVIEW=1 \
  -I"$ROOT" \
  "$ROOT/tests/preview_main.c" \
  "$ROOT/activity.c" "$ROOT/core.c" "$ROOT/state.c" "$ROOT/engine.c" "$ROOT/ui.c" \
  -o "$BIN"

# Every screen id plus the special checkmate preview state (99). Rendering the
# onboarding and local-lock surfaces matters too; a polished home screen is not
# much comfort when the profile picker has quietly turned into abstract art.
for screen in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 99; do
  "$BIN" "$OUT_DIR/screen-${screen}.ppm" "$screen"
done

python3 - "$OUT_DIR" <<'PY'
from pathlib import Path
import sys

root = Path(sys.argv[1])
paths = sorted(root.glob("screen-*.ppm"))
assert len(paths) == 16, paths
for path in paths:
    data = path.read_bytes()
    assert data.startswith(b"P6\n360 800\n255\n"), path
    payload = data.split(b"\n", 3)[3]
    assert len(payload) == 360 * 800 * 3, (path, len(payload))
    sample = payload[::997]
    assert len(set(sample)) > 8, (path, len(set(sample)))
print("UI preview smoke tests: PASS")
PY
