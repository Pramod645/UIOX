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

    if (!SCFS_S_ISDIR(f->f_inode->mode)) return SCFS_ENOTDIR;

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
static uint32_t scfs_dirname_of(InCoreInode *dp, uint32_t ino,
                                char *out, uint32_t outsz)
{
    uint32_t off = 0u;

    while (off < dp->size) {
        BmapResult r = bmap(dp, off);               /* 01_fsa */
        if (!r.valid || !r.buf) break;

        /* The block offset within the directory that this buffer holds. */
        uint32_t blk_start = off - (off % BLKSIZE);
        uint32_t p         = off - blk_start;

        while (p < BLKSIZE) {
            /* An entry is Bach's: a 2-byte inode number, then the name.
             * An inode number of 0 means the slot is empty — removed. */
            uint16_t e_ino = (uint16_t)((uint16_t)r.buf->data[p] |
                                        ((uint16_t)r.buf->data[p + 1] << 8));
            uint32_t nlen  = 0u;
            uint32_t q     = p + 2u;

            if (e_ino == 0u) {
                /* Empty slot: skip the whole entry.  Without an explicit
                 * length field the entry is name-terminated, so count to
                 * the NUL and round up. */
                while (q < BLKSIZE && r.buf->data[q] != 0u) q++;
                q++;                                   /* past the NUL */
                p = q;
                continue;
            }

            uint32_t name_at = q;
            while (q < BLKSIZE && r.buf->data[q] != 0u) { q++; nlen++; }
            if (q >= BLKSIZE) break;                   /* spans the block */

            if (e_ino == (uint16_t)ino) {
                if (nlen == 0u || nlen >= outsz) nlen = 0u;
                else {
                    for (uint32_t i = 0u; i < nlen; i++)
                        out[i] = r.buf->data[name_at + i];
                    out[nlen] = '\0';
                }
                brelse(r.buf);                     /* 01_fsa */
                return nlen;
            }

            q++;
            p = q;
        }

        brelse(r.buf);                             /* 01_fsa */
        off = blk_start + BLKSIZE;
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
    ino = dir_lookup(dp, "..");                    /* 01_fsa */

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
