/*
 *  30_KIX/32_FS/01_fsa/src/inode.c
 *
 *  Algorithms iget and iput, plus the inode cache and iupdate.
 *  Bach, The Design of the UNIX Operating System, Ch.4 §1, §2.
 *
 *  ── COMPATIBILITY ─────────────────────────────────────────────────────
 *  BELOW (00_buffcache / 31_BufferCache):
 *      inode_disk_read now calls  bread(dev, blkno)
 *      iupdate now calls          bwrite(buf, true, false)
 *  Both were one-argument calls in the first cut and would not link
 *  against the buffer layer's real contract.
 *
 *  ABOVE (10_scfs):
 *      iget_dev / iget / iput / iupdate / inode_disk_read /
 *      inode_access_ok are the names 10_scfs declares in
 *      uiox_kix_scfs_internal.h.  Signatures are unchanged except that
 *      inode_disk_read grew its leading dev.
 *
 *  ── Bach's Algorithm iget, verbatim ──────────────────────────────────
 *  input: file system inode number
 *  output: locked inode
 *  {
 *      while (not done)
 *      {
 *          if (inode in inode cache)
 *          {
 *              if (inode locked) { sleep; continue; }
 *              if (inode on inode free list) remove from free list;
 *              increase inode reference count;
 *              return (inode);
 *          }
 *          if (no inodes on free list) return (error);
 *          remove new inode from free list;
 *          reset inode number and file system;
 *          remove inode from old hash queue, place on new one;
 *          read inode from disk (algorithm bread);
 *          initialize inode (ex. references count to 1);
 *          return (inode);
 *      }
 *  }
 *
 *  ── Bach's Algorithm iput, verbatim ──────────────────────────────────
 *  input: pointer to in-core inode
 *  output: none
 *  {
 *      lock inode if not already locked;
 *      decrement inode reference count;
 *      if (reference count == 0)
 *      {
 *          if (inode link count == 0)
 *          {
 *              free disk blocks for file (algorithm free);
 *              set file type to 0;
 *              free inode (algorithm ifree);
 *          }
 *          if (file accessed or inode changed or file changed)
 *              update disk inode;
 *          put inode on free list;
 *      }
 *      release inode lock;
 *  }
 *
 *  @version 2.0.0  @date 2026-09-24
 */
#include "inode.h"
#include "superblock.h"
#include "uiox_klibc.h"

/* ─────────────────────────────────────────────────────────────
 * In-core inode cache and free/hash lists
 * ───────────────────────────────────────────────────────────── */
static InCoreInode  icache[MAX_INCACHE];
static InCoreInode *ifree_head = (InCoreInode *)0;
static InCoreInode *ifree_tail = (InCoreInode *)0;

#define IHASH_SIZE MAX_INCACHE
static InCoreInode *ihash[IHASH_SIZE];

/* ─────────────────────────────────────────────────────────────
 * Internal helpers
 * ───────────────────────────────────────────────────────────── */
static int ihash_slot(uint8_t dev, uint32_t ino)
{
    /* The device is folded in: two filesystems can hold the same inode
     * number, and they must not collide in one hash queue. */
    return (int)((((uint32_t)dev << 24) ^ ino) % IHASH_SIZE);
}

static void ifree_remove(InCoreInode *ip)
{
    if (ip->free_prev) ip->free_prev->free_next = ip->free_next;
    else               ifree_head               = ip->free_next;
    if (ip->free_next) ip->free_next->free_prev = ip->free_prev;
    else               ifree_tail               = ip->free_prev;
    ip->free_prev = ip->free_next = (InCoreInode *)0;
    ip->on_free_list = false;
}

static void ifree_append(InCoreInode *ip)
{
    ip->free_prev = ifree_tail;
    ip->free_next = (InCoreInode *)0;
    if (ifree_tail) ifree_tail->free_next = ip;
    else            ifree_head            = ip;
    ifree_tail       = ip;
    ip->on_free_list = true;
}

static void ihash_insert(InCoreInode *ip)
{
    int s         = ihash_slot(ip->dev, ip->ino);
    ip->hash_next = ihash[s];
    ihash[s]      = ip;
}

static void ihash_remove(InCoreInode *ip)
{
    int           s  = ihash_slot(ip->dev, ip->ino);
    InCoreInode **pp = &ihash[s];

    while (*pp) {
        if (*pp == ip) { *pp = ip->hash_next; ip->hash_next = (InCoreInode *)0; return; }
        pp = &(*pp)->hash_next;
    }
}

static InCoreInode *ihash_lookup(uint8_t dev, uint32_t ino)
{
    InCoreInode *ip = ihash[ihash_slot(dev, ino)];

    while (ip) {
        if (ip->ino == ino && ip->dev == dev) return ip;
        ip = ip->hash_next;
    }
    return (InCoreInode *)0;
}

