/*
 *  30_KIX/32_FS/10_scfs/src/inode.c
 *
 *  Freestanding fixes (v1.1):
 *    FIXED: #include "../../33_PCS/..."  →  #include "uiox_klibc.h"
 *    FIXED: fprintf(stderr, ...)         →  printf(...)  (2 occurrences)
 *    FIXED: strtok()                     →  manual token walker (freestanding,
 *                                           no hidden static state)
 *    FIXED: for (int i = ...)            →  int i; before loop (strict C11)
 */
#include "../include/inode.h"
#include "../include/buf.h"
#include "../include/fs.h"
#include "uiox_klibc.h"

inode_t inode_table[NINODE];

/* ── iget ──────────────────────────────────────────────────────
 * Locate inode (dev, inum) in the in-memory table.
 * If not present, read it from disk into a free slot.
 */
inode_t *iget(uint16_t dev, uint32_t inum)
{
    int      i;
    inode_t *ip      = NULL;
    inode_t *free_ip = NULL;

    for (i = 0; i < NINODE; i++) {
        if (inode_table[i].i_dev    == dev  &&
            inode_table[i].i_number == inum &&
            inode_table[i].i_count  > 0) {

            while (inode_table[i].i_flag & ILOCK) {
                inode_table[i].i_flag |= IWANT;
                /* sleep(inode_table + i); */
            }
            inode_table[i].i_count++;
            ilock(&inode_table[i]);
            return &inode_table[i];
        }
        if (inode_table[i].i_count == 0 && free_ip == NULL)
            free_ip = &inode_table[i];
    }

    if (free_ip == NULL) {
        printf("[iget] ERROR: inode table overflow\n");
        return NULL;
    }

    ip           = free_ip;
    ip->i_dev    = dev;
    ip->i_number = inum;
    ip->i_count  = 1;
    ip->i_flag   = 0;

    /* Read inode from disk (simulated) */
    {
        buf_t *bp = bread(dev, inum / 8 + 2);
        if (!bp) { ip->i_count = 0; return NULL; }
        brelse(bp);
    }

    ilock(ip);
    return ip;
}

/* ── iput ──────────────────────────────────────────────────────
 * Release reference to inode.
 */
void iput(inode_t *ip)
{
    if (!ip) return;

    ilock(ip);
    ip->i_count--;

    if (ip->i_count == 0) {
        if (ip->i_nlink == 0) {
            itrunc(ip);
            ip->i_mode = 0;
            ifree(ip->i_dev, ip->i_number);
        }
        if (ip->i_flag & (IUPD | IACC))
            iupdate(ip);
    }
    iunlock(ip);
}

/* ── ilock / iunlock ───────────────────────────────────────── */
void ilock(inode_t *ip)
{
    while (ip->i_flag & ILOCK) {
        ip->i_flag |= IWANT;
        /* sleep(ip); */
    }
    ip->i_flag |= ILOCK;
}

void iunlock(inode_t *ip)
{
    ip->i_flag &= (uint16_t)~ILOCK;
    if (ip->i_flag & IWANT) {
        ip->i_flag &= (uint16_t)~IWANT;
        /* wakeup(ip); */
    }
}

/* ── ialloc ────────────────────────────────────────────────────
 * Allocate a free inode on device dev.
 */
inode_t *ialloc(uint16_t dev)
{
    int i;
    for (i = 0; i < NINODE; i++) {
        if (inode_table[i].i_count  == 0 &&
            inode_table[i].i_number == 0) {
            inode_table[i].i_dev    = dev;
            inode_table[i].i_number = (uint32_t)(i + 1);
            inode_table[i].i_count  = 1;
            inode_table[i].i_nlink  = 0;
            inode_table[i].i_size   = 0;
            memset(inode_table[i].i_addr, 0,
                   sizeof inode_table[i].i_addr);
            ilock(&inode_table[i]);
            return &inode_table[i];
        }
    }
    printf("[ialloc] ERROR: no free inodes\n");
    return NULL;
}

/* ── ifree ─────────────────────────────────────────────────────
 * Return inode to the free pool.
 */
void ifree(uint16_t dev, uint32_t inum)
{
    int i;
    for (i = 0; i < NINODE; i++) {
        if (inode_table[i].i_dev    == dev &&
            inode_table[i].i_number == inum) {
            memset(&inode_table[i], 0, sizeof(inode_t));
            return;
        }
    }
}

/* ── iupdate ───────────────────────────────────────────────────
 * Write inode back to disk (simulated: just clear dirty flags).
 */
int iupdate(inode_t *ip)
{
    if (!ip) return -1;
    ip->i_flag &= (uint16_t)~(IUPD | IACC);
    return 0;
}

/* ── itrunc ────────────────────────────────────────────────────
 * Free all data blocks of an inode.
 */
int itrunc(inode_t *ip)
{
    int i;
    for (i = 0; i < NBLOCK_DIRECT + NBLOCK_INDIRECT; i++) {
        if (ip->i_addr[i]) {
            bfree(ip->i_dev, ip->i_addr[i]);
            ip->i_addr[i] = 0;
        }
    }
    ip->i_size  = 0;
    ip->i_flag |= IUPD;
    return 0;
}

/* ── iaccess ───────────────────────────────────────────────────
 * Check whether current process has the requested access
 * (mode: R=4, W=2, X=1) to inode ip.
 */
int iaccess(inode_t *ip, int mode)
{
    uint16_t perm = ip->i_mode & 0777;

    if (u.u_uid == 0)
        return 1;
    if (u.u_uid == ip->i_uid)
        perm = (uint16_t)(perm >> 6);
    else if (u.u_gid == ip->i_gid)
        perm = (uint16_t)(perm >> 3);

    return (perm & (uint16_t)mode) == (uint16_t)mode;
}

/* ── namei ─────────────────────────────────────────────────────
 * Parse a pathname and return the locked inode.
 *
 * strtok() replaced with a manual token walker — strtok uses
 * hidden static state and is not available freestanding.
 */
inode_t *namei(const char *path)
{
    inode_t *dp;
    char     buf[256];
    char    *p;

    if (!path || *path == '\0') return NULL;

    dp = (*path == '/') ? u.u_rdir : u.u_cdir;
    if (!dp) return NULL;

    dp->i_count++;   /* hold starting directory */

    strncpy(buf, path, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    p = buf;

    while (*p) {
        char    component[256];
        char   *c = component;
        inode_t *next;

        /* skip slashes */
        while (*p == '/') p++;
        if (*p == '\0') break;

        /* extract next component into component[] */
        while (*p && *p != '/') *c++ = *p++;
        *c = '\0';

        /* dp must be a directory */
        if ((dp->i_mode & IFMT) != IFDIR) { iput(dp); return NULL; }
        if (!iaccess(dp, 1)) { iput(dp); return NULL; }

        /* handle . and .. */
        if (strcmp(component, ".") == 0)
            continue;

        if (strcmp(component, "..") == 0) {
            /* at root, stay at root */
            if (dp == u.u_rdir) continue;
            /* otherwise: real kernel would walk to parent;
             * sim: treat as not found */
            iput(dp);
            return NULL;
        }

        /*
         * In a real kernel: bread directory blocks, compare
         * dirent names, iget the matching inode.
         */
        next = NULL;   /* placeholder — real impl reads dir blocks */
        (void)component;

        if (!next) { iput(dp); return NULL; }
        iput(dp);
        dp = next;
    }
    return dp;
}
