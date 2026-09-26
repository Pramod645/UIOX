/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_fchdir.c
 *
 * SCFS — fchdir, getcwd.
 * Bach, The Design of the UNIX Operating System.
 *
 * ── fchdir ─────────────────────────────────────────────────────────────
 * Bach's Algorithm chdir walks a path to an inode and stores it in the u
 * area.  fchdir is the same store with the walk already done: the
 * descriptor names the directory, so no namei and no path-name search.
 *
 *   1. the descriptor must be open and must name a DIRECTORY
 *   2. drop the old cwd's reference, adopt the new one
 *      (raising its count, because the u area now holds a second claim
 *       on an inode the file table entry already names)
 *
 * The count arithmetic is the difference from chdir: there, namei() had
 * just taken a reference that the u-area slot inherits.  Here the only
 * reference is the file table entry's, so fchdir must take one of its own
 * — otherwise close()ing the descriptor would iput() the inode out from
 * under the cwd and the next relative path would walk freed memory.
 *
 * ── getcwd ─────────────────────────────────────────────────────────────
 * Bach has no getcwd: the kernel does not record a directory's NAME, only
 * its inode.  To produce a path the walk must run backwards — from the
 * cwd's inode, scan the PARENT's data blocks for the entry whose inode
 * number matches, prepend that name, repeat until the root is reached.
 *
 * That is what this unit does, because the alternative is to keep a name
 * string in the u area and hand back a path that a rename elsewhere has
 * already falsified.
 *
 * @version 1.0.0  @date 2026-09-23
 */
#include "uiox_kix_scfs_internal.h"

/* ── fchdir ───────────────────────────────────────────────────────── */
int uiox_kix_scfs_fchdir(int fd)
{
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    if (!SCFS_IS_DIR(f->f_inode->mode)) return SCFS_ENOTDIR;

    /* The cwd slot holds a reference of its own, so take one before
     * adopting — see the note above; this is what makes close() on the
     * descriptor harmless afterwards. */
    f->f_inode->refcount++;

    InCoreInode *old = scfs_cwd_get();
    if (old && old != f->f_inode) iput(old);        /* 01_fsa */

    scfs_cwd_set(f->f_inode);
    return SCFS_OK;
}

/* ── a directory scan: find the name whose entry has this inode ───────
 * Reads the directory's data blocks a block at a time through the buffer
 * cache.  Returns the name length, or 0 when no entry matches — which for
 * a well-formed filesystem happens only at the root, whose ".." names
 * itself.
 */
/* ── scfs_dir_block_read ──────────────────────────────────────────────
 * Read one 4 KB directory block into 'blk'.
 *
 * bmap() returns a MAPPING — {dev, blkno, valid, blk_offset, io_bytes} —
 * NOT a buffer.  This function used to index r.buf->data and brelse(r.buf),
 * and BmapResult has never had a 'buf' member; the eleven errors that
 * produced were the whole of that defect.
 *
 * The unit conversion happens HERE, at the buffer boundary: bmap works in
 * UNFS 4096-byte blocks, the buffer cache in 512-byte sectors.
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

/* ── a directory scan: find the name whose entry has this inode ───────
 * Returns the name length, or 0 when no entry matches — which for a
 * well-formed filesystem happens only at the root, whose ".." names
 * itself.
 *
 * ── REWRITTEN: the entry format ──────────────────────────────────────
 * The previous body walked BACH's entry layout:
 *
 *      uint16_t e_ino = data[p] | (data[p+1] << 8);   // 2-byte inode
 *      uint32_t q     = p + 2u;                       // name follows
 *      while (data[q] != 0) { q++; nlen++; }          // NUL-terminated
 *
 * UNFS stores something else — unfs_format.h's unfs_dirent_t:
 *
 *      offset 0   uint32_t d_ino        (4, not 2)
 *      offset 4   uint16_t d_rec_len    (this record's own length)
 *      offset 6   uint8_t  d_name_len   (read, not scanned for)
 *      offset 7   uint8_t  d_type
 *      offset 8   char     d_name[]
 *
 * So the old walk read a 4-byte inode as two 2-byte halfwords, then found
 * a "name length" by scanning for a NUL that is not the field's terminator.
 * It would have returned wrong names on any real volume, or none at all.
 * namei.h's dirent_reclen() / dirent_next() / dirent_match() do the walk
 * now, which is also what dir_lookup() uses — so the two cannot disagree.
 * ───────────────────────────────────────────────────────────────────── */
