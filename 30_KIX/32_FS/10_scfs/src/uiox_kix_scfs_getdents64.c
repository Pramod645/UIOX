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

/* Bach's FileType -> the dirent d_type a caller expects */
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

/* build one dirent64; return its record length, or 0 if no room */
static uint32_t scfs_emit_dirent(uint8_t *out, uint32_t cap, uint32_t o,
                                 uint32_t ino, int64_t next_off,
                                 uint16_t dtype, const char *name,
                                 uint32_t namelen)
{
    uint32_t need   = 19u + namelen + 1u;   /* 8+8+2+1 + name + NUL */
    uint32_t reclen = (need + 7u) & (uint32_t)~7u;
    uint8_t *p;
    uint32_t i;

    if (o + reclen > cap) return 0u;        /* buffer full — stop here */

    p = out + o;

    *(uint64_t *)(p +  0) = (uint64_t)ino;
    *(int64_t  *)(p +  8) = next_off;
    *(uint16_t *)(p + 16) = (uint16_t)reclen;
    *(uint8_t  *)(p + 18) = (uint8_t)dtype;

    for (i = 0u; i < namelen; i++) p[19u + i] = (uint8_t)name[i];
    p[19u + namelen] = 0u;

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
    if (!SCFS_IS_DIR(dp->mode)) return SCFS_ENOTDIR;

    pos = *off;

    while (pos < dp->size) {
        BmapResult bm = bmap(dp, pos);      /* 01_fsa */
        uint32_t   blk_start;
        uint32_t   slot;
        int        progressed = 0;

        /* A directory block that maps to nothing ends the walk. */
        if (!bm.valid) break;

        blk_start = pos - (pos % (uint32_t)BLOCK_SIZE);
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
                    /* One byte longer than the field, so the terminator the
                     * emitter writes always fits even when the field is
                     * full. */
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
