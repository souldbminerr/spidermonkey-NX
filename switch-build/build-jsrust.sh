#!/bin/bash
set -e
source "$(dirname "$0")/env.sh"

echo "=== [jsrust] apply no_std patches to in-tree mozjs rust crates ==="
P="$SPK/patches/switch/mozglue_static_rust_lib.rs.patch"
if patch -p1 -d "$MOZ" --dry-run -R -f < "$P" >/dev/null 2>&1; then echo "  mozglue: already no_std"
else patch -p1 -d "$MOZ" < "$P"; echo "  mozglue: patched no_std"; fi

sed -i 's/^arrayvec = "0.7"/arrayvec = { version = "0.7", default-features = false }/' \
    "$MOZ/mozglue/static/rust/Cargo.toml"
UF="$MOZ/intl/bidi/rust/unicode-bidi-ffi/Cargo.toml"
sed -i 's#icu_properties = { version = "1.4.0", features = \["bidi"\] }#icu_properties = { version = "1.4.0", default-features = false, features = ["bidi", "compiled_data"] }#' "$UF"
sed -i 's#unicode-bidi = { version = "0.3.15", features = \["smallvec"\] }#unicode-bidi = { version = "0.3.15", default-features = false, features = ["smallvec", "hardcoded-data"] }#' "$UF"
python3 "$(dirname "$0")/patch_nostd.py" "$MOZ/intl/bidi/rust/unicode-bidi-ffi/src/lib.rs" 1
python3 "$(dirname "$0")/patch_nostd.py" "$MOZ/js/src/rust/shared/lib.rs" 1

echo "=== [jsrust] local no_std copies of encoding_c / encoding_c_mem ==="
REG="$(ls -d /root/.cargo/registry/src/*/ | head -1)"
rm -rf "$JSRUST_PATCHES"; mkdir -p "$JSRUST_PATCHES"
cp -r "$REG"/encoding_c-0.9.* "$JSRUST_PATCHES/encoding_c"
cp -r "$REG"/encoding_c_mem-0.2.* "$JSRUST_PATCHES/encoding_c_mem"
grep -q '^edition' "$JSRUST_PATCHES/encoding_c/Cargo.toml" || \
    sed -i '/^\[package\]/a edition = "2021"' "$JSRUST_PATCHES/encoding_c/Cargo.toml"
python3 "$(dirname "$0")/patch_nostd.py" "$JSRUST_PATCHES/encoding_c/src/lib.rs" 1
python3 "$(dirname "$0")/patch_nostd.py" "$JSRUST_PATCHES/encoding_c_mem/src/lib.rs" 0

echo "=== [jsrust] set up standalone workspace ==="
rm -rf "$JSRUST_BUILD"; mkdir -p "$JSRUST_BUILD/src"
cat > "$JSRUST_BUILD/Cargo.toml" <<EOF
[package]
name = "jsrust-switch"
version = "0.0.0"
edition = "2021"
[lib]
name = "jsrust"
crate-type = ["staticlib"]
path = "src/lib.rs"
[dependencies]
jsrust_shared = { path = "$MOZ/js/src/rust/shared" }
mozglue-static = { path = "$MOZ/mozglue/static/rust" }
[patch.crates-io]
mozbuild = { path = "$MOZ/build/rust/mozbuild" }
mozilla-central-workspace-hack = { path = "$MOZ/build/workspace-hack" }
encoding_c = { path = "$JSRUST_PATCHES/encoding_c" }
encoding_c_mem = { path = "$JSRUST_PATCHES/encoding_c_mem" }
[profile.release]
panic = "abort"
EOF
printf '#![no_std]\nextern crate jsrust_shared;\nextern crate mozglue_static;\n' > "$JSRUST_BUILD/src/lib.rs"

echo "=== [jsrust] build (nightly, -Zbuild-std=core,alloc) ==="
export BUILDCONFIG_RS="$MOZ/obj-switch/build/rust/mozbuild/buildconfig.rs"
export RUSTC="$NIGHTLY/rustc" RUSTFLAGS="-Cpanic=abort"
CXXENV="$SWITCH_DEFS $SWITCH_INCS"
export CC_aarch64_nintendo_switch_freestanding="$BIN/aarch64-none-elf-gcc"
export CXX_aarch64_nintendo_switch_freestanding="$BIN/aarch64-none-elf-g++"
export CFLAGS_aarch64_nintendo_switch_freestanding="$CXXENV"
export CXXFLAGS_aarch64_nintendo_switch_freestanding="$CXXENV"
export CARGO_TARGET_AARCH64_NINTENDO_SWITCH_FREESTANDING_LINKER="$BIN/aarch64-none-elf-gcc"
cd "$JSRUST_BUILD"
"$NIGHTLY/cargo" build --release -Zbuild-std=core,alloc --target "$RUST_TARGET_SWITCH"

LIB="$JSRUST_BUILD/target/$RUST_TARGET_SWITCH/release/libjsrust.a"
ls -lh "$LIB" && echo "[jsrust] OK -> $LIB"
