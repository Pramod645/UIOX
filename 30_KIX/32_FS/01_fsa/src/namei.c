/*
 *  30_KIX/32_FS/01_fsa/src/namei.c
 *
 *  Algorithm namei, and the three directory operations it uses.
 *  Bach, The Design of the UNIX Operating System, Ch.4 §4.
 *
 *  ── Bach's Algorithm namei, verbatim ─────────────────────────────────
 *  input: path name
 *  output: locked inode
 *  {
 *      if (path name starts from root)
 *          working inode = root inode (algorithm iget);
 *      else
 *          working inode = current directory inode (algorithm iget);
 *      while (there is more path name)
 *      {
 *          read next path name component from input;
 *          verify that working inode is of directory, access permission OK;
 *          if (working inode is of root and component is "..")
 *              continue;   // loop back to while
 *          read directory (working inode by repeated use of algorithm
 *              bmap, bread and brelse);
 *          if (component matches an entry in directory (working inode))
 *          {
 *              get inode number for matched component;
 *              release working inode (algorithm iput);
 *              working inode = inode of matched component (algorithm iget);
 *          }
 *          else return (no inode);
 *      }
 *      return (working inode);
 *  }
 *
 *  ── CHANGED in this revision (v3.1.0) ─────────────────────────────────
 *  1. The dirent header is 8 bytes, and d_rec_len is READ, not computed.
 *
 *     The field order is {d_ino, d_rec_len, d_name_len, d_type, d_name[]}
 *     per unfs_format.h.  An earlier revision wrote d_type into the byte
 *     that is d_rec_len's low half — which corrupted each entry's own
 *     length the moment it was written.  Every write site below now sets
 *     d_rec_len from dirent_needed().
 *
 *     dirent_reclen() reads d_rec_len because the stored length is
 *     authoritative: the LAST entry in a block owns the block's spare
 *     bytes, and a freed entry keeps the slot it was given.  Recomputing
 *     from d_name_len would re-derive a shorter number.
 *
 *  2. The walk device is hoisted at entry, which fixes the walk.
 *
 *     namei() previously dereferenced 'cwd' at the start and then, after
 *     iput()ing the working inode, had no device left for the components
 *     that followed — the file carried a placeholder in that position.
 *     A directory entry stores only an inode NUMBER, so the device has to
 *     travel with the walk.  It is captured once, before the loop, and
 *     every iget_dev() below uses it.
 *
 *  3. The block unit is 4096 while the buffer unit is 512.
 *
 *     Every directory block is read through dir_block_read(), which
 *     assembles UNFS_SECTORS_PER_BLOCK (8) consecutive sectors.  The old
 *     code stepped BLOCK_SIZE and passed bm.blkno straight to bread().
 *
 *  4. the LOG POD
 *
 *     printf() is gone.  The 32_FS session established that 01_fsa must
 *     not pull in three colliding uiox_printf() prototypes, so the
 *     diagnostics are removed rather than converted — nothing below
 *     needs a stdio declaration.
 *
 *  ── what is still not done ────────────────────────────────────────────
 *  dir_add's pass 3 needs extent allocation, and bmap_alloc() is still a
 *  stub.  It returns -1 rather than pretending.  superblock.h DOES
 *  provide the primitive it wants — fs_alloc_begin(dev, &blkno_out) is
 *  documented as the split form specifically so a caller can attach the
 *  block number before dirtying the buffer — so the work is wiring, not
 *  invention.
 *
 *  @version 3.1.0  @date 2026-09-26
 */
#include "namei.h"
#include "bmap.h"
#include "superblock.h"
#include "buffer.h"
#include "uiox_klibc.h"

/* ═════════════════════════════════════════════════════════════
 * Internal: read one 4 KB directory block into 'blk'.
 *
 * bm is an extent address in UNFS units; the buffer lives in 512-byte
 * sectors.  Returns true when every sector of the block came back.
 * ═════════════════════════════════════════════════════════════ */
