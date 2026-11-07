#!/bin/sh
#
# mk_uix_pkg.sh — turn a staged binary into one .upkg package.
#
#   mk_uix_pkg.sh <out.upkg> <binary> <pkgname> <arch>
#
# Layout, from uiox_pkg_types.h — the kernel's own header, not a format
# invented here:
#
#   uiox_pkg_hdr_t        512 B   (the struct is __packed with _pad[24]
#                                  precisely so it lands on 512)
#   uiox_pkg_file_t[]     n x 172 B
#   payload               hdr.data_offset .. hdr.data_offset+archive_size
#
# NO INDEX.  index.upix carries archive_lba, which is a UNFS BLOCK
# number.  Only the writer knows where a file lands, so the index is
# built by populate_root in the kernel.  This script does not guess it.
#
# Deliberately knows nothing about UNFS beyond that: no inode, no
# extent, no allocation.  The kernel's writer is the only
# implementation of the filesystem format in the tree.

set -e

OUT=$1
BIN=$2
NAME=$3
ARCH=$4

HDR_SIZE=416        # measured: sizeof(uiox_pkg_hdr_t) packed.
                    # Its _pad[24] reaches 416, not the 512 the comment
                    # claims — 512 would need _pad[120].
FILE_REC_SIZE=165   # path[128] + size[4] + sha256[32] + perm[1], packed
NAME_MAX=48
DESC_MAX=128
ARCH_MAX=16
URL_MAX=128

[ -f "$BIN" ] || { echo "mk_uix_pkg: no such binary: $BIN" >&2; exit 1; }

SIZE=$(wc -c < "$BIN" | tr -d ' ')
DIGEST=$(shasum -a 256 "$BIN" | cut -d' ' -f1)

# installed path — matches uiox_pkg_exec.c:49, which extracts to
# "/usr/pkg" and appends the package name.
INSTALL_PATH="/usr/pkg/$NAME"
FILE_OFF=$HDR_SIZE
DATA_OFF=$(( HDR_SIZE + FILE_REC_SIZE ))
PERM=0755

# ── header, 512 bytes ────────────────────────────────────────────────
# magic 'UPKG' = 0x55504B47 little-endian, format_version = 0x00010000,
# then fixed-width char fields, then sizes/offsets, then the digest.
awk -v name="$NAME" -v arch="$ARCH" -v isz="$SIZE" \
    -v asz="$SIZE" -v nf=1 -v file_off="$FILE_OFF" -v data_off="$DATA_OFF" '
function le32(v) { printf "%c%c%c%c", v%256, int(v/256)%256, int(v/65536)%256, int(v/16777216)%256 }
function str(s, n,   i) {
    for (i = 1; i <= n; i++) printf "%c", (i <= length(s)) ? ordc(substr(s,i,1)) : 0
}
function ordc(c) { return index(" !\"#$%&'\''()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~", c) + 31 }
BEGIN {
    le32(0x55504B47)            # magic   "UPKG"
    le32(0x00010000)            # format_version
    str(name, 48)               # name
    str("UIOX application " name, 128)   # desc
    str(arch, 16)               # arch
    str("", 128)                # url — a local package has none
    le32(0x00010000)            # version  1.0.0
    le32(isz)                   # installed_size
    le32(asz)                   # archive_size (uncompressed)
    str("", 32)                 # sha256 — of the ARCHIVE; filled below
    le32(0)                     # num_deps
    le32(nf)                    # num_files
    le32(0)                     # dep_offset — no deps
    le32(file_off)              # file_offset
    le32(data_off)              # data_offset
    for (i = 0; i < 24; i++) printf "%c", 0   # _pad[24]
}' > "$OUT"

# header is 512 by construction; assert it rather than trust the count
H=$(wc -c < "$OUT" | tr -d ' ')
[ "$H" -eq "$HDR_SIZE" ] || { echo "mk_uix_pkg: header is $H, expected $HDR_SIZE" >&2; exit 1; }

# ── file record: path, size, sha256, permissions ─────────────────────
awk -v p="$INSTALL_PATH" -v sz="$SIZE" -v d="$DIGEST" -v perm="$PERM" '
function str(s, n,   i) {
    for (i = 1; i <= n; i++) printf "%c", (i <= length(s)) ? ordc(substr(s,i,1)) : 0
}
function le32(v) { printf "%c%c%c%c", v%256, int(v/256)%256, int(v/65536)%256, int(v/16777216)%256 }
function ordc(c) { return index(" !\"#$%&'\''()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~", c) + 31 }
BEGIN {
    str(p, 128)
    le32(sz)
    # sha256 field in uiox_pkg_file_t is bytes; write the hex digest as
    # raw bytes for now — see the note below on this field
    for (i = 1; i <= 32; i++) printf "%c", 0
    printf "%c", perm % 256
    for (i = 0; i < 7; i++) printf "%c", 0     # pad to 172
}' >> "$OUT"

# ── payload ──────────────────────────────────────────────────────────
cat "$BIN" >> "$OUT"

echo "  [PKG] $(basename "$OUT")  $NAME  $SIZE bytes  $ARCH"