/* ─────────────────────────────────────────────────────────────
 * inode_cache_init
 * ───────────────────────────────────────────────────────────── */
void inode_cache_init(void)
{
    uint32_t i;

    memset(icache, 0, sizeof icache);
    memset(ihash,  0, sizeof ihash);
    ifree_head = ifree_tail = (InCoreInode *)0;

    for (i = 0u; i < MAX_INCACHE; i++)
        ifree_append(&icache[i]);

    printf("[inode] cache init: %u slots\n", (unsigned)MAX_INCACHE);
}

/* ─────────────────────────────────────────────────────────────
 * inode_disk_read
 *
 * Reads the DiskInode for (dev, ino) through the buffer cache.
 * Caller must brelse(*out_buf) when done with the data.
 *
 * The inode-list block itself is on the same device as the inode, so
 * the dev argument serves both the bread call and the geometry.
 * ───────────────────────────────────────────────────────────── */
DiskInode *inode_disk_read(uint8_t dev, uint32_t ino, BufHdr **out_buf)
{
    uint32_t  blkno;
    uint32_t  offset;
    BufHdr   *buf;

    /* MAX_INODES and INODE_START_BLOCK are gone: fs_types.h no longer
     * defines them, and unfs_format.h is the format's owner.
     *
     * The bound is what GROUP 0's inode table physically holds —
     * UNFS_ITABLE_BLOCKS (8) x UNFS_INODES_PER_BLOCK (16) = 128 inodes.
     *
     * Deliberately NOT unfs_sb_t.s_inode_count, which is the volume's
     * real total.  This is the lowest inode-layer primitive and it is
     * called FROM the superblock path (sb_refill_inodes -> this), so
     * reaching for sb_get(dev) here would be a circular dependency at
     * mount time — the bound has to be computable before any superblock
     * is loaded.  A later revision can add a per-device limit once the
     * mount path can supply one. */
    if (ino == 0u || ino > (UNFS_ITABLE_BLOCKS * UNFS_INODES_PER_BLOCK))
        return (DiskInode *)0;

    blkno  = ((ino - 1u) / UNFS_INODES_PER_BLOCK) + UNFS_GROUP0_ITABLE;
    offset = ((ino - 1u) % UNFS_INODES_PER_BLOCK) * (uint32_t)sizeof(DiskInode);

    buf = bread(dev, blkno);                /* ◀ (dev, blkno) */
    if (!buf) return (DiskInode *)0;

    *out_buf = buf;
    return (DiskInode *)(buf->data + offset);
}

/* ─────────────────────────────────────────────────────────────
 * iupdate — write in-core inode back to its disk block
 * ───────────────────────────────────────────────────────────── */
