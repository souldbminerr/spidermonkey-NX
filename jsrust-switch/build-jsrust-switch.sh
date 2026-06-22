#!/bin/bash
export RUSTUP_HOME=/root/.rustup CARGO_HOME=/root/.cargo PATH=/root/.cargo/bin:$PATH
MOZ=/root/sm/mozjs-128.13.0
SPK=/mnt/c/Users/sould/0ad-switch/0ad-src/libraries/source/spidermonkey
SHIMS="$SPK/switch-shims"
BIN=/opt/devkitpro/devkitA64/bin; LIBNX=/opt/devkitpro/libnx; PORTLIBS=/opt/devkitpro/portlibs/switch
NIGHT=/root/.rustup/toolchains/nightly-x86_64-unknown-linux-gnu/bin
TGT=aarch64-nintendo-switch-freestanding; BUILD=/root/jsrust-build

echo "=== jsrust_shared/lib.rs head (uses Vec/Box?) ==="
grep -cE "Vec|Box|String" "$MOZ/js/src/rust/shared/lib.rs"
echo "=== patch jsrust_shared no_std ==="
python3 /mnt/c/tmp/patch_nostd.py "$MOZ/js/src/rust/shared/lib.rs" 1
head -8 "$MOZ/js/src/rust/shared/lib.rs"

export BUILDCONFIG_RS="$MOZ/obj-switch/build/rust/mozbuild/buildconfig.rs"
export RUSTC="$NIGHT/rustc" RUSTFLAGS="-Cpanic=abort"
CXXENV="-D__linux__ -D__gnu_linux__ -D_GNU_SOURCE -DMOZ_SWITCH_NO_SIGNAL -I$SHIMS -I$LIBNX/include -I$PORTLIBS/include"
export CC_aarch64_nintendo_switch_freestanding="$BIN/aarch64-none-elf-gcc"
export CXX_aarch64_nintendo_switch_freestanding="$BIN/aarch64-none-elf-g++"
export CFLAGS_aarch64_nintendo_switch_freestanding="$CXXENV"
export CXXFLAGS_aarch64_nintendo_switch_freestanding="$CXXENV"
export CARGO_TARGET_AARCH64_NINTENDO_SWITCH_FREESTANDING_LINKER="$BIN/aarch64-none-elf-gcc"

cd "$BUILD"
echo "=== rebuild ==="
"$NIGHT/cargo" build --release -Zbuild-std=core,alloc --target "$TGT" > /tmp/m3f14.log 2>&1
echo "cargo exit: $?"
echo "=== errors ==="
grep -nE "error\[|error:|can't find crate|cannot find|could not|panic_handler|no global|cannot be shared|undefined" /tmp/m3f14.log | head -18
echo "=== which crate ==="
grep -m5 -oE "/root/(jsrust-patches|sm)/[^ ]+\.rs" /tmp/m3f14.log | sort -u | head
echo "=== artifact ==="
ls -la "$BUILD/target/$TGT/release/libjsrust.a" 2>/dev/null && echo "*** FREESTANDING libjsrust.a BUILT (M3f) ***"
