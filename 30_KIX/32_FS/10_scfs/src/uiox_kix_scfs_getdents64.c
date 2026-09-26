/*
 *  30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_getdents64.c
 *
 *  SCFS — getdents64.  The readdir path.
 *
 *  ── the layout, as unfs_format.h defines it ─────────────────────────
 *      struct unfs_dirent {          // VARIABLE length, 4-aligned 
 *          uint32_t d_ino          ;   offset 0
 *          uint16_t d_rec_len      ;   offset 4  this record's own length
 *          uint8_t  d_name_len     ;   offset 6  bytes of d_name, no NUL
 *          uint8_t  d_type         ;   offset 7  UNFS_DT_* / UNFS_FT_*
 *          char     d_name[]       ;   offset 8
 *      } __attribute__((packed));
 *
 *  NOT the fixed 32-byte record this file was written against.  The
 *  previous body walked entries as a DirEntry ARRAY —
 *
 *      ent[slot].ino        ent[slot].name        slot * sizeof(DirEntry)
 *
 *  — which is what produced ten compile errors at once: DirEntry has no
 *  member 'ino' or 'name', and a variable-length record cannot be indexed
 *  by a fixed stride at all.  namei.h's dirent_reclen()  dirent_next()
 *  walk the block now, so this file and dir_lookup() dir_add() cannot
 *  disagree about entry layout.
 *
 *  ── the name bound ───────────────────────────────────────────────────
 *  UNFS_NAME_MAX is 255, and d_name_len is what the record itself records.
 *  A name is not NUL-terminated on disk, so the emitter adds the
 *  terminator — the local buffer is one byte longer than the longest
 *  legal name for exactly that reason.
 *
 *  ── resumability ─────────────────────────────────────────────────────
 *  The call fills the caller's buffer and stops, leaving the rest for the
 *  next call.  *off carries the byte offset of the NEXT entry, computed
 *  from each record's own d_rec_len, so a full buffer re-reads nothing it
 *  already emitted.
 */
#include "uiox_kix_scfs_internal.h"

/* Entries per 4 KB block is no longer a constant: UNFS dirents are
 * variable-length, so a block holds between one long-named entry and
 * (UNFS_BLOCK_SIZE / DIRENT_HDR_SIZE) minimal ones.  The bound below is
 * the worst case, used only to cap an iteration. */
/* No fixed entry count: UNFS dirents are variable-length.  The block
 * bound is UNFS_BLOCK_SIZE and the walk advances by d_rec_len. */

/* ── i_mode -> the dirent d_type a caller expects ──────────────────────
 * The conversion lives in uiox_kix_scfs.h now (scfs_dtype_of_mode), so
 * this file and dir_add() cannot disagree about it.  Kept as a thin
 * wrapper because the call site below is unchanged. */
static uint16_t scfs_dtype_of(uint16_t mode)
{
    return (uint16_t)scfs_dtype_of_mode(mode);
}

/* ── scfs_dir_block_read ──────────────────────────────────────────────
 * Read one 4 KB directory block into 'blk'.
 *
 * bmap() returns a MAPPING — {dev, blkno, valid, blk_offset, io_bytes} —
 * NOT a buffer, and bread() takes (dev, blkno), not a blkno alone.  The
 * previous body called bread(bm.blkno) and indexed buf->data, which is
 * the other half of the ten errors.
 *
 * The unit conversion lives HERE: bmap works in UNFS 4096-byte blocks,
 * the buffer cache in 512-byte sectors.
 * ───────────────────────────────────────────────────────────────────── */
static bool scfs_dir_block_read(uint8_t dev, uint32_t blkno, uint8_t *blk)
{
    uint32_t s;

    for (s = 0u; s < (uint32_t)UNFS_SECTORS_PER_BLOCK; s++) {
        BufHdr *buf = bread(dev, blkno * (uint32_t)UNFS_SECTORS_PER_BLOCK + s);
        if (!buf) return false;

        memcpy(blk + (s * (uint32_t)BCACHE_SECTOR_SIZE),
               buf->data, (size_t)BCACHE_SECTOR_SIZE);

        brelse(buf);
    }
    return true;
}

