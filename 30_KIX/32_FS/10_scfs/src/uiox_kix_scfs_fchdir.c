#include "uiox_kix_scfs_internal.h"

/*
 * fchdir — chdir with the walk already done.  The count arithmetic is the
 * difference from chdir: there, namei() had just taken a reference that
 * the u-area slot inherits.  Here the only reference is the file table
 * entry's, so fchdir must take one of its own — otherwise close()ing the
 * descriptor would iput() the inode out from under the cwd.
 */
int uiox_kix_scfs_fchdir(int fd)
{
    scfs_file_t *f = scfs_getf(fd);
    InCoreInode *old;

    if (!f) return SCFS_EBADF;
    if (!SCFS_IS_DIR(f->f_inode->mode)) return SCFS_ENOTDIR;

    f->f_inode->refcount++;

    old = scfs_cwd_get();
    if (old && old != f->f_inode) iput(old);

    scfs_cwd_set(f->f_inode);
    return SCFS_OK;
}

/*
 * A directory scan: find the entry whose inode number matches.
 * Returns the name length, or 0 when no entry matches — which for a
 * well-formed filesystem happens only at the root, whose ".." is itself.
 */
static uint32_t scfs_dirname_of(InCoreInode *dp, uint32_t ino,
                                char *out, uint32_t outsz)
{
    uint32_t off = 0u;

    while (off < dp->size) {
        BmapResult bm = bmap(dp, off);          /* 01_fsa */
        BufEntry  *buf;
        DirEntry  *ent;
        uint32_t   blk_start;
        uint32_t   slot;
        uint32_t   end;

        if (!bm.valid) break;

        buf = bread(bm.blkno);                  /* 01_fsa */
        if (!buf) break;

        blk_start = off - (off % (uint32_t)BLOCK_SIZE);
        slot      = (off - blk_start) / (uint32_t)sizeof(DirEntry);
        end       = (uint32_t)BLOCK_SIZE / (uint32_t)sizeof(DirEntry);
        ent       = (DirEntry *)buf->data;

        while (slot < end) {
            if (ent[slot].ino == (uint32_t)ino) {
                uint32_t n = 0u;
                uint32_t i;

                /* names are fixed-size fields, never NUL-terminated by
                 * construction — scan bounded to the field width */
                while (n < (uint32_t)MAX_NAME_LEN && ent[slot].name[n]) n++;

                if (n != 0u && n < outsz) {
                    for (i = 0u; i < n; i++) out[i] = ent[slot].name[i];
                    out[n] = '\0';
                } else {
                    n = 0u;
                }

                brelse(buf);                    /* 01_fsa */
                return n;
            }
            slot++;
        }

        brelse(buf);                            /* 01_fsa */
        off = blk_start + (uint32_t)BLOCK_SIZE;
    }

    return 0u;
}

/* Look up the entry named ".." in a directory. */
static uint32_t scfs_parent_ino(InCoreInode *dp)
{
    uint32_t ino = dir_lookup(dp, "..");        /* 01_fsa */
    if (ino == dp->ino) return 0u;              /* root names itself */
    return ino;
}

/*
 * getcwd — Bach has no such call: the kernel does not record a
 * directory's NAME, only its inode.  To produce a path the walk runs
 * backwards, collecting one component per level.  Keeping a path string
 * in the u area instead would hand back a name a rename elsewhere had
 * already falsified.
 */
int uiox_kix_scfs_getcwd(char *buf, uint32_t size)
{
    char         parts[SCFS_PATH_MAX];
    uint32_t     plen = 0u;
    InCoreInode *cur;

    if (!buf || size == 0u) return SCFS_EINVAL;

    cur = scfs_cwd_get();
    if (!cur) { buf[0] = '\0'; return SCFS_ENOENT; }

    cur->refcount++;

    while (1) {
        uint32_t     pino = scfs_parent_ino(cur);
        InCoreInode *par;
        InCoreInode *next;
        char         name[SCFS_PATH_MAX];
        uint32_t     nlen;
        uint32_t     i;

        if (pino == 0u) break;                  /* reached the root */

        par = iget(pino);                       /* 01_fsa */
        if (!par) break;

        nlen = scfs_dirname_of(par, cur->ino, name, sizeof(name));
        iput(par);

        if (nlen == 0u) break;                  /* unlinked — stop */

        if (nlen + 1u + plen >= sizeof(parts)) {
            iput(cur);
            return SCFS_ERANGE;
        }

        /* prepend "name/" */
        for (i = plen; i > 0u; i--) parts[i + nlen] = parts[i - 1u];
        for (i = 0u; i < nlen; i++) parts[i] = name[i];
        parts[nlen] = '/';
        plen += nlen + 1u;

        next = iget(pino);
        iput(cur);
        if (!next) { cur = (InCoreInode *)0; break; }
        cur = next;
    }

    if (cur) iput(cur);

    {
        uint32_t o = 0u;
        uint32_t i;

        if (size < 2u) return SCFS_ERANGE;
        buf[o++] = '/';
        for (i = 0u; i < plen && o + 1u < size; i++) buf[o++] = parts[i];
        buf[o] = '\0';
    }

    return SCFS_OK;
}
