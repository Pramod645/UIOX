/*
 *  30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_getdents64.c
 *
 *  SCFS — getdents64.  The readdir path.
 *
 *  ── the layout, as namei.h defines it ────────────────────────────────
 *      typedef struct {
 *          uint32_t ino;
 *          char     name[MAX_NAME_LEN];     // 28
 *      } DirEntry;
 *
 *  FIXED-SIZE records, 16 per 512-byte block.  dir_lookup() walks them as
 *  an array, and a name is NEVER NUL-terminated by construction — it is
 *  28 bytes, filled by strncpy.  Reading it needs a bounded scan, not
 *  strlen.
 *
 *  ── the name bound, stated exactly ───────────────────────────────────
 *  dir_add() writes names with
 *          strncpy(entries[i].name, name, MAX_NAME_LEN - 1);
 *  so it can fill at most bytes 0..26 and byte 27 stays NUL.  That is the
 *  ONLY reason a 27-character name is safe to read as a C string.
 *
 *  This unit does not rely on that.  A name read off disk is untrusted
 *  input: any writer — a corrupted block, a future caller of dir_add()
 *  that passes MAX_NAME_LEN, a hand-built filesystem image — could fill
 *  all 28 bytes and leave no terminator.  So the scan is bounded to
 *  MAX_NAME_LEN and NEVER reads past the field, which is what
 *  scfs_dirent_name_len() below does.  A name that fills the field
 *  exactly is emitted in full, with the NUL added by the emitter.
 *
 *  ── resumability ─────────────────────────────────────────────────────
 *  The call must fill the caller's buffer and stop, leaving the rest for
 *  the next call.  *off carries the offset of the NEXT entry to emit and
 *  only advances past entries actually written, so a full buffer re-reads
 *  nothing it already emitted.
 *
 *  v1.2: bounded name scan — never reads past the 28-byte field.
 */
#include "uiox_kix_scfs_internal.h"

/* 16 entries per 512-byte block, from namei.h's own arithmetic. */
#define SCFS_DIRENT_PER_BLOCK (BLOCK_SIZE / (int)sizeof(DirEntry))

/*
 * The length of a name inside a DirEntry, IN BYTES, never exceeding the
 * field.  Bounded by MAX_NAME_LEN, so a field with no terminator yields
 * the full 28 rather than walking off the end.
 */
static uint32_t scfs_dirent_name_len(const char *name)
{
    uint32_t n = 0u;

    while (n < (uint32_t)MAX_NAME_LEN && name[n] != '\0') n++;
    return n;
}

/* ── Bach's FileType -> the dirent d_type a caller expects ─────────── */
static uint16_t scfs_dtype_of(FileType ft)
{
    switch (ft) {
    case FT_DIR:     return 4u;   /* DT_DIR  */
    case FT_CHAR:    return 2u;   /* DT_CHR  */
    case FT_BLOCK:   return 6u;   /* DT_BLK  */
    case FT_REGULAR: return 8u;   /* DT_REG  */
    case FT_FIFO:    return 1u;   /* DT_FIFO */
    case FT_SYMLINK: return 10u;  /* DT_LNK  */
    default:         return 0u;   /* DT_UNKNOWN */
    }
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
    if (inode_type(dp) != FT_DIR) return SCFS_ENOTDIR;

    pos = *off;

    while (pos < dp->size) {
        BmapResult bm = bmap(dp, pos);      /* 01_fsa */
        uint32_t   blk_start;
        uint32_t   slot;
        int        progressed = 0;

        /* A directory block that maps to nothing ends the walk. */
        if (!bm.valid) break;

        blk_start = pos - (pos % BLOCK_SIZE);
        slot      = (pos - blk_start) / (uint32_t)sizeof(DirEntry);

        {
            BufEntry *buf = bread(bm.blkno);     /* 01_fsa */
            DirEntry *ent;
            uint32_t  end;

            if (!buf) break;

            ent = (DirEntry *)buf->data;
            end = (uint32_t)SCFS_DIRENT_PER_BLOCK;

            while (slot < end) {
                uint32_t entry_end;
                uint32_t ino;

                ino       = ent[slot].ino;
                entry_end = blk_start + (slot + 1u) * (uint32_t)sizeof(DirEntry);

                /* An empty slot was unlinked: dir_remove sets ino = 0 and
                 * name[0] = '\0'.  Skip it — the normal case after an
                 * unlink, not corruption. */
                if (ino != 0u) {
                    /* The local copy is one byte longer than the field, so
                     * the terminator the emitter writes always fits even
                     * when the field itself is full. */
                    char     nbuf[MAX_NAME_LEN + 1u];
                    uint16_t dtype = 0u;
                    uint32_t nlen;
                    uint32_t reclen;
                    uint32_t i;

                    nlen = scfs_dirent_name_len(ent[slot].name);
                    if (nlen == 0u) { slot++; pos = entry_end; continue; }

                    /* Look the inode up so d_type can be reported.  A miss
                     * is not fatal: DT_UNKNOWN is legal and the name and
                     * number are still correct. */
                    {
                        InCoreInode *e = iget(ino);      /* 01_fsa */
                        if (e) { dtype = scfs_dtype_of(inode_type(e)); iput(e); }
                    }

                    for (i = 0u; i < nlen; i++) nbuf[i] = ent[slot].name[i];
                    nbuf[nlen] = '\0';

                    reclen = scfs_emit_dirent(out, cap, written, ino,
                                              (int64_t)entry_end, dtype,
                                              nbuf, nlen);
                    if (reclen == 0u) {
                        /* Buffer full.  pos still names THIS entry, so the
                         * next call re-reads it and nothing is lost. */
                        brelse(buf);                     /* 01_fsa */
                        *off = pos;
                        return (int32_t)written;
                    }

                    written   += reclen;
                    progressed = 1;
                }

                slot++;
                pos = entry_end;
            }

            brelse(buf);                                 /* 01_fsa */
        }

        /* Nothing readable in this block — advance past it, so a run of
         * dead slots cannot spin here forever. */
        if (!progressed) pos = blk_start + (uint32_t)BLOCK_SIZE;
    }

    *off = pos;
    return (int32_t)written;
}