void iupdate(InCoreInode *ip)
{
    BufHdr    *buf;
    DiskInode *di;

    if (!ip) return;

    di = inode_disk_read(ip->dev, ip->ino, &buf);
    if (!di) return;

    /* DiskInode carries the FORMAT's field names now — i_* — while the
     * in-core struct still uses the short ones.  The mapping is written
     * out rather than renamed away, so the difference stays visible. */
    di->i_mode  = ip->mode;
    di->i_nlink = ip->nlink;
    di->i_uid   = ip->uid;
    di->i_gid   = ip->gid;
    di->i_size  = ip->size;
    /* The block map is an extent array on disk, not addr[]: four inline
     * extents plus the overflow tree pointer.  A plain copy, because the
     * in-core mirror uses the same field names. */
    memcpy(di->i_extents, ip->i_extents, sizeof ip->i_extents);
    di->i_extent_tree = ip->i_extent_tree;
    di->i_atime_ns = (uint64_t)ip->atime;
    di->i_mtime_ns = (uint64_t)ip->mtime;
    di->i_ctime_ns = (uint64_t)ip->ctime;
    di->dev        = ip->dev;               /* ◀ in-core only */

    /* Mark the buffer dirty, then write it synchronously.  The buffer
     * layer expresses dirtiness by the bwrite flags, not by a field on
     * the header — BufHdr has no `dirty` member. */
    bwrite(buf, true, false);               /* ◀ write now + release  */

    ip->flags &= (uint8_t)~(IFLAG_ACCESSED | IFLAG_CHANGED | IFLAG_MODIFIED);
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm iget  (§1) — the general, multi-device form
 * ───────────────────────────────────────────────────────────── */
InCoreInode *iget_dev(uint8_t dev, uint32_t ino)
{
    InCoreInode *ip;
    BufHdr      *buf;
    DiskInode   *di;

    /* Same bound as inode_disk_read — group 0's inode table — for the
     * same reason, and so the two cannot disagree about what is valid. */
    if (ino == 0u || ino > (UNFS_ITABLE_BLOCKS * UNFS_INODES_PER_BLOCK)) {
        printf("[iget] ERROR: invalid ino=%u\n", (unsigned)ino);
        return (InCoreInode *)0;
    }

    while (1) {
        /* ── (1) the inode is in the cache ─────────────────────────── */
        ip = ihash_lookup(dev, ino);
        if (ip) {
            if (ip->locked) {
                printf("[iget] dev=%u ino=%u locked - waiting\n",
                       (unsigned)dev, (unsigned)ino);
                continue;               /* Bach: sleep; we spin */
            }
            if (ip->on_free_list) ifree_remove(ip);
            ip->refcount++;
            ip->locked = true;
            printf("[iget] cache hit: dev=%u ino=%u ref=%d\n",
                   (unsigned)dev, (unsigned)ino, ip->refcount);
            return ip;
        }

        /* ── (2) not in cache — take a slot from the free list ─────── */
        if (!ifree_head) {
            printf("[iget] ERROR: inode cache full\n");
            return (InCoreInode *)0;
        }

        ip = ifree_head;
        ifree_remove(ip);

        /* remove from its OLD hash queue before reassigning the identity */
        if (ip->ino != 0u) ihash_remove(ip);

        ip->ino = ino;
        ip->dev = dev;                      /* ◀ reset the filesystem */
        ihash_insert(ip);

        /* ── read the inode from disk (algorithm bread) ────────────── */
        di = inode_disk_read(dev, ino, &buf);
        if (!di) {
            ifree_append(ip);
            return (InCoreInode *)0;
        }

        ip->mode   = di->i_mode;
        ip->nlink  = di->i_nlink;
        ip->uid    = di->i_uid;
        ip->gid    = di->i_gid;
        ip->size   = di->i_size;
        memcpy(ip->i_extents, di->i_extents, sizeof ip->i_extents);
        ip->i_extent_tree = di->i_extent_tree;
        ip->atime  = (time_t)di->i_atime_ns;
        ip->mtime  = (time_t)di->i_mtime_ns;
        ip->ctime  = (time_t)di->i_ctime_ns;
        ip->dev    = di->dev;
        ip->refcount = 1;
        ip->locked   = true;
        ip->flags    = 0u;

        brelse(buf);                        /* buffer layer */

        printf("[iget] disk read: dev=%u ino=%u size=%u nlink=%u\n",
               (unsigned)dev, (unsigned)ino, ip->size, ip->nlink);
        return ip;
    }
}

/* iget — the single-filesystem convenience.  10_scfs's older callers use
 * this form; the device is ROOT_DEV (0). */
InCoreInode *iget(uint32_t ino)
{
    return iget_dev(ROOT_DEV, ino);
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm iput  (§2)
 *
 * ── the ORDER inside the refcount==0 branch is load-bearing ──────────
 * The first cut freed the blocks and the inode, THEN tested the dirty
 * flags and called iupdate — which re-reads the disk inode it had just
 * released, writing a freed inode's block back through the cache.  The
 * write is now done FIRST, while the inode is still live.
 * ───────────────────────────────────────────────────────────── */
void iput(InCoreInode *ip)
{
    if (!ip) return;

    if (ip->refcount > 0) ip->refcount--;

    if (ip->refcount == 0) {

        /* ── (a) persist the inode while it still exists ───────────── */
        if (ip->flags & (IFLAG_ACCESSED | IFLAG_CHANGED | IFLAG_MODIFIED))
            iupdate(ip);

        /* ── (b) link count 0: free the blocks, then the inode ─────── */
        if (ip->nlink == 0) {
            fs_free_inode_blocks(ip);       /* algorithm free  (§6)    */
            ip->mode = 0u;                  /* set file type to 0      */
            ifree(ip->ino);                 /* algorithm ifree (§8)    */
            printf("[iput] dev=%u ino=%u: link==0, blocks+inode freed\n",
                   (unsigned)ip->dev, (unsigned)ip->ino);
        }

        /* ── (c) back on the free list ─────────────────────────────── */
        if (!ip->on_free_list) ifree_append(ip);
    }

    ip->locked = false;
}

/* ─────────────────────────────────────────────────────────────
 * inode_access_ok — permission test
 *
 * No super-user bypass.  See the note in inode.h: if one is wanted it
 * goes here, so every caller gets the same answer.
 * ───────────────────────────────────────────────────────────── */
bool inode_access_ok(const InCoreInode *ip, uint16_t uid, uint16_t gid,
                     int want_read, int want_write, int want_exec)
{
    uint16_t perm;
    uint16_t bits;

    if (!ip) return false;

    perm = inode_perm(ip);

    if (uid == ip->uid)      bits = (uint16_t)((perm >> 6) & 7u);
    else if (gid == ip->gid) bits = (uint16_t)((perm >> 3) & 7u);
    else                     bits = (uint16_t)(perm & 7u);

    if (want_read  && !(bits & 4u)) return false;
    if (want_write && !(bits & 2u)) return false;
    if (want_exec  && !(bits & 1u)) return false;
    return true;
}
