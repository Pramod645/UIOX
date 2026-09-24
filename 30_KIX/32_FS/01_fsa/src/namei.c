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
 *  ── COMPATIBILITY ─────────────────────────────────────────────────────
 *  BELOW (00_buffcache):
 *      every directory read is now  bread(bm.dev, bm.blkno)
 *      every directory write is now bwrite(buf, true, false)
 *      BufEntry -> BufHdr; no `dirty` field exists on the header
 *
 *  ABOVE (10_scfs):
 *      dir_lookup / dir_add / dir_remove / namei / fs_mkfs keep their
 *      signatures — 10_scfs calls them unchanged.
 *
 *  ── the device in namei ───────────────────────────────────────────────
 *  A path walk stays on one filesystem: the working inode's dev travels
 *  with it, and a directory's data blocks are on that same device.  The
 *  walk reads bm.dev, which bmap now fills from the inode.
 *
 *  ── directory entries are FIXED-SIZE ──────────────────────────────────
 *      typedef struct { uint32_t ino; char name[MAX_NAME_LEN]; } DirEntry;
 *
 *  512 / 32 = 16 entries per block.  A name is NEVER NUL-terminated by
 *  construction — it is 28 bytes filled by strncpy — so every comparison
 *  is bounded by MAX_NAME_LEN and every copy uses the field width.
 *
 *  ── the lock protocol Bach relies on ─────────────────────────────────
 *  A function that RETURNS a locked inode (namei) leaves the lock set.
 *  One that COMPLETES a directory lookup (dir_lookup/add/remove) or
 *  takes a working inode for its own use releases it before returning.
 *  Getting this backwards deadlocks the next iget on the same inode.
 *
 *  @version 2.0.0  @date 2026-09-24
 */
#include "namei.h"
#include "bmap.h"
#include "superblock.h"
#include "buffer.h"
#include "uiox_klibc.h"

/* 512 / sizeof(DirEntry) = 16 */
#define DIRENTS_PER_BLOCK   (BLOCK_SIZE / (int)sizeof(DirEntry))

/* ─────────────────────────────────────────────────────────────
 * Internal: compare a stored name against a wanted name.
 *
 * The stored field is MAX_NAME_LEN bytes and may have NO terminator, so
 * strncmp alone is not enough — a 28-byte name would compare equal to a
 * longer wanted name sharing its first 28 bytes.  The stored length is
 * found first, bounded by the field, and the lengths must match.
 * ───────────────────────────────────────────────────────────── */
static int dirname_eq(const char *stored, const char *want)
{
    uint32_t slen = 0u;
    uint32_t wlen = 0u;

    while (slen < (uint32_t)MAX_NAME_LEN && stored[slen] != '\0') slen++;
    while (want[wlen] != '\0') wlen++;

    if (slen != wlen) return 0;
    if (slen == 0u)   return 0;

    for (uint32_t i = 0u; i < slen; i++)
        if (stored[i] != want[i]) return 0;

    return 1;
}

/* ─────────────────────────────────────────────────────────────
 * Internal: copy a name into a directory entry's fixed field.
 * Always leaves the field NUL-terminated within its bound.
 * ───────────────────────────────────────────────────────────── */
