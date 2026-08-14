#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD="$ROOT/build"
STUBS="$BUILD/stubs"
CC="${CLANG:-clang}"
TARGET="--target=aarch64-linux-android29"
COMMON=("$TARGET" -ffreestanding -fPIC -fvisibility=hidden -fno-stack-protector -ffunction-sections -fdata-sections -O2 -Wall -Wextra -Werror -I"$ROOT")
mkdir -p "$BUILD" "$STUBS"
cat > "$STUBS/android_stub.c" <<'STUB'
#define S(x) __attribute__((visibility("default"))) void x(void) {}
S(ANativeActivity_setWindowFlags)
S(ANativeActivity_setWindowFormat)
S(ANativeWindow_getWidth)
S(ANativeWindow_getHeight)
S(ANativeWindow_setBuffersGeometry)
S(ANativeWindow_lock)
S(ANativeWindow_unlockAndPost)
S(ALooper_forThread)
S(ALooper_prepare)
S(ALooper_addFd)
S(ALooper_removeFd)
S(AInputQueue_attachLooper)
S(AInputQueue_detachLooper)
S(AInputQueue_getEvent)
S(AInputQueue_preDispatchEvent)
S(AInputQueue_finishEvent)
S(AInputEvent_getType)
S(AMotionEvent_getAction)
S(AMotionEvent_getX)
S(AMotionEvent_getY)
STUB
cat > "$STUBS/libc_stub.c" <<'STUB'
#define S(x) __attribute__((visibility("default"))) void x(void) {}
S(open) S(read) S(write) S(close) S(pipe) S(fork) S(dup2) S(execv) S(_exit)
S(waitpid) S(kill) S(malloc) S(free) S(memset) S(memcpy) S(memmove) S(strlen)
S(strcmp) S(strncmp) S(strstr) S(strchr) S(poll) S(unlink) S(rename)
S(clock_gettime) S(nanosleep) S(pthread_create) S(pthread_detach) S(pthread_join)
STUB
"$CC" "$TARGET" -fno-builtin -Wno-incompatible-library-redeclaration -Wno-invalid-noreturn -nostdlib -fuse-ld=lld -shared -Wl,-soname,libandroid.so -Wl,--build-id=none "$STUBS/android_stub.c" -o "$STUBS/libandroid.so"
"$CC" "$TARGET" -fno-builtin -Wno-incompatible-library-redeclaration -Wno-invalid-noreturn -nostdlib -fuse-ld=lld -shared -Wl,-soname,libc.so -Wl,--build-id=none "$STUBS/libc_stub.c" -o "$STUBS/libc.so"
for src in core state engine ui activity; do
  "$CC" "${COMMON[@]}" -DSF_FREESTANDING=1 -c "$ROOT/$src.c" -o "$BUILD/$src.o"
done
"$CC" "$TARGET" -nostdlib -fuse-ld=lld -shared \
  "$BUILD/activity.o" "$BUILD/core.o" "$BUILD/state.o" "$BUILD/engine.o" "$BUILD/ui.o" \
  -L"$STUBS" -Wl,--no-as-needed -landroid -lc \
  -Wl,-soname,libsf_chess.so -Wl,-z,now -Wl,--gc-sections \
  -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384 -Wl,--build-id=none \
  -o "$BUILD/libsf_chess.so"
file "$BUILD/libsf_chess.so"
