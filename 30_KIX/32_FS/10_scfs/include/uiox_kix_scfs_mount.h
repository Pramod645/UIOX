/*
 * 30_KIX/32_FS/10_scfs/include/uiox_kix_scfs_mount.h   — modernised
 *
 * UIOX mount table + superblock (Bach Ch.4/9).
 *
 * @version 2.0.0  @date 2026-09-21
 */
#ifndef UIOX_KIX_SCFS_MOUNT_H
#define UIOX_KIX_SCFS_MOUNT_H

#include "uiox_kix_scfs_inode.h"
#include "uiox_kix_scfs_buf.h"
#include "uiox_klibc.h"

#define NMOUNT  20

/* Super block (Bach Ch.4). */
typedef struct super_block {
    uint32_t s_isize;       /* number of blocks of i-nodes            */
    uint32_t s_fsize;       /* total blocks in the filesystem         */
    uint32_t s_nfree;       /* count of free blocks in s_free[]       */
    uint32_t s_free[50];    /* free block list (Bach 50-entry cache)  */
    uint16_t s_ninode;      /* count of free i-nodes in s_inode[]     */
    uint32_t s_inode[100];  /* free i-node list (Bach 100-entry cache)*/
    uint8_t  s_flock;       /* free block list lock                   */
    uint8_t  s_ilock;       /* free i-node list lock                  */
    uint8_t  s_fmod;        /* super block modified flag              */
    uint8_t  s_ronly;       /* read-only mount flag                   */
    time_t   s_time;        /* last super block write                 */
    uint32_t s_tfree;       /* total free blocks (statfs)             */
    uint16_t s_tinode;      /* total free i-nodes (statfs)            */
    uint16_t s_m;           /* interleave factor                      */
    uint16_t s_n;           /* sectors per cylinder                   */
    char     s_fname[6];    /* filesystem name                        */
    char     s_fpack[6];    /* filesystem pack name                   */
} super_block_t;

/* Mount table entry (Bach Ch.9) */
typedef struct mount {
    uint16_t      m_dev;         /* device number                     */
    buf_t        *m_bufp;        /* buffer holding the super block    */
    inode_t      *m_inodp;       /* i-node of the covered directory   */
    inode_t      *m_mount_root;  /* root i-node of the mounted FS     */
    super_block_t m_sb;          /* this filesystem's super block     */
    int           m_flags;       /* MNT_* flags                       */
    uint8_t       m_mounted;     /* slot in use                       */
    const char   *m_fsname;      /* registered backend name           */
} mount_t;

#define MNT_RDONLY  0x01
#define MNT_FORCE   0x02     /* umount2: skip the busy check          */
#define MNT_NOSUID  0x04
#define MNT_NOEXEC  0x08

extern mount_t mount_table[NMOUNT];

mount_t *getmount(uint16_t dev);        /* NULL when not mounted */
mount_t *mount_alloc(void);
void     mount_free(mount_t *mp);

/* Bach Ch.9 — path walk reaching a mount point is redirected to the
 * mounted filesystem's root i-node. */
inode_t *mount_find_root(inode_t *covered);

#endif /* UIOX_KIX_SCFS_MOUNT_H */