static bool dir_block_read(const BmapResult *bm, uint8_t *blk)
{
    uint32_t s;

    for (s = 0u; s < (uint32_t)UNFS_SECTORS_PER_BLOCK; s++) {
        BufHdr *buf = bread(bm->dev,
                            bm->blkno * (uint32_t)UNFS_SECTORS_PER_BLOCK + s);
        if (!buf) return false;

        memcpy(blk + (s * (uint32_t)BCACHE_SECTOR_SIZE),
               buf->data,
               (size_t)BCACHE_SECTOR_SIZE);

        brelse(buf);
    }
    return true;
}

/* ═════════════════════════════════════════════════════════════
 * Internal: write one 4 KB directory block back.
 * ═════════════════════════════════════════════════════════════ */
static bool dir_block_write(uint8_t dev, uint32_t blkno, const uint8_t *blk)
{
    uint32_t s;

    for (s = 0u; s < (uint32_t)UNFS_SECTORS_PER_BLOCK; s++) {
        BufHdr *buf = bread(dev, blkno * (uint32_t)UNFS_SECTORS_PER_BLOCK + s);
        if (!buf) return false;

        memcpy(buf->data,
               blk + (s * (uint32_t)BCACHE_SECTOR_SIZE),
               (size_t)BCACHE_SECTOR_SIZE);

        bwrite(buf, true, false);       /* persist + release */
    }
    return true;
}

/* ═════════════════════════════════════════════════════════════
 * Internal: write one entry into a slot, at a given record length.
 *
 * d_rec_len is what the walk trusts, so it is set from the space this
 * entry is given — which may be LARGER than the name needs when the slot
 * is the last in a block and has absorbed the spare bytes.
 * ═════════════════════════════════════════════════════════════ */
static void dir_ent_store(DirEntry *de, uint32_t rec_len,
                          uint32_t ino, uint8_t type,
                          const char *name, uint32_t len)
{
    de->d_ino      = ino;
    de->d_rec_len  = (uint16_t)rec_len;
    de->d_name_len = (uint8_t)len;
    de->d_type     = type;

    memcpy(de->d_name, name, (size_t)len);
}

/* ═════════════════════════════════════════════════════════════
 * Internal: find the LAST live entry in a block — the one whose record
 * owns the block's spare bytes.  Returns NULL on an empty block.
 * ═════════════════════════════════════════════════════════ */
static DirEntry *dir_last_entry(uint8_t *blk)
{
    uint8_t   *end  = blk + (uint32_t)UNFS_BLOCK_SIZE;
    DirEntry  *de   = (DirEntry *)blk;
    DirEntry  *last = NULL;

    while ((uint8_t *)de < end) {
        if (de->d_name_len == 0u) break;        /* terminating slot */
        last = de;
        de   = dirent_next(de, end);
        if (!de) break;
    }
    return last;
}

/* ═════════════════════════════════════════════════════════════
 * dir_lookup — search a directory for a component.
 *
 * 'len' is the component length; names are NOT NUL-terminated on disk.
 * Returns the inode number of the match, or 0 if not found.
 * ═════════════════════════════════════════════════════════════ */
uint32_t dir_lookup(InCoreInode *dir, const char *name, uint32_t len)
{
    uint32_t offset = 0u;
    uint8_t  blk[UNFS_BLOCK_SIZE];

    if (!dir || !name || len == 0u) return 0u;
    if (len > (uint32_t)UNFS_NAME_MAX) return 0u;
    if ((dir->mode & UNFS_IFMT) != UNFS_IFDIR) return 0u;

    while (offset < dir->size) {
        BmapResult bm = bmap(dir, offset);
        uint8_t   *end;
        DirEntry  *de;

        if (!bm.valid) break;
        if (!dir_block_read(&bm, blk)) break;

        end = blk + (uint32_t)UNFS_BLOCK_SIZE;
        de  = (DirEntry *)blk;

        while ((uint8_t *)de < end) {
            if (de->d_name_len == 0u) break;    /* end of entries */

            if (dirent_match(de, name, len)) {
                return de->d_ino;
            }

            de = dirent_next(de, end);
            if (!de) break;
        }

        offset += UNFS_BLOCK_SIZE;              /* one 4 KB block */
    }
    return 0u;
}

