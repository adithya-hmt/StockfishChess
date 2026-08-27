#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SDK_ROOT="${ANDROID_HOME:-$HOME/Android/Sdk}"
CMDLINE_VERSION="15859902"
CMDLINE_ZIP="commandlinetools-linux-${CMDLINE_VERSION}_latest.zip"
CMDLINE_URL="https://dl.google.com/android/repository/${CMDLINE_ZIP}"
CMDLINE_SHA256="4e4c464f145a7512b57d088ac6c278c03c9eea610886b35a5e0804e74eedf583"
STOCKFISH_TAG="sf_18"
STOCKFISH_TAR="stockfish-android-armv8.tar"
STOCKFISH_SHA256="e2eca54b0e3189ec7de338133c2b34fa8f5cdec3d2473519b414a5cb6815e768"
STOCKFISH_URL="https://github.com/official-stockfish/Stockfish/releases/download/${STOCKFISH_TAG}/${STOCKFISH_TAR}"
DEV_ENGINE_SHA256="acfb4dde7aa0c0d3ed9645c871ef733b471696635cec84fb5be8e8f1d38bbe02"
ENGINE_LABEL_VALUE="Stockfish (bundled)"
ENGINE_REF_VALUE=""
INSTALL_APK=0
SKIP_SDK=0
SKIP_PACKAGES=0
DEVICE_SERIAL=""

usage() {
  cat <<USAGE
Usage: $0 [--install] [--serial DEVICE] [--skip-sdk] [--skip-packages]

  --install        Install the verified APK on an authorized Android device.
  --serial DEVICE  Select one adb device when several are connected.
  --skip-sdk       Skip Android command-line SDK installation.
  --skip-packages  Skip dnf package installation; use the existing toolchain.
USAGE
}

while (($#)); do
  case "$1" in
    --install) INSTALL_APK=1 ;;
    --serial)
      shift
      [[ $# -gt 0 ]] || { echo "--serial needs a device ID" >&2; exit 2; }
      DEVICE_SERIAL="$1"
      ;;
    --skip-sdk) SKIP_SDK=1 ;;
    --skip-packages) SKIP_PACKAGES=1 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown argument: $1" >&2; usage >&2; exit 2 ;;
  esac
  shift
done

if [[ ! -r /etc/os-release ]]; then
  echo "Cannot identify the operating system." >&2
  exit 1
fi
# shellcheck disable=SC1091
source /etc/os-release
if [[ "${ID:-}" != "fedora" ]]; then
  echo "This setup script targets Fedora; detected ${PRETTY_NAME:-unknown}." >&2
  exit 1
fi

if (( ! SKIP_PACKAGES )); then
  printf '\n==> Installing Fedora packages\n'
  sudo dnf install -y \
    clang lld binutils file git gh rsync curl python3 unzip zip tar \
    java-21-openjdk-devel android-tools
fi

if (( ! SKIP_SDK )); then
  printf '\n==> Installing the official Android command-line SDK\n'
  cache_dir="${XDG_CACHE_HOME:-$HOME/.cache}/cerelytic-chess"
  archive="$cache_dir/$CMDLINE_ZIP"
  mkdir -p "$cache_dir" "$SDK_ROOT/cmdline-tools"

  if [[ ! -f "$archive" ]] || ! echo "$CMDLINE_SHA256  $archive" | sha256sum -c - >/dev/null 2>&1; then
    rm -f "$archive"
    curl -fL --retry 3 --retry-delay 2 "$CMDLINE_URL" -o "$archive"
  fi
  echo "$CMDLINE_SHA256  $archive" | sha256sum -c -

  temp_dir="$(mktemp -d)"
  trap 'rm -rf "$temp_dir"' EXIT
  unzip -q "$archive" -d "$temp_dir"
  rm -rf "$SDK_ROOT/cmdline-tools/latest"
  mkdir -p "$SDK_ROOT/cmdline-tools/latest"
  cp -a "$temp_dir/cmdline-tools/." "$SDK_ROOT/cmdline-tools/latest/"

  sdkmanager="$SDK_ROOT/cmdline-tools/latest/bin/sdkmanager"
  export ANDROID_HOME="$SDK_ROOT"
  export ANDROID_SDK_ROOT="$SDK_ROOT"
  export PATH="$SDK_ROOT/platform-tools:$SDK_ROOT/cmdline-tools/latest/bin:$PATH"

  mkdir -p "$HOME/.config/environment.d"
  cat > "$HOME/.config/environment.d/90-android.conf" <<ENV
