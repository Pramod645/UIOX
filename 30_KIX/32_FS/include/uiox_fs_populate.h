/* 30_KIX/32_FS/include/uiox_fs_populate.h */
#ifndef UIOX_FS_POPULATE_H
#define UIOX_FS_POPULATE_H

#include "bmap.h"        /* bmap, BmapResult — 01_fsa, not 10_unfs */
#include "uiox_pkg_store.h"   /* uiox_pkg_index_hdr_t, _entry_t, MAGIC */

/* ── the package store ──────────────────────────────────────────────
 *
 * Two passes over the packed blob, and the ORDER is forced:
 *
 *   1. write each .upkg into /pkg, and for each one record the block it
 *      landed at.  That block number is archive_lba, and it is knowable
 *      only here, after the file exists — which is why the host packer
 *      does not write the index.
 *
 *   2. write /pkg/index.upix LAST, one 4096-byte block for the header
 *      and one 4096-byte block per entry, per uiox_pkg_store.h's own
 *      description of the on-disk format.
 *
 * n is bounded by UIOX_PKG_INDEX_MAX_ENTRIES (256) on the way in; the
 * reader clamps again at uiox_pkg_store.c:202.
 */
#define PKG_DIR_NAME  "pkg"
#define PKG_IDX_NAME  "index.upix"


/*
 * Populate the volume at first boot.
 *
 * Returns 0 on success, -1 on failure.  On success *out_index_blk holds
 * the UNFS block of /pkg/index.upix, or 0 when no store was embedded or
 * the store's own write failed.
 *
 * The index block is an OUTPUT because only the writer knows it: it is
 * this filesystem's allocation, and the same reason each entry's
 * archive_lba is recorded rather than derived.  uiox_pkg_store_mount
 * refuses to run without it (uiox_pkg_store.c:157).
 */
int uiox_fs_populate_root(uiox_uint32_t *out_index_blk);


#endif /* UIOX_FS_POPULATE_H */