/* ═════════════════════════════════════════════════════════════
 * dir_add — add a (name, ino, type) entry to a directory.
 * Returns 0 on success, -1 on failure.
 *
 * Three cases, in Bach's order:
 *   1. a free slot inside an existing block, where a removed entry left
 *      d_ino == 0 with enough room
 *   2. the free tail of the last entry in an existing block
 *   3. a brand-new block at the end of the directory
 * ═════════════════════════════════════════════════════════════ */
int dir_add(InCoreInode *dir, const char *name, uint32_t len,
            uint32_t ino, uint8_t type)
{
    uint32_t offset;
    uint32_t need;

    if (!dir || !name || ino == 0u)     return -1;
    if (len == 0u)                      return -1;
    if (len > (uint32_t)UNFS_NAME_MAX)  return -1;
    if ((dir->mode & UNFS_IFMT) != UNFS_IFDIR) return -1;

    need = dirent_needed(len);
    if (need > (uint32_t)UNFS_BLOCK_SIZE) return -1;

    /* ── pass 1: a free slot inside a block ───────────────────────────
     * A removed entry keeps its bytes and sets d_ino = 0, with d_rec_len
     * still describing the space it owns. */
    offset = 0u;
    while (offset < dir->size) {
        BmapResult bm = bmap(dir, offset);
        uint8_t    blk[UNFS_BLOCK_SIZE];
        uint8_t   *end;
        DirEntry  *de;

        if (!bm.valid) break;
        if (!dir_block_read(&bm, blk)) return -1;

        end = blk + (uint32_t)UNFS_BLOCK_SIZE;
        de  = (DirEntry *)blk;

        while ((uint8_t *)de < end) {
            if (de->d_name_len == 0u) break;

            if (dirent_free(de) && dirent_reclen(de) >= need) {
                dir_ent_store(de, dirent_reclen(de),
                              ino, type, name, len);

                if (!dir_block_write(bm.dev, bm.blkno, blk)) return -1;

                dir->flags |= IFLAG_MODIFIED | IFLAG_CHANGED;
                return 0;
            }

            de = dirent_next(de, end);
            if (!de) break;
        }

        offset += UNFS_BLOCK_SIZE;
    }

    /* ── pass 2: the free tail of the last entry in a block ──────────
     * Bach's scheme, and ext2's: the last entry owns the block's spare
     * bytes.  The new entry takes 'need' of them, the tail keeps the
     * rest, and the tail's d_rec_len is rewritten to its new (shorter)
     * length so the walk still steps correctly. */
    offset = 0u;
    while (offset < dir->size) {
        BmapResult bm = bmap(dir, offset);
        uint8_t    blk[UNFS_BLOCK_SIZE];
        DirEntry  *last;
        DirEntry  *nw;
        uint32_t   start_off;
        uint32_t   keep;
        uint32_t   spare;

        if (!bm.valid) break;
        if (!dir_block_read(&bm, blk)) return -1;

        last = dir_last_entry(blk);
        if (!last) { offset += UNFS_BLOCK_SIZE; continue; }

        start_off = (uint32_t)((uint8_t *)last - blk);

        /* the tail's record must be big enough to hold its own name plus
         * the new entry, or there is no room to split it */
        keep = dirent_needed((uint32_t)last->d_name_len);
        if (start_off + keep + need > (uint32_t)UNFS_BLOCK_SIZE) {
            offset += UNFS_BLOCK_SIZE;
            continue;
        }

        spare = dirent_reclen(last) - keep;

        if (spare >= need) {
            /* shrink the tail to its minimum, then place the new entry */
            last->d_rec_len = (uint16_t)keep;

            nw = (DirEntry *)((uint8_t *)last + keep);
            dir_ent_store(nw, spare, ino, type, name, len);

            if (!dir_block_write(bm.dev, bm.blkno, blk)) return -1;

            dir->flags |= IFLAG_MODIFIED | IFLAG_CHANGED;
            return 0;
        }

        offset += UNFS_BLOCK_SIZE;
    }

    /* ── pass 3: extend the directory by one block ───────────────────
     * bmap_alloc() is NOT implemented for extents — it reports an
     * invalid mapping rather than writing into a struct that has no
     * addr[] field.  So this path cannot succeed yet, and it returns -1
     * instead of pretending.  superblock.h's fs_alloc_begin(dev, &blkno)
     * is the primitive that will fill this in. */
    {
        BmapResult bm = bmap_alloc(dir, dir->size);
        uint8_t    blk[UNFS_BLOCK_SIZE];
        DirEntry  *first;

        if (!bm.valid) return -1;               /* ◀ allocation pending */

        memset(blk, 0, sizeof blk);
        first = (DirEntry *)blk;

        /* the single entry in a fresh block owns the WHOLE block */
        dir_ent_store(first, (uint32_t)UNFS_BLOCK_SIZE,
                      ino, type, name, len);

        if (!dir_block_write(bm.dev, bm.blkno, blk)) return -1;

        dir->size += UNFS_BLOCK_SIZE;
        dir->flags |= IFLAG_MODIFIED | IFLAG_CHANGED;
        return 0;
    }
}

