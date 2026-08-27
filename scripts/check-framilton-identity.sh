#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"

old_identity="$(printf '%s%s' 'cere' 'lytic')"

content_hits="$(grep -RniI --exclude-dir=.git --exclude-dir=dist --exclude-dir=build --exclude-dir=apk-final "$old_identity" . || true)"
name_hits="$(find . -path './.git' -prune -o -iname "*${old_identity}*" -print)"

if [[ -n "$content_hits" || -n "$name_hits" ]]; then
  echo "Former identity references are not allowed in Framilton Chess." >&2
  [[ -z "$content_hits" ]] || printf '%s\n' "$content_hits" >&2
  [[ -z "$name_hits" ]] || printf '%s\n' "$name_hits" >&2
  exit 1
fi

echo "Framilton identity check: PASS"