static uint32_t scfs_dirname_of(InCoreInode *dp, uint32_t ino,
                                char *out, uint32_t outsz)
{
    uint32_t off = 0u;
    uint8_t  blk[UNFS_BLOCK_SIZE];

    while (off < dp->size) {
        BmapResult r = bmap(dp, off);               /* 01_fsa */

        if (!r.valid) break;
        if (!scfs_dir_block_read(r.dev, r.blkno, blk)) break;

        {
            uint8_t  *end = blk + (uint32_t)UNFS_BLOCK_SIZE;
            DirEntry *de  = (DirEntry *)blk;

            while ((uint8_t *)de < end) {
                if (de->d_name_len == 0u) break;    /* end of entries */

                if (de->d_ino == ino) {
                    uint32_t nlen = (uint32_t)de->d_name_len;

                    if (nlen == 0u || nlen >= outsz) return 0u;

                    memcpy(out, de->d_name, (size_t)nlen);
                    out[nlen] = '\0';
                    return nlen;
                }

                de = dirent_next(de, end);
                if (!de) break;
            }
        }

        off += UNFS_BLOCK_SIZE;                     /* one 4 KB block */
    }

    return 0u;
}

/* Look up the entry named ".." in a directory and read its inode out. */
static uint32_t scfs_parent_ino(InCoreInode *dp)
{
    static const char dotdot[3] = { '.', '.', '\0' };
    (void)dotdot;

    InCoreInode *p = (InCoreInode *)0;
    uint32_t     ino = 0u;

    /* dir_lookup() resolves one fixed name, which is all ".." needs. */
    ino = dir_lookup(dp, "..", 2u);                 /* 01_fsa */

    /* A directory whose ".." names itself is the root. */
    if (ino == dp->ino) return 0u;

    (void)p;
    return ino;
}

/* ── getcwd ───────────────────────────────────────────────────────── */
int uiox_kix_scfs_getcwd(char *buf, uint32_t size)
{
    char         parts[SCFS_PATH_MAX];
    uint32_t     plen = 0u;
    InCoreInode *cur;

    if (!buf || size == 0u) return SCFS_EINVAL;

    cur = scfs_cwd_get();
    if (!cur) { buf[0] = '\0'; return SCFS_ENOENT; }

    cur->refcount++;                                /* borrow the cwd */

    /* Walk upward, collecting one component per level. */
    while (1) {
        uint32_t pino = scfs_parent_ino(cur);

        if (pino == 0u) break;                      /* reached the root */

        InCoreInode *par = iget(pino);              /* 01_fsa */
        if (!par) break;

        char     name[SCFS_PATH_MAX];
        uint32_t nlen = scfs_dirname_of(par, cur->ino, name, sizeof(name));

        iput(par);                                  /* 01_fsa */

        if (nlen == 0u) break;                      /* unlinked — stop */

        /* Prepend "name/" to parts.  Collecting backwards and reversing
         * once would be tidier; collecting backwards into the buffer's
         * front is what the fixed-size array makes cheap. */
        if (nlen + 1u + plen >= sizeof(parts)) { iput(cur); return SCFS_ERANGE; }

        for (uint32_t i = plen; i > 0u; i--) parts[i + nlen] = parts[i - 1u];
        for (uint32_t i = 0u; i < nlen; i++) parts[i] = name[i];
        parts[nlen] = '/';
        plen += nlen + 1u;

        InCoreInode *next = iget(pino);
        iput(cur);
        if (!next) { cur = (InCoreInode *)0; break; }
        cur = next;
    }

    if (cur) iput(cur);                             /* 01_fsa */

    /* Assemble "/" + parts, or "/" for the root itself. */
    {
        uint32_t o = 0u;
        if (size < 2u) return SCFS_ERANGE;
        buf[o++] = '/';
        for (uint32_t i = 0u; i < plen && o + 1u < size; i++)
            buf[o++] = parts[i];
        /* A trailing slash at the root is the one harmless redundancy. */
        buf[o] = '\0';
    }

    return SCFS_OK;
}