/* ═════════════════════════════════════════════════════════════
 * dir_remove — remove the entry with 'name'.
 * Returns 0 on success, -1 if not found.
 *
 * Bach zeroes the INODE NUMBER and leaves the name bytes; that is what
 * makes the slot reusable by dir_add and recognisable as empty by
 * getdents64.  d_rec_len and d_name_len are RETAINED so the slot still
 * describes the space it owns — clearing either would make the walk stop
 * early or step wrong and lose every entry after it.
 * ═════════════════════════════════════════════════════════════ */
int dir_remove(InCoreInode *dir, const char *name, uint32_t len)
{
    uint32_t offset = 0u;
    uint8_t  blk[UNFS_BLOCK_SIZE];

    if (!dir || !name || len == 0u) return -1;
    if ((dir->mode & UNFS_IFMT) != UNFS_IFDIR) return -1;

    while (offset < dir->size) {
        BmapResult bm = bmap(dir, offset);
        uint8_t   *end;
        DirEntry  *de;

        if (!bm.valid) break;
        if (!dir_block_read(&bm, blk)) return -1;

        end = blk + (uint32_t)UNFS_BLOCK_SIZE;
        de  = (DirEntry *)blk;

        while ((uint8_t *)de < end) {
            if (de->d_name_len == 0u) break;

            if (de->d_ino != 0u && dirent_match(de, name, len)) {
                de->d_ino  = 0u;            /* slot free; lengths kept */
                de->d_type = UNFS_DT_UNKNOWN;

                if (!dir_block_write(bm.dev, bm.blkno, blk)) return -1;

                dir->flags |= IFLAG_MODIFIED | IFLAG_CHANGED;
                return 0;
            }

            de = dirent_next(de, end);
            if (!de) break;
        }

        offset += UNFS_BLOCK_SIZE;
    }
    return -1;
}

/* ═════════════════════════════════════════════════════════════
 * Algorithm namei  (§4)
 *
 * Returns a LOCKED inode — the caller must iput() it.
 * ═════════════════════════════════════════════════════════════ */
