#!/bin/bash
set -e
source "$(dirname "$0")/env.sh"
JSRUST="$JSRUST_BUILD/target/$RUST_TARGET_SWITCH/release/libjsrust.a"
JSLIBS=/root/jstest-libs
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

echo "=== symlink prebuilt libs ==="
ln -sf "$MOZ/obj-switch/js/src/build/libjs_static.a" "$JSLIBS/lib/libjs_static.a"
ln -sf "$JSRUST" "$JSLIBS/lib/libjsrust.a"

echo "=== build NRO (switch-test) ==="
cd "$SPK/switch-test"
make clean >/dev/null 2>&1 || true
make MOZ="$MOZ" JSLIBS="$JSLIBS" DEBUG_SELFHOST="${SWITCH_DEBUG_SELFHOST:-0}" -j"${JOBS:-8}"
NRO=$(ls *.nro 2>/dev/null | head -1)
if [ -z "$NRO" ]; then echo "  build FAILED"; exit 1; fi
cp "$NRO" "$SPK/"
[ -n "$NRO_OUT" ] && cp "$NRO" "$NRO_OUT/" 2>/dev/null || true
echo "  built $NRO -> $SPK/$NRO"

JELF="$SPK/switch-test/jstest.elf"
if [ -f "$JELF" ]; then
  RAW=$("$BIN/aarch64-none-elf-objdump" -d "$JELF" 2>/dev/null | grep -c 'mrs.*tpidr_el0' || true)
  SOFT=$("$BIN/aarch64-none-elf-objdump" -d "$JELF" 2>/dev/null | grep -c '<__aarch64_read_tp>' || true)
  echo "=== TLS ABI check: raw 'mrs tpidr_el0'=$RAW (want 0), '__aarch64_read_tp' calls=$SOFT (want many) ==="
fi
exit 0
