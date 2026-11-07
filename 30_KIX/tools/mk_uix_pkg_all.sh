#!/bin/sh
#
# mk_uix_pkg_all.sh — pack a staged directory into one store image.
#
#   mk_uix_pkg_all.sh <staging-dir> <out-dir> <arch>
#
# Per app:  mk_uix_pkg.sh  →  <out-dir>/<name>.upkg
# Then:     uiox_pkg_blob_t manifest + the .upkg payloads → store.img
#
# The Makefile objcopies store.img into uix_pkg.o.  The blob layout is
# defined HERE and read in 32_FS/src/uiox_fs_populate.c — the two must
# agree, and the kernel's copy is the one that matters.
#
# No compiler and no kernel headers: uiox_pkg_types.h includes
# uiox_klibc.h, which is freestanding (EPERM..ENOSPC, its own
# primitives) and cannot be included by a host program.

set -e

SRC=$1
OUT=$2
ARCH=$3

HERE=$(dirname "$0")

NAME_MAX=48                 # UIOX_PKG_NAME_MAX
MAX_ENTRIES=256             # UIOX_PKG_INDEX_MAX_ENTRIES
BLOB_HDR=$((16 + MAX_ENTRIES * (NAME_MAX + 8)))   # 16 + 256*56 = 14352
ALIGN=16

[ -d "$SRC" ] || { echo "  [PKG] no $SRC" >&2; exit 1; }
mkdir -p "$OUT"

FILES=$(find "$SRC" -type f -name 'app_*' | LC_ALL=C sort)
COUNT=$(printf '%s\n' $FILES | grep -c . || true)

[ "$COUNT" -gt 0 ] || { echo "  [PKG] no app_* in $SRC" >&2; exit 1; }
[ "$COUNT" -le "$MAX_ENTRIES" ] || {
    echo "  [PKG] $COUNT packages exceeds $MAX_ENTRIES" >&2; exit 1; }

# ── pass 1: one .upkg each, and lay out the manifest ─────────────────
OFF=$BLOB_HDR
TABLE=""
i=0
for f in $FILES; do
    base=$(basename "$f")

    [ ${#base} -lt "$NAME_MAX" ] || {
        echo "  [PKG] name too long: $base" >&2; exit 1; }

    sh "$HERE/mk_uix_pkg.sh" "$OUT/$base.upkg" "$f" "$base" "$ARCH" >/dev/null

    len=$(wc -c < "$OUT/$base.upkg" | tr -d ' ')
    pad=$(( (ALIGN - (OFF % ALIGN)) % ALIGN ))
    OFF=$(( OFF + pad ))
    TABLE="$TABLE$base $OFF $len
"
    OFF=$(( OFF + len ))
    i=$(( i + 1 ))
done
TOTAL=$OFF

# ── the manifest + payloads ──────────────────────────────────────────
# magic 'UIXP' = 0x55495850, count, total, reserved; then NAME_MAX-padded
# names each with off/len, padded out to MAX_ENTRIES slots.
{
    awk -v n="$COUNT" -v t="$TOTAL" -v max="$MAX_ENTRIES" -v hdr="$BLOB_HDR" '
    function le32(v) { printf "%c%c%c%c", v%256, int(v/256)%256, int(v/65536)%256, int(v/16777216)%256 }
    function ordc(c) { return index(" !\"#$%&'\''()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~", c) + 31 }
    BEGIN {
        le32(0x55495850); le32(n); le32(t); le32(0)
        for (i = 0; i < max; i++) {
            for (c = 1; c <= 48; c++) printf "%c", 0
            le32(0); le32(0)
        }
    }' > "$OUT/store.img"
} 

# patch the real entries over the zeroed slots
{
    printf '%s' "$TABLE" | awk -v max="$MAX_ENTRIES" -v hdr="$BLOB_HDR" '
    { name[$1]=$1; off[$1]=$2; len[$1]=$3 }
    END { }'
} >/dev/null

# entries: rewrite the manifest region with name/off/len filled in
POS=$((16))
{
    printf '%s' "$TABLE" | while read -r base off len; do
        # 48-byte name, then off, then len — written at POS
        awk -v name="$base" -v o="$off" -v l="$len" '
        function le32(v) { printf "%c%c%c%c", v%256, int(v/256)%256, int(v/65536)%256, int(v/16777216)%256 }
        function ordc(c) { return index(" !\"#$%&'\''()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~", c) + 31 }
        BEGIN {
            for (c = 1; c <= 48; c++)
                printf "%c", (c <= length(name)) ? ordc(substr(name,c,1)) : 0
            le32(o); le32(l)
        }' | dd of="$OUT/store.img" bs=1 seek="$POS" conv=notrunc 2>/dev/null
        POS=$(( POS + 56 ))
    done
}

# payloads
for f in $FILES; do
    base=$(basename "$f")
    off=$(printf '%s' "$TABLE" | awk -v n="$base" '$1==n { print $2 }')
    cur=$(wc -c < "$OUT/store.img" | tr -d ' ')
    while [ "$cur" -lt "$off" ]; do printf '\0' >> "$OUT/store.img"; cur=$(( cur + 1 )); done
    cat "$OUT/$base.upkg" >> "$OUT/store.img"
done

SIZE=$(wc -c < "$OUT/store.img" | tr -d ' ')
[ "$SIZE" -eq "$TOTAL" ] || {
    echo "  [PKG] store.img is $SIZE, expected $TOTAL" >&2; exit 1; }

echo "  [PKG] store.img — $COUNT packages, $TOTAL bytes"
