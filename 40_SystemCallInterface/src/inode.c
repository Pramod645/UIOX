#include "../include/inode.h"
#include "../include/buf.h"
#include "../include/fs.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

inode_t inode_table[NINODE];

/* ── iget ─────────────────────────────────────────────────────────
 * Locate inode (dev, inum) in the in-memory table.
 * If not present, read it from disk into a free slot.
 */
inode_t *iget(uint16_t dev, uint32_t inum)
{
    inode_t *ip     = NULL;
    inode_t *free_ip = NULL;

    /* Search inode table */
    for (int i = 0; i < NINODE; i++) {
        if (inode_table[i].i_dev == dev &&
            inode_table[i].i_number == inum &&
            inode_table[i].i_count > 0) {

            /* Found — wait if locked */
            while (inode_table[i].i_flag & ILOCK) {
                inode_table[i].i_flag |= IWANT;
                /* sleep(inode_table + i); */   /* real kernel: sleep */
            }
            inode_table[i].i_count++;
            ilock(&inode_table[i]);
            return &inode_table[i];
        }
        if (inode_table[i].i_count == 0 && free_ip == NULL)
            free_ip = &inode_table[i];
    }

    if (free_ip == NULL) {
        fprintf(stderr, "iget: inode table overflow\n");
        return NULL;
    }

    ip = free_ip;
    ip->i_dev    = dev;
    ip->i_number = inum;
    ip->i_count  = 1;
    ip->i_flag   = 0;

    /* Read inode from disk (simulated) */
    buf_t *bp = bread(dev, inum / 8 + 2);   /* inode block */
    if (!bp) {
        ip->i_count = 0;
        return NULL;
    }
    /* In a real kernel: copy disk inode fields from bp->b_data */
    brelse(bp);

    ilock(ip);
    return ip;
}

/* ── iput ─────────────────────────────────────────────────────────
 * Release reference to inode. If count drops to 0 and link
 * count is 0, free all blocks and the inode itself.
 */
void iput(inode_t *ip)
{
    if (!ip) return;

    ilock(ip);
    ip->i_count--;

    if (ip->i_count == 0) {
        if (ip->i_nlink == 0) {
            itrunc(ip);             /* free all file blocks */
            ip->i_mode = 0;
            ifree(ip->i_dev, ip->i_number);
        }
        if (ip->i_flag & (IUPD | IACC))
            iupdate(ip);
    }
    iunlock(ip);
}

/* ── ilock / iunlock ──────────────────────────────────────────── */
void ilock(inode_t *ip)
{
    while (ip->i_flag & ILOCK) {
        ip->i_flag |= IWANT;
        /* sleep(ip); */   /* real kernel sleeps here */
    }
    ip->i_flag |= ILOCK;
}

void iunlock(inode_t *ip)
{
    ip->i_flag &= ~ILOCK;
    if (ip->i_flag & IWANT) {
        ip->i_flag &= ~IWANT;
        /* wakeup(ip); */
    }
}

/* ── ialloc ───────────────────────────────────────────────────────
 * Allocate a free inode on device dev.
 */
inode_t *ialloc(uint16_t dev)
{
    /* In a real kernel: search super block free inode list,
     * scan inode list on disk if super block list empty. */
    for (int i = 0; i < NINODE; i++) {
        if (inode_table[i].i_count == 0 &&
            inode_table[i].i_number == 0) {
            inode_table[i].i_dev    = dev;
            inode_table[i].i_number = i + 1;
            inode_table[i].i_count  = 1;
            inode_table[i].i_nlink  = 0;
            inode_table[i].i_size   = 0;
            inode_table[i].i_flag   = IUPD;
            memset(inode_table[i].i_addr, 0,
                   sizeof(inode_table[i].i_addr));
            ilock(&inode_table[i]);
            return &inode_table[i];
        }
    }
    fprintf(stderr, "ialloc: no free inodes\n");
    return NULL;
}

/* ── ifree ────────────────────────────────────────────────────────
 * Return inode inum on device dev to the free list.
 */
void ifree(uint16_t dev, uint32_t inum)
{
    for (int i = 0; i < NINODE; i++) {
        if (inode_table[i].i_dev == dev &&
            inode_table[i].i_number == inum) {
            memset(&inode_table[i], 0, sizeof(inode_t));
            return;
        }
    }
}

/* ── iupdate ──────────────────────────────────────────────────────
 * Write updated inode back to disk.
 */
int iupdate(inode_t *ip)
{
    buf_t *bp = getblk(ip->i_dev, ip->i_number / 8 + 2);
    if (!bp) return -1;
    /* Copy inode fields into bp->b_data (simulated) */
    ip->i_flag &= ~(IUPD | IACC);
    bwrite(bp);
    return 0;
}

/* ── itrunc ───────────────────────────────────────────────────────
 * Free all data blocks of an inode (algorithm free).
 */
int itrunc(inode_t *ip)
{
    for (int i = 0; i < NBLOCK_DIRECT + NBLOCK_INDIRECT; i++) {
        if (ip->i_addr[i]) {
            bfree(ip->i_dev, ip->i_addr[i]);
            ip->i_addr[i] = 0;
        }
    }
    ip->i_size  = 0;
    ip->i_flag |= IUPD;
    return 0;
}

/* ── iaccess ──────────────────────────────────────────────────────
 * Check whether current process has the requested access
 * (mode: R=4, W=2, X=1) to inode ip.
 */
int iaccess(inode_t *ip, int mode)
{
    extern u_area_t u;
    uint16_t perm = ip->i_mode & 0777;

    if (u.u_uid == 0)           /* root can do anything */
        return 1;
    if (u.u_uid == ip->i_uid)
        perm >>= 6;
    else if (u.u_gid == ip->i_gid)
        perm >>= 3;

    return (perm & mode) == (uint16_t)mode;
}

/* ── namei ────────────────────────────────────────────────────────
 * Parse a pathname and return the locked inode.
 * Handles absolute ('/') and relative paths.
 */
inode_t *namei(const char *path)
{
    extern u_area_t u;
    if (!path || *path == '\0') return NULL;

    inode_t *dp = (*path == '/') ? u.u_rdir : u.u_cdir;
    if (!dp) return NULL;

    dp->i_count++;   /* simulate iget on start dir */

    char buf[256];
    strncpy(buf, path, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *token = strtok(buf, "/");
    while (token) {
        /* dp must be a directory */
        if ((dp->i_mode & IFMT) != IFDIR) {
            iput(dp);
            return NULL;
        }
        if (!iaccess(dp, 1)) {   /* execute permission = search */
            iput(dp);
            return NULL;
        }

        /* Handle mount points: redirect to mounted fs root */
        if (dp->i_flag & IMOUNT) {
            for (int i = 0; i < NMOUNT; i++) {
                extern mount_t mount_table[];
                if (mount_table[i].m_inodp == dp) {
                    iput(dp);
                    dp = mount_table[i].m_mount_root;
                    dp->i_count++;
                    break;
                }
            }
        }

        /* Scan directory for matching entry (simulated) */
        /* In real kernel: bread directory blocks and compare dirent */
        inode_t *next = NULL;   /* placeholder for found inode */
        (void)token;            /* suppress unused-variable warning */

        if (!next) {
            iput(dp);
            return NULL;
        }
        iput(dp);
        dp    = next;
        token = strtok(NULL, "/");
    }
    return dp;
}