ANDROID_HOME=$SDK_ROOT
ANDROID_SDK_ROOT=$SDK_ROOT
ENV
  profile_line='export PATH="$ANDROID_HOME/platform-tools:$ANDROID_HOME/cmdline-tools/latest/bin:$PATH"'
  grep -Fqx "$profile_line" "$HOME/.bashrc" 2>/dev/null || printf '\n%s\n' "$profile_line" >> "$HOME/.bashrc"

  yes | "$sdkmanager" --licenses >/dev/null || true
  "$sdkmanager" \
    "platform-tools" \
    "platforms;android-36" \
    "build-tools;36.0.0"
fi

export ANDROID_HOME="$SDK_ROOT"
export ANDROID_SDK_ROOT="$SDK_ROOT"
export PATH="$SDK_ROOT/platform-tools:$SDK_ROOT/cmdline-tools/latest/bin:$PATH"

APKSIGNER_BIN="${APKSIGNER:-}"
if [[ -z "$APKSIGNER_BIN" && -d "$SDK_ROOT/build-tools" ]]; then
  APKSIGNER_BIN="$(find "$SDK_ROOT/build-tools" -type f -name apksigner -print | sort -V | tail -n 1)"
fi

detect_engine_metadata() {
  local artifact="$1" engine_sha embedded_ref=""
  engine_sha="$(unzip -p "$artifact" stockfish-android-arm64-universal | sha256sum | awk '{print $1}')"
  if command -v strings >/dev/null 2>&1; then
    embedded_ref="$(unzip -p "$artifact" stockfish-android-arm64-universal | strings | grep -E '^(5062aee5|cb3d4ee9)$' | head -n 1 || true)"
  fi

  case "$embedded_ref" in
    cb3d4ee9)
      ENGINE_LABEL_VALUE="Stockfish 18"
      ENGINE_REF_VALUE="sf_18 / cb3d4ee9b47d0c5aae855b12379378ea1439675c"
      ;;
    5062aee5)
      ENGINE_LABEL_VALUE="Stockfish dev-20260810-5062aee5"
      ENGINE_REF_VALUE="5062aee519a1ba262d472d8ab139851ced56573e"
      ;;
    *)
      if [[ "$engine_sha" == "$DEV_ENGINE_SHA256" ]]; then
        ENGINE_LABEL_VALUE="Stockfish dev-20260810-5062aee5"
        ENGINE_REF_VALUE="5062aee519a1ba262d472d8ab139851ced56573e"
      else
        ENGINE_LABEL_VALUE="Stockfish (verified bundle)"
        ENGINE_REF_VALUE="engine SHA-256 ${engine_sha}"
      fi
      ;;
  esac
}

