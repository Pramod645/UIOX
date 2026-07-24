#include "inode.h"
#include "superblock.h"
#include "/Users/pramodkumar/Hack/WS/UIOX/30_KIX/33_PCS/include/uiox_klibc.h"

/* ─────────────────────────────────────────────────────────────
 * In-core inode cache and free/hash lists
 * ───────────────────────────────────────────────────────────── */
static InCoreInode  icache[MAX_INCACHE];
static InCoreInode *ifree_head = NULL;
static InCoreInode *ifree_tail = NULL;
#define IHASH_SIZE MAX_INCACHE
static InCoreInode *ihash[IHASH_SIZE];

/* ─────────────────────────────────────────────────────────────
 * Internal helpers
 * ───────────────────────────────────────────────────────────── */
static int ihash_slot(uint32_t ino) { return (int)(ino % IHASH_SIZE); }

static void ifree_remove(InCoreInode *ip)
{
    if (ip->free_prev) ip->free_prev->free_next = ip->free_next;
    else               ifree_head               = ip->free_next;
    if (ip->free_next) ip->free_next->free_prev = ip->free_prev;
    else               ifree_tail               = ip->free_prev;
    ip->free_prev = ip->free_next = NULL;
    ip->on_free_list = false;
}

static void ifree_append(InCoreInode *ip)
{
    ip->free_prev = ifree_tail;
    ip->free_next = NULL;
    if (ifree_tail) ifree_tail->free_next = ip;
    else            ifree_head            = ip;
    ifree_tail       = ip;
    ip->on_free_list = true;
}

static void ihash_insert(InCoreInode *ip)
{
    int s          = ihash_slot(ip->ino);
    ip->hash_next  = ihash[s];
    ihash[s]       = ip;
}

static void ihash_remove(InCoreInode *ip)
{
    int          s    = ihash_slot(ip->ino);
    InCoreInode *prev = NULL, *cur = ihash[s];
    while (cur) {
        if (cur == ip) {
            if (prev) prev->hash_next = cur->hash_next;
            else      ihash[s]        = cur->hash_next;
            cur->hash_next = NULL;
            return;
        }
        prev = cur; cur = cur->hash_next;
    }
}

static InCoreInode *ihash_lookup(uint32_t ino)
{
    InCoreInode *ip = ihash[ihash_slot(ino)];
    while (ip) {
        if (ip->ino == ino) return ip;
        ip = ip->hash_next;
    }
    return NULL;
}

/* ─────────────────────────────────────────────────────────────
 * inode_cache_init
 * ───────────────────────────────────────────────────────────── */
void inode_cache_init(void)
{
    memset(icache, 0, sizeof icache);
    memset(ihash,  0, sizeof ihash);
    ifree_head = ifree_tail = NULL;

    for (int i = 0; i < MAX_INCACHE; i++)
        ifree_append(&icache[i]);

    printf("[inode] cache init: %d slots\n", MAX_INCACHE);
}

/* ─────────────────────────────────────────────────────────────
 * inode_disk_read
 * Reads the DiskInode for 'ino' from the buffer cache.
 * Caller must call brelse(*out_buf) when done with the data.
 * ───────────────────────────────────────────────────────────── */
DiskInode *inode_disk_read(uint32_t ino, BufEntry **out_buf)
{
    if (ino == 0 || ino > MAX_INODES) return NULL;

    uint32_t  blkno  = ((ino - 1) / INODES_PER_BLOCK) + INODE_START_BLOCK;
    uint32_t  offset = ((ino - 1) % INODES_PER_BLOCK) * sizeof(DiskInode);

    BufEntry *buf = bread(blkno);
    if (!buf) return NULL;
    *out_buf = buf;
    return (DiskInode *)(buf->data + offset);
}

/* ─────────────────────────────────────────────────────────────
 * iupdate — write in-core inode back to its disk block
 * ───────────────────────────────────────────────────────────── */
