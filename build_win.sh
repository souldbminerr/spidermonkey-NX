# WSL to compile spidermonkey, MSYS2 for NRO
set -euo pipefail

JOBS="${JOBS:-4}"
WSL_DISTRO="${WSL_DISTRO:-archlinux}"

# Repo root as msys2 (/c/..), Windows (C:/..) and WSL (/mnt/c/..) paths.
SPK_MSYS="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SPK_WIN="$(cygpath -m "$SPK_MSYS")"
SPK_WSL="$(printf '%s\n' "$SPK_MSYS" | sed -E 's#^/([a-zA-Z])/#/mnt/\L\1/#')"

echo "==> repo msys2=$SPK_MSYS  win=$SPK_WIN  wsl=$SPK_WSL  jobs=$JOBS"

echo "##################################################################"
echo "# Phase 1 (WSL:$WSL_DISTRO) - mozjs + jsrust + assemble real libs #"
echo "##################################################################"
wsl.exe -d "$WSL_DISTRO" -e bash -c "
  set -e
  cd '$SPK_WSL'
  export JOBS='$JOBS'
  bash switch-build/build-mozjs.sh
  bash switch-build/build-jsrust.sh
  ASSEMBLE_ONLY=1 bash switch-build/make-nro.sh
"

echo "##################################################################"
echo "# Phase 2 (msys2) - link NRO with devkitA64                       #"
echo "##################################################################"
export DEVKITPRO=/opt/devkitpro
export PATH="/opt/devkitpro/devkitA64/bin:/opt/devkitpro/tools/bin:$PATH"
cd "$SPK_MSYS/switch-test"
make clean >/dev/null 2>&1 || true
make -j"$JOBS" \
  MOZ_DIST="$SPK_WIN/switch-build/dist" \
  JSLIBS="$SPK_WIN/switch-build/libs"

NRO="$SPK_MSYS/switch-test/SpiderMonkey-NX.nro"
if [ ! -f "$NRO" ]; then
  echo "==> ERROR: NRO not produced" >&2
  exit 1
fi
cp -f "$NRO" "$SPK_MSYS/SpiderMonkey-NX.nro"
echo "==> built $SPK_MSYS/SpiderMonkey-NX.nro"
if [ -n "${NRO_OUT:-}" ]; then
  mkdir -p "$NRO_OUT" && cp -f "$NRO" "$NRO_OUT/" && echo "==> copied to $NRO_OUT"
fi