ensure_stockfish_artifact() {
  local artifact="$ROOT/stockfish-android-arm64-universal.zip"
  local cache_dir="${XDG_CACHE_HOME:-$HOME/.cache}/cerelytic-chess"
  local tarball="$cache_dir/$STOCKFISH_TAR"
  local unpack normalized engine license
  if [[ -f "$artifact" ]] \
    && unzip -Z1 "$artifact" | grep -Fx stockfish-android-arm64-universal >/dev/null \
    && unzip -Z1 "$artifact" | grep -Fx Copying.txt >/dev/null; then
    detect_engine_metadata "$artifact"
    return
  fi

  printf '\n==> Fetching verified Stockfish 18 ARM64 engine\n'
  mkdir -p "$cache_dir"
  if [[ ! -f "$tarball" ]] || ! echo "$STOCKFISH_SHA256  $tarball" | sha256sum -c - >/dev/null 2>&1; then
    rm -f "$tarball"
    curl -fL --retry 3 --retry-delay 2 "$STOCKFISH_URL" -o "$tarball"
  fi
  echo "$STOCKFISH_SHA256  $tarball" | sha256sum -c -

  unpack="$(mktemp -d)"
  normalized="$(mktemp -d)"
  tar -xf "$tarball" -C "$unpack"
  engine="$(find "$unpack" -type f -name stockfish-android-armv8 -print -quit)"
  license="$(find "$unpack" -type f \( -name Copying.txt -o -name COPYING \) -print -quit)"
  [[ -n "$engine" && -n "$license" ]] || { echo "Official Stockfish archive layout was not recognized." >&2; exit 3; }
  cp "$engine" "$normalized/stockfish-android-arm64-universal"
  cp "$license" "$normalized/Copying.txt"
  chmod 755 "$normalized/stockfish-android-arm64-universal"
  (cd "$normalized" && zip -q "$artifact" stockfish-android-arm64-universal Copying.txt)
  rm -rf "$unpack" "$normalized"
  ENGINE_LABEL_VALUE="Stockfish 18"
  ENGINE_REF_VALUE="sf_18 / cb3d4ee9b47d0c5aae855b12379378ea1439675c"
}

run_tests() {
  local t="${TMPDIR:-/tmp}/cerelytic-v3-tests"
  mkdir -p "$t"
  printf '\n==> Running native, persistence, engine, packaging, and renderer tests\n'
  clang -std=c11 -Wall -Wextra -Werror \
    native/tests/core_test.c native/core.c -o "$t/core-test"
  "$t/core-test"

  clang -std=c11 -Wall -Wextra -Werror \
    native/tests/state_test.c native/state.c -o "$t/state-test"
  "$t/state-test"

  clang -std=c11 -Wall -Wextra -Werror -pthread \
    native/tests/engine_parser_test.c native/engine.c native/core.c -o "$t/engine-test"
  "$t/engine-test"

  clang -std=c11 -Wall -Wextra -Werror \
    native/tests/app_model_test.c native/app_model.c -o "$t/model-test"
  "$t/model-test"

  python3 native/tests/test_patch_manifest.py
  python3 native/tests/test_patch_resources.py
  native/tests/test_stockfish_artifact.sh
  native/tests/render_preview_smoke.sh "$t/previews"
  bash -n native/build_native.sh native/build_apk.sh scripts/setup-fedora.sh scripts/push-github.sh scripts/verify-apk.sh native/tests/test_stockfish_artifact.sh native/tests/test_stockfish_integration.sh
  python3 -m py_compile native/patch_manifest.py native/patch_resources.py
}

cd "$ROOT"
chmod +x native/build_native.sh native/build_apk.sh native/tests/render_preview_smoke.sh native/tests/test_stockfish_artifact.sh native/tests/test_stockfish_integration.sh scripts/verify-apk.sh
ensure_stockfish_artifact
run_tests

printf '\n==> Building Chess v3 APK\n'
mkdir -p dist "$HOME/.local/share/cerelytic-chess"
OUTPUT="$ROOT/dist/Chess-v3-offline-arm64.apk"
DEFAULT_KEYSTORE="$HOME/.local/share/cerelytic-chess/release.keystore"
BUNDLED_KEYSTORE="$ROOT/signing/cerelytic-v3-release.keystore"
KEYSTORE_PATH="${KEYSTORE:-$DEFAULT_KEYSTORE}"
STORE_PASSWORD="${STOREPASS:-cerelytic-local-release}"
KEY_PASSWORD="${KEYPASS:-$STORE_PASSWORD}"
KEY_ALIAS_VALUE="${KEY_ALIAS:-cerelyticlocal}"

