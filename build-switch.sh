#!/bin/bash
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
source "$HERE/switch-build/env.sh"

apply_0ad=0
for arg in "$@"; do
  [ "$arg" = "-0ad" ] && apply_0ad=1
done

if [ ! -d "$MOZ" ]; then
  echo "=== extracting mozjs to $MOZ ==="
  mkdir -p "$(dirname "$MOZ")"
  tar xJf "$MOZ.tar.xz" -C "$(dirname "$MOZ")"
fi

if [ "$apply_0ad" = "1" ]; then
  echo "=== applying 0 A.D. mozjs patches ==="
  for d in FixRustLinkage FixLibNames FixPkgConfigDebug FixExtraGCTracing \
           SuppressDanglingPointerWarning FixPython3_14; do
    f="$HERE/patches/$d.diff"; [ -e "$f" ] || f="$HERE/patches/$d.patch"
    [ -e "$f" ] || continue
    if patch -p1 -d "$MOZ" --dry-run -R -f < "$f" >/dev/null 2>&1; then
      echo "  already applied: $d"
    else
      echo "  applying: $d"; patch -p1 -d "$MOZ" < "$f"
    fi
  done
fi

bash "$HERE/switch-build/build-all.sh"
echo "=== done ==="
