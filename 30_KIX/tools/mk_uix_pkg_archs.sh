#!/bin/sh
#
# mk_uix_pkg_archs.sh — build the userspace store image for every arch,
# from a clean output directory.
#
#   mk_uix_pkg_archs.sh              # all four: arm64 arm32 riscv64 x86_64
#   mk_uix_pkg_archs.sh arm64        # just one
#   mk_uix_pkg_archs.sh --no-clean arm64
#
# Per arch:
#   1. rm -rf 30_KIX/build/<arch>/pkg        (clean, so a stale .upkg or a
#                                             stale store.img cannot be
#                                             mistaken for a fresh one)
#   2. mk_uix_pkg_all.sh <staging> <out> <arch>
#   3. verify store.img is non-empty and report its size
#
# WHY THE CLEAN MATTERS HERE: the failure that started this was a 0-byte
# uix_pkg.o, produced because a failed packer left no store.img and
# objcopy happily converted nothing.  A stale store.img is worse than a
# missing one — it links and boots with the WRONG packages.  Deleting the
# directory first is what makes the reported size mean something.

set -e

HERE=$(dirname "$0")
ROOT=$(cd "$HERE/../.." && pwd)
MFDIR="$ROOT/30_KIX"

CLEAN=yes
if [ "$1" = "--no-clean" ]; then
    CLEAN=no
    shift
fi

if [ $# -gt 0 ]; then
    ARCHS="$*"
else
    ARCHS="arm64 arm32 riscv64 x86_64"
fi

for arch in $ARCHS; do
    STAGE="$ROOT/50_UIX/21_apps/build/$arch/image/21_apps_"
    OUT="$MFDIR/build/$arch/pkg"

    printf '\n== %s\n' "$arch"

    if [ ! -d "$STAGE" ]; then
        echo "   staging missing: $STAGE" >&2
        echo "   run ./rebuild_uix.sh $arch first" >&2
        exit 1
    fi

    APPS=$(find "$STAGE" -type f -name 'app_*' | wc -l | tr -d ' ')
    if [ "$APPS" -eq 0 ]; then
        echo "   no app_* in $STAGE" >&2
        exit 1
    fi

    if [ "$CLEAN" = yes ]; then
        rm -rf "$OUT"
        echo "   clean  $OUT"
    fi
    mkdir -p "$OUT"

    sh "$HERE/mk_uix_pkg_all.sh" "$STAGE" "$OUT" "$arch"

    [ -s "$OUT/store.img" ] || {
        echo "   store.img EMPTY — packer produced nothing" >&2
        exit 1
    }

    SIZE=$(wc -c < "$OUT/store.img" | tr -d ' ')
    UPKGS=$(ls "$OUT"/*.upkg 2>/dev/null | wc -l | tr -d ' ')

    printf '   store.img  %s bytes  (%s packages)\n' "$SIZE" "$UPKGS"
done

printf '\ndone.\n'