static void dirname_store(char *dst, const char *src)
{
    uint32_t i = 0u;

    while (i < (uint32_t)(MAX_NAME_LEN - 1) && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

/* ─────────────────────────────────────────────────────────────
 * dir_lookup — search a directory for a component.
 * Returns the inode number of the match, or 0 if not found.
 *
 * Bach iterates the directory with bmap + bread + brelse; the blkno it
 * reads is on the inode's own device.
 * ───────────────────────────────────────────────────────────── */
uint32_t dir_lookup(InCoreInode *dir, const char *name)
{
    uint32_t offset = 0u;

    if (!dir || !name) return 0u;
    if (inode_type(dir) != FT_DIR) return 0u;

    while (offset < dir->size) {
        BmapResult bm = bmap(dir, offset);
        BufHdr    *buf;
        DirEntry  *entries;
        int        i;

        if (!bm.valid) break;

        buf = bread(bm.dev, bm.blkno);          /* ◀ (dev, blkno) */
        if (!buf) break;

        entries = (DirEntry *)buf->data;

        for (i = 0; i < DIRENTS_PER_BLOCK; i++) {
            if (entries[i].ino == 0u) continue;

            if (dirname_eq(entries[i].name, name)) {
                uint32_t found = entries[i].ino;
                brelse(buf);
                return found;
            }
        }
        brelse(buf);                            /* buffer layer */
        offset += BLOCK_SIZE;                   /* Bach's adjust */
    }
    return 0u;
}

/* ─────────────────────────────────────────────────────────────
 * dir_add — add a (name, ino) entry to a directory.
 * Returns 0 on success, -1 on failure.
 * ───────────────────────────────────────────────────────────── */
int dir_add(InCoreInode *dir, const char *name, uint32_t ino)
{
    uint32_t offset = 0u;

    if (!dir || !name || ino == 0u) return -1;
    if (inode_type(dir) != FT_DIR) return -1;

    /* ── look for a free slot in the blocks that exist ─────────────── */
    while (offset < dir->size) {
        BmapResult bm = bmap(dir, offset);
        BufHdr    *buf;
        DirEntry  *entries;
        int        i;

        if (!bm.valid) break;

        buf = bread(bm.dev, bm.blkno);          /* ◀ (dev, blkno) */
        if (!buf) return -1;

        entries = (DirEntry *)buf->data;

        for (i = 0; i < DIRENTS_PER_BLOCK; i++) {
            if (entries[i].ino == 0u) {
                entries[i].ino = ino;
                dirname_store(entries[i].name, name);

                /* BufHdr has no `dirty` member — the write flags carry
                 * that now, and bwrite releases the buffer. */
                bwrite(buf, true, false);       /* ◀ persist */
                brelse(NULL);

                dir->size += (uint32_t)sizeof(DirEntry);
                dir->flags |= IFLAG_MODIFIED | IFLAG_CHANGED;

                printf("[dir_add] '%s' -> ino=%u in dir ino=%u (dev=%u)\n",
                       name, (unsigned)ino, (unsigned)dir->ino,
                       (unsigned)dir->dev);
                return 0;
            }
        }
        brelse(buf);
        offset += BLOCK_SIZE;
    }

    /* ── no free slot — extend the directory by one block ──────────── */
    {
        BmapResult bm = bmap_alloc(dir, dir->size);
        BufHdr    *buf;
        DirEntry  *entries;

        if (!bm.valid) return -1;

        buf = bread(bm.dev, bm.blkno);          /* ◀ (dev, blkno) */
        if (!buf) return -1;

        entries = (DirEntry *)buf->data;
        entries[0].ino = ino;
        dirname_store(entries[0].name, name);

        bwrite(buf, true, false);               /* ◀ persist + release */

        dir->size += BLOCK_SIZE;
        dir->flags |= IFLAG_MODIFIED | IFLAG_CHANGED;

        printf("[dir_add] '%s' -> ino=%u (new block %u) in dir ino=%u\n",
               name, (unsigned)ino, (unsigned)bm.blkno, (unsigned)dir->ino);
        return 0;
    }
}

/* ─────────────────────────────────────────────────────────────
 * dir_remove — remove the entry with 'name'.
 * Returns 0 on success, -1 if not found.
 *
 * Bach zeroes the INODE NUMBER and leaves the name bytes; that is what
 * makes the slot reusable by dir_add and recognisable as empty by both
 * dir_lookup and getdents64.
 * ───────────────────────────────────────────────────────────── */
int dir_remove(InCoreInode *dir, const char *name)
{
    uint32_t offset = 0u;

    if (!dir || !name) return -1;

    while (offset < dir->size) {
        BmapResult bm = bmap(dir, offset);
        BufHdr    *buf;
        DirEntry  *entries;
        int        i;

        if (!bm.valid) break;

        buf = bread(bm.dev, bm.blkno);          /* ◀ (dev, blkno) */
        if (!buf) return -1;

        entries = (DirEntry *)buf->data;

        for (i = 0; i < DIRENTS_PER_BLOCK; i++) {
            if (entries[i].ino != 0u &&
                dirname_eq(entries[i].name, name)) {

                entries[i].ino     = 0u;
                entries[i].name[0] = '\0';

                bwrite(buf, true, false);       /* ◀ persist + release */

                dir->flags |= IFLAG_MODIFIED;
                printf("[dir_remove] '%s' removed from dir ino=%u\n",
                       name, (unsigned)dir->ino);
                return 0;
            }
        }
        brelse(buf);
        offset += BLOCK_SIZE;
    }
    return -1;
}

/* ═════════════════════════════════════════════════════════════
 * Algorithm namei  (§4)
 *
 * Returns a LOCKED inode — the caller must iput() it.
 * ═════════════════════════════════════════════════════════════════ */
InCoreInode *namei(const char *path, InCoreInode *cwd,
                   uint16_t uid, uint16_t gid)
{
    InCoreInode *wip;
    char         component[MAX_NAME_LEN];

    if (!path || path[0] == '\0') return (InCoreInode *)0;

    /* ── where the walk starts ─────────────────────────────────────── *
     * An absolute path starts at the root; a relative one at the cwd.
     * A cwd carries its own device, so a relative walk under a mounted
     * filesystem stays on it. */
    if (path[0] == '/') {
        wip = iget(ROOT_INO);
        while (*path == '/') path++;
    } else {
        if (!cwd) { printf("[namei] ERROR: no cwd\n"); return (InCoreInode *)0; }
        wip = iget_dev(cwd->dev, cwd->ino);     /* ◀ same filesystem */
    }
    if (!wip) return (InCoreInode *)0;

    while (*path) {
        uint32_t found_ino;
        int      len = 0;

        while (*path == '/') path++;
        if (*path == '\0') break;

        while (*path && *path != '/' && len < MAX_NAME_LEN - 1)
            component[len++] = *path++;
        component[len] = '\0';

        /* ── verify the working inode is a searchable directory ────── */
        if (inode_type(wip) != FT_DIR) {
            printf("[namei] ERROR: '%s' not a directory\n", component);
            iput(wip);
            return (InCoreInode *)0;
        }
        if (!inode_access_ok(wip, uid, gid, 0, 0, 1)) {
            printf("[namei] ERROR: no execute perm on dir ino=%u\n",
                   (unsigned)wip->ino);
            iput(wip);
            return (InCoreInode *)0;
        }

        /* ── Bach's root/".." special case ─────────────────────────── */
        if (component[0] == '.' && component[1] == '.' && component[2] == '\0'
            && wip->ino == ROOT_INO) {
            printf("[namei] '..' at root - staying\n");
            continue;
        }

        /* ── read the directory and match the component ────────────── */
        found_ino = dir_lookup(wip, component);
        iput(wip);                              /* release the working inode */

        if (!found_ino) {
            printf("[namei] '%s' not found\n", component);
            return (InCoreInode *)0;
        }

        wip = iget(found_ino);
        if (!wip) return (InCoreInode *)0;

        printf("[namei] component '%s' -> ino=%u\n",
               component, (unsigned)found_ino);
    }

    return wip;     /* locked, as Bach specifies */
}

/* ═════════════════════════════════════════════════════════════
 * fs_mkfs — initialise the root directory.
 *
 * Bach's mkfs: create the root inode, set its link count to 2, and write
 * the "." and ".." entries that make it a directory.
 * ═════════════════════════════════════════════════════════════════ */
void fs_mkfs(void)
{
    InCoreInode *root;

    printf("[mkfs] creating filesystem\n");

    root = ialloc(FT_DIR,
                  PERM_UR | PERM_UW | PERM_UX |
                  PERM_GR | PERM_GX |
                  PERM_OR | PERM_OX,
                  0u, 0u);
    if (!root) {
        printf("[mkfs] ERROR: cannot alloc root inode\n");
        return;
    }

    /* A directory starts at 2: its own "." and the entry its parent
     * holds.  The root has no parent, so ".." points at itself and the
     * count stays 2. */
    root->nlink = 2u;
    root->flags |= IFLAG_CHANGED;

    dir_add(root, ".",  root->ino);
    dir_add(root, "..", root->ino);

    iupdate(root);
    iput(root);

    printf("[mkfs] root ino=%u (dev=%u) created\n",
           (unsigned)ROOT_INO, (unsigned)root->dev);
}