/* ── build one dirent64; return its record length, or 0 if no room ─── */
static uint32_t scfs_emit_dirent(uint8_t *out, uint32_t cap, uint32_t o,
                                 uint32_t ino, int64_t next_off,
                                 uint16_t dtype, const char *name,
                                 uint32_t namelen)
{
    /* 8 d_ino + 8 d_off + 2 d_reclen + 1 d_type = 19, then name + NUL,
     * padded to an 8-byte boundary so a caller can walk with pointer
     * arithmetic. */
    uint32_t need   = 19u + namelen + 1u;
    uint32_t reclen = (need + 7u) & (uint32_t)~7u;
    uint8_t *p;
    uint32_t i;

    if (o + reclen > cap) return 0u;      /* buffer full — stop here */

    p = out + o;

    *(uint64_t *)(p +  0) = (uint64_t)ino;
    *(int64_t  *)(p +  8) = next_off;
    *(uint16_t *)(p + 16) = (uint16_t)reclen;
    *(uint8_t  *)(p + 18) = (uint8_t)dtype;

    /* namelen is <= MAX_NAME_LEN by the caller's scan, and the caller's
     * buffer is MAX_NAME_LEN + 1 so the terminator below always fits. */
    for (i = 0u; i < namelen; i++) p[19u + i] = (uint8_t)name[i];
    p[19u + namelen] = 0u;

    /* Zero the pad so a walker never reads stale bytes. */
    for (i = 19u + namelen + 1u; i < reclen; i++) p[i] = 0u;

    return reclen;
}

int uiox_kix_scfs_getdents64(int fd, uint8_t *out, uint32_t cap, uint32_t *off)
{
    scfs_file_t *f;
    InCoreInode *dp;
    uint32_t     written = 0u;
    uint32_t     pos;

    if (!out || !off) return SCFS_EFAULT;

    f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    dp = f->f_inode;
    if (!inode_is_dir(dp)) return SCFS_ENOTDIR;

    pos = *off;

    while (pos < dp->size) {
        BmapResult bm = bmap(dp, pos);      /* 01_fsa */

        /* A directory block that maps to nothing ends the walk. */
        if (!bm.valid) break;

        {
            /* One 4 KB UNFS directory block, assembled from its eight
             * 512-byte sectors.  The walk below advances by each record's
             * OWN d_rec_len — never by a fixed stride. */
            uint8_t  blk[UNFS_BLOCK_SIZE];
            uint8_t *end;
            DirEntry *de;

            if (!scfs_dir_block_read(bm.dev, bm.blkno, blk)) break;

            end = blk + (uint32_t)UNFS_BLOCK_SIZE;
            de  = (DirEntry *)blk;

            while ((uint8_t *)de < end) {
                /* d_name_len 0 terminates the block's entries. */
                if (de->d_name_len == 0u) break;

                /* Advance by the record's own length before any early
                 * exit, so 'pos' always names the NEXT entry. */
                {
                    uint32_t reclen_hdr = dirent_reclen(de);
                    uint32_t entry_end  = pos + reclen_hdr;

                    if (de->d_ino != 0u && de->d_name_len != 0u) {
                        uint32_t nlen = (uint32_t)de->d_name_len;
                        uint16_t dtype = 0u;
                        uint32_t reclen;

                        if (nlen > (uint32_t)UNFS_NAME_MAX)
                            nlen = (uint32_t)UNFS_NAME_MAX;

                        /* d_type is on disk, so no iget() is needed to
                         * report it — the header says exactly that:
                         * "so a walker needs no iget() per entry".  Fall
                         * back to a lookup only when the field is 0. */
                        dtype = (uint16_t)de->d_type;
                        if (dtype == 0u) {
                            InCoreInode *e = iget(de->d_ino);   /* 01_fsa */
                            if (e) { dtype = scfs_dtype_of(e->mode); iput(e); }
                        }

                        reclen = scfs_emit_dirent(out, cap, written,
                                                  de->d_ino,
                                                  (int64_t)entry_end,
                                                  dtype,
                                                  de->d_name, nlen);
                        if (reclen == 0u) {
                            /* Buffer full.  'pos' still names THIS entry
                             * because entry_end was not committed, so the
                             * next call re-reads it and nothing is lost. */
                            *off = pos;
                            return (int32_t)written;
                        }

                        written += reclen;
                    }

                    pos = entry_end;
                }

                de = dirent_next(de, end);
                if (!de) break;
            }
        }

        /* The block is consumed — move to the next 4 KB unit whether or
         * not anything was emitted, so a run of dead slots cannot spin
         * here forever. */
        pos = ((pos / (uint32_t)UNFS_BLOCK_SIZE) + 1u) * (uint32_t)UNFS_BLOCK_SIZE;
    }

    *off = pos;
    return (int32_t)written;
}