void iupdate(InCoreInode *ip)
{
    BufEntry *buf;
    DiskInode *di = inode_disk_read(ip->ino, &buf);
    if (!di) return;

    di->mode  = ip->mode;
    di->nlink = ip->nlink;
    di->uid   = ip->uid;
    di->gid   = ip->gid;
    di->size  = ip->size;
    memcpy(di->addr, ip->addr, sizeof ip->addr);
    di->atime = ip->atime;
    di->mtime = ip->mtime;
    di->ctime = ip->ctime;

    buf->dirty = true;
    bwrite(buf);
    brelse(buf);

    ip->flags &= (uint8_t)~(IFLAG_ACCESSED | IFLAG_CHANGED | IFLAG_MODIFIED);
    printf("[inode] iupdate: ino=%u written to disk\n", ip->ino);
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm iget  (§1)
 * ───────────────────────────────────────────────────────────── */
InCoreInode *iget(uint32_t ino)
{
    if (ino == 0 || ino > MAX_INODES) {
        fprintf(stderr, "[iget] invalid ino=%u\n", ino);
        return NULL;
    }

    while (1) {
        /* ── Check inode cache ─────────────────────────────── */
        InCoreInode *ip = ihash_lookup(ino);
        if (ip) {
            if (ip->locked) {
                /* Sleep until unlocked (simulated: just warn) */
                printf("[iget] ino=%u locked — waiting\n", ino);
                /* In a real kernel: sleep(event inode unlocked); continue */
            }
            /* Special mount-point processing would go here */
            if (ip->on_free_list) ifree_remove(ip);
            ip->refcount++;
            ip->locked = true;
            printf("[iget] cache hit: ino=%u  ref=%d\n",
                   ino, ip->refcount);
            return ip;
        }

        /* ── Not in cache — need a free slot ─────────────── */
        if (!ifree_head) {
            fprintf(stderr, "[iget] inode cache full\n");
            return NULL;
        }

        ip = ifree_head;
        ifree_remove(ip);

        /* Remove from old hash chain; insert under new ino */
        if (ip->ino != 0) ihash_remove(ip);
        ip->ino = ino;
        ihash_insert(ip);

        /* Read DiskInode from disk */
        BufEntry  *buf;
        DiskInode *di = inode_disk_read(ino, &buf);
        if (!di) {
            ifree_append(ip);
            return NULL;
        }

        /* Initialise in-core inode from disk data */
        ip->mode      = di->mode;
        ip->nlink     = di->nlink;
        ip->uid       = di->uid;
        ip->gid       = di->gid;
        ip->size      = di->size;
        memcpy(ip->addr, di->addr, sizeof ip->addr);
        ip->atime     = di->atime;
        ip->mtime     = di->mtime;
        ip->ctime     = di->ctime;
        ip->refcount  = 1;
        ip->locked    = true;
        ip->flags     = 0;

        brelse(buf);
        printf("[iget] disk read: ino=%u  size=%u  nlink=%u\n",
               ino, ip->size, ip->nlink);
        return ip;
    }
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm iput  (§2)
 * ───────────────────────────────────────────────────────────── */
void iput(InCoreInode *ip)
{
    if (!ip) return;

    if (!ip->locked) ip->locked = true; /* lock if not already */

    ip->refcount--;
    printf("[iput] ino=%u  ref→%d\n", ip->ino, ip->refcount);

    if (ip->refcount == 0) {

        if (ip->nlink == 0) {
            /* File has been unlinked — free all disk blocks */
            extern void fs_free_inode_blocks(InCoreInode *ip);
            fs_free_inode_blocks(ip);
            ip->mode = 0;
            /* Free inode on disk */
            extern void ifree(uint32_t ino);
            ifree(ip->ino);
            printf("[iput] ino=%u: link==0, blocks+inode freed\n", ip->ino);
        }

        /* Write back if dirty */
        if (ip->flags & (IFLAG_ACCESSED | IFLAG_CHANGED | IFLAG_MODIFIED))
            iupdate(ip);

        /* Return to free list */
        ifree_append(ip);
    }

    ip->locked = false;
}

/* ─────────────────────────────────────────────────────────────
 * inode_access_ok  — simplified permission check
 * ───────────────────────────────────────────────────────────── */
bool inode_access_ok(const InCoreInode *ip, uint16_t uid, uint16_t gid,
                     int want_read, int want_write, int want_exec)
{
    uint16_t perm = inode_perm(ip);
    uint16_t bits;

    if (uid == ip->uid)      bits = (uint16_t)((perm >> 6) & 7);
    else if (gid == ip->gid) bits = (uint16_t)((perm >> 3) & 7);
    else                     bits = (uint16_t)(perm & 7);

    if (want_read  && !(bits & 4)) return false;
    if (want_write && !(bits & 2)) return false;
    if (want_exec  && !(bits & 1)) return false;
    return true;
}
