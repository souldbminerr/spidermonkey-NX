#!/bin/bash
set -e
source "$(dirname "$0")/env.sh"
JSRUST="$JSRUST_BUILD/target/$RUST_TARGET_SWITCH/release/libjsrust.a"
JSLIBS="$SPK/switch-build/libs"
export PATH="$BIN:$DKP/tools/bin:$PATH"

echo "=== build libswitchextra.a ==="
mkdir -p "$JSLIBS/lib"
SW_ARCH="-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE -fno-stack-protector"
$BIN/aarch64-none-elf-gcc $SW_ARCH -D__linux__ -D_GNU_SOURCE -I"$SHIMS" \
    -c "$SHIMS/switch_libc_compat.c" -o /tmp/libc_compat.o
$BIN/aarch64-none-elf-gcc $SW_ARCH -D__SWITCH__ -I"$LIBNX/include" -I"$PORTLIBS/include" \
    -c "$SHIMS/switch_virtmem.c" -o /tmp/switch_virtmem.o
OBJS=$(find "$MOZ"/obj-switch/mfbt "$MOZ"/obj-switch/mozglue "$MOZ"/obj-switch/memory -name '*.o' \
       -not -name 'Unified_cpp_mozglue_interposers0.o')
rm -f "$JSLIBS/lib/libswitchextra.a"
$BIN/aarch64-none-elf-ar rcs "$JSLIBS/lib/libswitchextra.a" $OBJS /tmp/switch_virtmem.o /tmp/libc_compat.o
echo "  libswitchextra.a: $(ls -lh "$JSLIBS/lib/libswitchextra.a" | awk '{print $5}')"

echo "=== copy prebuilt libs (real files so msys2/devkitA64 can read them) ==="
cp -f "$MOZ/obj-switch/js/src/build/libjs_static.a" "$JSLIBS/lib/libjs_static.a"
cp -f "$JSRUST" "$JSLIBS/lib/libjsrust.a"

# ASSEMBLE_ONLY=1: stage artifacts for an out-of-WSL (msys2) NRO link and stop.
# mach populates dist/include with symlinks into the source tree; WSL-style
# symlinks are opaque to msys2's gcc, so materialize them as REAL files with
# cp -rL. Regenerated build artifact (gitignored), not committed.
if [ "${ASSEMBLE_ONLY:-0}" = "1" ]; then
  DIST="$SPK/switch-build/dist"
  echo "=== materialize dist/include -> $DIST/include (real files for msys2) ==="
  rm -rf "$DIST/include"
  mkdir -p "$DIST"
  cp -rL "$MOZ/obj-switch/dist/include" "$DIST/include"
  echo "=== libs assembled in $JSLIBS/lib (ASSEMBLE_ONLY) ==="
  ls -lh "$JSLIBS/lib"
  exit 0
fi

echo "=== build NRO (switch-test) ==="
cd "$SPK/switch-test"
make clean >/dev/null 2>&1 || true
make MOZ="$MOZ" JSLIBS="$JSLIBS" DEBUG_SELFHOST="${SWITCH_DEBUG_SELFHOST:-0}" -j"${JOBS:-8}"
NRO=$(ls *.nro 2>/dev/null | head -1)
if [ -z "$NRO" ]; then echo "  build FAILED"; exit 1; fi
cp "$NRO" "$SPK/"
[ -n "$NRO_OUT" ] && cp "$NRO" "$NRO_OUT/" 2>/dev/null || true
echo "  built $NRO -> $SPK/$NRO"

JELF="$SPK/switch-test/SpiderMonkey-NX.elf"
if [ -f "$JELF" ]; then
  RAW=$("$BIN/aarch64-none-elf-objdump" -d "$JELF" 2>/dev/null | grep -c 'mrs.*tpidr_el0' || true)
  SOFT=$("$BIN/aarch64-none-elf-objdump" -d "$JELF" 2>/dev/null | grep -c '<__aarch64_read_tp>' || true)
  echo "=== TLS ABI check: raw 'mrs tpidr_el0'=$RAW (want 0), '__aarch64_read_tp' calls=$SOFT (want many) ==="
fi
exit 0