# The Fedora bundle carries the compatibility key used for the downloadable v3
# APK. The public source archive deliberately does not. Install it into the
# user's private app-specific directory only when no explicit KEYSTORE was set.
if [[ -z "${KEYSTORE:-}" && -f "$BUNDLED_KEYSTORE" ]]; then
  mkdir -p "$(dirname "$DEFAULT_KEYSTORE")"
  if [[ ! -f "$DEFAULT_KEYSTORE" ]]; then
    install -m 600 "$BUNDLED_KEYSTORE" "$DEFAULT_KEYSTORE"
  elif ! keytool -list -keystore "$DEFAULT_KEYSTORE" -storepass "$STORE_PASSWORD" -alias "$KEY_ALIAS_VALUE" >/dev/null 2>&1; then
    backup="$DEFAULT_KEYSTORE.pre-v3.$(date +%Y%m%d%H%M%S)"
    cp -p "$DEFAULT_KEYSTORE" "$backup"
    install -m 600 "$BUNDLED_KEYSTORE" "$DEFAULT_KEYSTORE"
    echo "Existing app-specific key was incompatible and was backed up to: $backup" >&2
  fi
  KEYSTORE_PATH="$DEFAULT_KEYSTORE"
fi

build_env=(
  STOCKFISH_ARTIFACT="$ROOT/stockfish-android-arm64-universal.zip"
  APK_OUT="$OUTPUT"
  KEYSTORE="$KEYSTORE_PATH"
  STOREPASS="$STORE_PASSWORD"
  KEYPASS="$KEY_PASSWORD"
  KEY_ALIAS="$KEY_ALIAS_VALUE"
  ENGINE_LABEL="$ENGINE_LABEL_VALUE"
  ENGINE_REF="$ENGINE_REF_VALUE"
)
[[ -n "$APKSIGNER_BIN" ]] && build_env+=(APKSIGNER="$APKSIGNER_BIN")
env "${build_env[@]}" ./native/build_apk.sh

if [[ -n "$APKSIGNER_BIN" ]]; then
  "$APKSIGNER_BIN" verify --verbose --print-certs "$OUTPUT"
else
  jarsigner -verify "$OUTPUT"
fi
python3 - "$OUTPUT" <<'PY'
import sys, zipfile
with zipfile.ZipFile(sys.argv[1]) as z:
    assert z.testzip() is None
    names = set(z.namelist())
    required = {
        "AndroidManifest.xml", "resources.arsc",
        "lib/arm64-v8a/libsf_chess.so", "lib/arm64-v8a/libstockfish.so",
        "assets/NOTICE.txt", "assets/BUILD-METADATA.txt",
    }
    assert required <= names, required - names
print("APK archive verification: PASS")
PY
sha256sum "$OUTPUT" | tee "$OUTPUT.sha256"

if (( INSTALL_APK )); then
  adb_args=()
  [[ -n "$DEVICE_SERIAL" ]] && adb_args=(-s "$DEVICE_SERIAL")
  adb "${adb_args[@]}" start-server
  if ! adb "${adb_args[@]}" get-state 2>/dev/null | grep -qx device; then
    echo "No authorized Android device found. Enable USB debugging, reconnect, and rerun with --install." >&2
    exit 4
  fi
  if ! adb "${adb_args[@]}" install -r "$OUTPUT"; then
    cat >&2 <<'MSG'
Android rejected the update, usually because the installed APK was signed by a
different local key. Uninstall com.cerelytic.knight and rerun with --install.
Uninstalling deletes the old app's local profiles and game history.
MSG
    exit 5
  fi
fi

printf '\nBuilt: %s\n' "$OUTPUT"
printf 'Signing key retained at: %s\n' "$KEYSTORE_PATH"
printf 'Keep that key private; losing it prevents in-place Android updates. Humanity invented this ceremony for good reasons, mostly.\n'
