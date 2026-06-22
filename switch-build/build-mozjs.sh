#!/bin/bash
set -e
source "$(dirname "$0")/env.sh"
TAR="$MOZ.tar.xz"

echo "=== [mozjs] restore in-tree sources to pristine (clean, reproducible build) ==="
TMP=/tmp/sm-pristine; rm -rf "$TMP"; mkdir -p "$TMP"; cd "$TMP"
for rel in mozglue/static/rust/lib.rs mozglue/static/rust/Cargo.toml \
           js/src/rust/shared/lib.rs js/src/rust/shared/Cargo.toml \
           intl/bidi/rust/unicode-bidi-ffi/src/lib.rs intl/bidi/rust/unicode-bidi-ffi/Cargo.toml \
           js/src/vm/SelfHosting.cpp js/src/vm/Compression.cpp \
           js/src/util/NativeStack.cpp mozglue/misc/StackWalk.cpp; do
  tar xJf "$TAR" "mozjs-128.13.0/$rel" 2>/dev/null && cp "mozjs-128.13.0/$rel" "$MOZ/$rel"
done
sed -i '/# SWITCH_BUILD_STD/d; /cargo_build_flags += -Zbuild-std=core,alloc/d' "$MOZ/config/makefiles/rust.mk"

for f in js/src/util/NativeStack.cpp mozglue/misc/StackWalk.cpp; do
  grep -q SWITCH_PTHREAD_GETATTR "$MOZ/$f" || \
    sed -i '1i /*SWITCH_PTHREAD_GETATTR*/\n#include <pthread.h>\nextern "C" int pthread_getattr_np(pthread_t, pthread_attr_t *);' "$MOZ/$f"
done

if [ "${SWITCH_DEBUG_SELFHOST:-0}" = "1" ]; then
  echo "  [debug] applying self-hosting/zlib instrumentation"
  python3 "$(dirname "$0")/debug/patch_selfhost.py"
  python3 "$(dirname "$0")/debug/patch_compress.py"
fi

echo "=== [mozjs] toolchain: devkitA64 gcc + -fPIC + switch shims ==="
SWITCH_ARCH="-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft"
export CC="$BIN/aarch64-none-elf-gcc $SWITCH_ARCH $SWITCH_DEFS -fPIC -fno-stack-protector $SWITCH_INCS"
export CXX="$BIN/aarch64-none-elf-g++ $SWITCH_ARCH $SWITCH_DEFS -fPIC -fno-stack-protector $SWITCH_INCS"
export AR="$BIN/aarch64-none-elf-ar"
export HOST_CC=gcc HOST_CXX=g++

echo "=== [mozjs] clobber + build (-fPIC) ==="
cd "$MOZ"
rm -rf obj-switch
./mach build -j"${JOBS:-8}"

LIB="$MOZ/obj-switch/js/src/build/libjs_static.a"
ls -lh "$LIB" && echo "[mozjs] OK -> $LIB (-fPIC)"
