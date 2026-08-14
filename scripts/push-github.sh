#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
REPO="${GITHUB_REPOSITORY:-adithya-hmt/StockfishChess}"
REMOTE="https://github.com/${REPO}.git"
MESSAGE="feat: ship Cerelytic Chess v3 local product"
WAIT=1
CONFIGURE_SIGNING=1
KEYSTORE_PATH="${KEYSTORE:-$HOME/.local/share/cerelytic-chess/release.keystore}"
STORE_PASSWORD="${STOREPASS:-cerelytic-local-release}"
KEY_PASSWORD="${KEYPASS:-$STORE_PASSWORD}"
KEY_ALIAS_VALUE="${KEY_ALIAS:-cerelyticlocal}"

while (($#)); do
  case "$1" in
    --repo)
      shift
      [[ $# -gt 0 ]] || { echo "--repo needs OWNER/NAME" >&2; exit 2; }
      REPO="$1"
      REMOTE="https://github.com/${REPO}.git"
      ;;
    --message)
      shift
      [[ $# -gt 0 ]] || { echo "--message needs text" >&2; exit 2; }
      MESSAGE="$1"
      ;;
    --no-wait) WAIT=0 ;;
    --skip-signing-secrets) CONFIGURE_SIGNING=0 ;;
    --keystore)
      shift
      [[ $# -gt 0 ]] || { echo "--keystore needs a file" >&2; exit 2; }
      KEYSTORE_PATH="$1"
      ;;
    -h|--help)
      cat <<USAGE
Usage: $0 [--repo OWNER/NAME] [--message TEXT] [--no-wait]
          [--keystore FILE] [--skip-signing-secrets]

Synchronizes this source tree to the repository's main branch. By default it
also configures the four stable signing secrets from the private local key, so
GitHub Actions can publish an upgrade-compatible v3.0.0 release.
USAGE
      exit 0
      ;;
    *) echo "Unknown argument: $1" >&2; exit 2 ;;
  esac
  shift
done

for tool in git gh rsync; do
  command -v "$tool" >/dev/null || { echo "$tool is required" >&2; exit 1; }
done
gh auth status >/dev/null 2>&1 || {
  echo "GitHub CLI is not authenticated. Run: gh auth login" >&2
  exit 2
}
gh repo view "$REPO" >/dev/null 2>&1 || {
  echo "Repository is unavailable to this account: $REPO" >&2
  exit 3
}

if (( CONFIGURE_SIGNING )); then
  if [[ ! -f "$KEYSTORE_PATH" && -f "$ROOT/signing/cerelytic-v3-release.keystore" ]]; then
    KEYSTORE_PATH="$ROOT/signing/cerelytic-v3-release.keystore"
  fi
  [[ -f "$KEYSTORE_PATH" ]] || {
    echo "Stable signing key not found: $KEYSTORE_PATH" >&2
    echo "Build locally once with scripts/setup-fedora.sh or pass --skip-signing-secrets." >&2
    exit 3
  }
  keytool -list -keystore "$KEYSTORE_PATH" -storepass "$STORE_PASSWORD" -alias "$KEY_ALIAS_VALUE" >/dev/null
  encoded="$(base64 -w 0 "$KEYSTORE_PATH")"
  gh secret set ANDROID_KEYSTORE_BASE64 --repo "$REPO" --body "$encoded"
  gh secret set ANDROID_KEYSTORE_PASSWORD --repo "$REPO" --body "$STORE_PASSWORD"
  gh secret set ANDROID_KEY_ALIAS --repo "$REPO" --body "$KEY_ALIAS_VALUE"
  gh secret set ANDROID_KEY_PASSWORD --repo "$REPO" --body "$KEY_PASSWORD"
  unset encoded
  echo "Stable Android signing secrets configured for $REPO."
fi

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

git clone "$REMOTE" "$work/repo"
rsync -a --delete \
  --exclude .git \
  --exclude dist \
  --exclude '*.apk' \
  --exclude '*.apk.sha256' \
  --exclude '*.keystore' \
  --exclude signing \
  --exclude stockfish-android-arm64-universal.zip \
  --exclude stockfish-android-armv8.tar \
  --exclude native/build \
  --exclude native/apk-final \
  --exclude native/__pycache__ \
  --exclude '__pycache__' \
  "$ROOT/" "$work/repo/"

cd "$work/repo"
git add -A
if git diff --cached --quiet; then
  echo "GitHub already matches the Cerelytic Chess v3 source tree."
  gh workflow run build.yml --repo "$REPO" --ref main
else
  git commit -m "$MESSAGE"
  git push origin HEAD:main
fi

printf 'Source pushed to %s\n' "$REMOTE"
if (( ! WAIT )); then
  echo "GitHub Actions was triggered; not waiting because --no-wait was supplied."
  exit 0
fi

printf '\nWaiting for GitHub Actions to manufacture the APK, because apparently even a pawn needs CI now.\n'
sleep 4
run_id="$(gh run list --repo "$REPO" --workflow build.yml --branch main --limit 1 --json databaseId --jq '.[0].databaseId')"
[[ -n "$run_id" && "$run_id" != "null" ]] || { echo "Could not locate the build workflow run." >&2; exit 4; }
gh run watch "$run_id" --repo "$REPO" --exit-status

if gh release view v3.0.0 --repo "$REPO" >/dev/null 2>&1; then
  gh release view v3.0.0 --repo "$REPO" --json name,url,assets \
    --jq '{name, url, assets: [.assets[].name]}'
else
  cat <<MSG
The workflow completed, but v3.0.0 was not published. Configure the four stable
signing secrets documented in README.md, then rerun the Build Cerelytic Chess v3
APK workflow. An APK workflow artifact is still available from run $run_id.
MSG
fi
