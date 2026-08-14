#!/usr/bin/env bash
set -Eeuo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
PUBLISH=0
SETUP_ARGS=()

pause_on_error() {
  code=$?
  echo
  echo "Cerelytic Chess v3 stopped with exit code $code."
  echo "The command above contains the real error; it has not been hidden behind a cheerful spinner."
  if [[ -t 0 ]]; then
    read -r -p "Press Enter to leave this runner..." _ || true
  fi
  exit "$code"
}
trap pause_on_error ERR

while (($#)); do
  case "$1" in
    --publish) PUBLISH=1 ;;
    *) SETUP_ARGS+=("$1") ;;
  esac
  shift
done

cd "$ROOT"
chmod +x scripts/setup-fedora.sh scripts/push-github.sh scripts/verify-apk.sh
./scripts/setup-fedora.sh "${SETUP_ARGS[@]}"
./scripts/verify-apk.sh "$ROOT/dist/CerelyticChess-v3-offline-arm64.apk" "$ROOT/stockfish-android-arm64-universal.zip"

if (( PUBLISH )); then
  if ! gh auth status >/dev/null 2>&1; then
    echo
    echo "GitHub authentication is required once. A browser/device flow will open."
    gh auth login
  fi
  ./scripts/push-github.sh
fi

echo
echo "Cerelytic Chess v3 is ready:"
echo "$ROOT/dist/CerelyticChess-v3-offline-arm64.apk"
if (( ! PUBLISH )); then
  echo "Run ./RUN-NOW.sh --publish to configure GitHub signing secrets, push source, and publish v3.0.0."
fi
