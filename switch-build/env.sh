#!/bin/bash

set -e

: "${MOZ:=/root/sm/mozjs-128.13.0}"
: "${JSRUST_BUILD:=/root/jsrust-build}"
: "${JSRUST_PATCHES:=/root/jsrust-patches}"# local no_std-patched crates.io crates
DKP=/opt/devkitpro
BIN="$DKP/devkitA64/bin"
LIBNX="$DKP/libnx"
PORTLIBS="$DKP/portlibs/switch"
NIGHTLY="$(echo /root/.rustup/toolchains/nightly-*/bin | awk '{print $1}')"
RUST_TARGET_SWITCH=aarch64-nintendo-switch-freestanding

SWB="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SPK="$(cd "$SWB/.." && pwd)"
SHIMS="$SPK/switch-shims"

SWITCH_DEFS="-D__linux__ -D__gnu_linux__ -D_GNU_SOURCE -DMOZ_SWITCH_NO_SIGNAL"
SWITCH_INCS="-I$SHIMS -I$LIBNX/include -I$PORTLIBS/include"
SWITCH_FORCE="-include $SHIMS/switch_pthread_compat.h"

export RUSTUP_HOME=/root/.rustup CARGO_HOME=/root/.cargo
export PATH="/root/.cargo/bin:$PATH"
export DEVKITPRO="$DKP" DEVKITA64="$DKP/devkitA64"
export MOZ_NOSPAM=1 MACH_BUILD_PYTHON_NATIVE_PACKAGE_SOURCE=none
export MOZBUILD_STATE_PATH="$MOZ/mozbuild-state"
export MOZCONFIG="$SPK/mozconfig.switch"

echo "[env] MOZ=$MOZ  SPK=$SPK  NIGHTLY=$NIGHTLY"