InCoreInode *namei(const char *path, InCoreInode *cwd,
                   uint16_t uid, uint16_t gid)
{
    InCoreInode *wip;
    uint8_t      dev;                       /* the device the walk is on */
    char         component[UNFS_NAME_MAX + 1u];

    if (!path || path[0] == '\0') return (InCoreInode *)0;

    /* ── the device is captured ONCE, before anything is released ────
     * A directory entry stores only an inode number, so the device has
     * to travel with the walk.  Every iget_dev() below uses this value,
     * including the ones after the working inode is iput().  Reading
     * cwd->dev later would be a use-after-free, and dropping it entirely
     * is what left the previous revision without a device at all. */
    if (path[0] == '/') {
        dev = cwd ? cwd->dev : ROOT_DEV;
        wip = iget_dev(dev, UNFS_ROOT_INO);
        while (*path == '/') path++;
    } else {
        if (!cwd) return (InCoreInode *)0;
        dev = cwd->dev;
        wip = iget_dev(dev, cwd->ino);      /* same filesystem */
        while (*path == '/') path++;        /* tolerate "./" forms */
    }
    if (!wip) return (InCoreInode *)0;

    while (*path) {
        uint32_t found_ino;
        uint32_t clen;

        while (*path == '/') path++;
        if (*path == '\0') break;

        /* Collect one component.  A component longer than UNFS_NAME_MAX
         * cannot name anything on this filesystem, so the walk fails
         * rather than silently truncating to a DIFFERENT file's name —
         * which is what a `len < MAX_NAME_LEN - 1` guard did. */
        clen = 0u;
        while (*path && *path != '/') {
            if (clen >= (uint32_t)UNFS_NAME_MAX) { iput(wip); return (InCoreInode *)0; }
            component[clen++] = *path++;
        }
        component[clen] = '\0';

        /* ── verify the working inode is a searchable directory ────── */
        if ((wip->mode & UNFS_IFMT) != UNFS_IFDIR) {
            iput(wip);
            return (InCoreInode *)0;
        }
        if (!inode_access_ok(wip, uid, gid, 0, 0, 1)) {
            iput(wip);
            return (InCoreInode *)0;
        }

        /* ── Bach's root/".." special case ─────────────────────────── */
        if (clen == 2u && component[0] == '.' && component[1] == '.'
            && wip->ino == UNFS_ROOT_INO) {
            continue;
        }

        /* ── read the directory and match the component ────────────── */
        found_ino = dir_lookup(wip, component, clen);
        iput(wip);                              /* release the working inode */

        if (!found_ino) return (InCoreInode *)0;

        wip = iget_dev(dev, found_ino);         /* ◀ the walk's device */
        if (!wip) return (InCoreInode *)0;
    }

    return wip;     /* locked, as Bach specifies */
}

/* ═════════════════════════════════════════════════════════════
 * fs_mkfs — initialise the root directory.
 *
 * Bach's mkfs: create the root inode, set its link count to 2, and write
 * the "." and ".." entries that make it a directory.
 *
 * The device-aware allocator is ialloc_dev(dev, ...) — the plain
 * ialloc() in superblock.h has no dev parameter.
 * ═════════════════════════════════════════════════════════════ */
int fs_mkfs(uint8_t dev)
{
    InCoreInode *root;

    /* UNFS_IFDIR, not the removed FileType's FT_DIR — the format's type
     * encoding is what lands in i_mode.  (FT_DIR << 12) is 0x2000, which
     * is UNFS_IFCHR, so the old spelling wrote a character device. */
    root = ialloc_dev(dev, (uint16_t)UNFS_IFDIR,
                      PERM_UR | PERM_UW | PERM_UX |
                      PERM_GR | PERM_GX |
                      PERM_OR | PERM_OX,
                      0u, 0u);
    if (!root) return -1;

    /* A directory starts at 2: its own "." and the entry its parent
     * holds.  The root has no parent, so ".." points at itself and the
     * count stays 2. */
    root->nlink  = 2u;
    root->flags |= IFLAG_CHANGED;

    if (dir_add(root, ".",  1u, root->ino, (uint8_t)UNFS_DT_DIR) != 0) {
        iput(root);
        return -1;
    }
    if (dir_add(root, "..", 2u, root->ino, (uint8_t)UNFS_DT_DIR) != 0) {
        iput(root);
        return -1;
    }

    iupdate(root);
    iput(root);
    return 0;
}
