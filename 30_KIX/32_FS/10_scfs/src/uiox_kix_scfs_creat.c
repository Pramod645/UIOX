/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_creat.c
 *
 * SCFS — Algorithm creat.  Bach, The Design of the UNIX Operating System.
 *
 * ── Bach's algorithm, verbatim ─────────────────────────────────────────
 * input: file name, permission settings
 * output: file descriptor
 * {
 *   get inode for file name (algorithm namei);
 *   if (file already exists)
 *   {
 *       if (not permitted access)
 *       {
 *           release inode (algorithm iput);
 *           return (error);
 *       }
 *   }
 *   else                                     // file does not exist yet
 *   {
 *       assign free inode from file system (algorithm ialloc);
 *       create new directory entry in parent directory: include new file
 *           name and newly assigned inode number;
 *   }
 *   allocate file table entry for inode, initialize count;
 *   if (file did exist at time of create)
 *       free all file blocks (algorithm free);
 *   unlock (inode);
 *   return (user file descriptor);
 * }
 *
 * creat() is open(path, O_CREAT|O_WRONLY|O_TRUNC) written out as its own
 * algorithm.  The old kernel had it separately because the shell used it
 * constantly; both paths reach the same two 01_fsa calls — ialloc() and
 * dir_add() — or, when the file exists, fs_free_inode_blocks().
 *
 * @version 1.0.0  @date 2026-09-23
 */
#include "uiox_kix_scfs_internal.h"

int uiox_kix_scfs_creat(const char *path, uint16_t perm)
{
    if (!path) return SCFS_EFAULT;

    /* ── get the inode for the file name (algorithm namei) ──────────── */
    InCoreInode *ip = namei(path, scfs_cwd_get(), 0u, 0u);   /* 01_fsa */

    int existed = (ip != (InCoreInode *)0);

    if (existed) {
        /* ── the file exists — check access is permitted ────────────── */
        if (!inode_access_ok(ip, 0u, 0u, 0, 1, 0)) {   /* 01_fsa */
            iput(ip);                                  /* release it */
            return SCFS_EACCES;
        }
        if (SCFS_IS_DIR(ip->mode)) {
            iput(ip);
            return SCFS_EISDIR;      /* creat() makes regular files */
        }
    } else {
        /* ── the file does not exist — assign a free inode and enter
         *    the new name in its parent ─────────────────────────────── */
        ip = scfs_create_node(path, perm);
        if (!ip) return SCFS_EACCES;
    }

    /* ── allocate a FILE TABLE entry, initialize count ──────────────── */
    scfs_file_t *f = scfs_falloc(ip, O_WRONLY, 0777u);
    if (!f) { iput(ip); return SCFS_ENFILE; }

    /* ── allocate the USER FD slot ─────────────────────────────────── */
    int fd = scfs_ufd_alloc(f);
    if (fd < 0) {
        scfs_fclose_entry(f);
        iput(ip);
        return SCFS_EMFILE;
    }

    /* ── the file existed: free all its blocks (algorithm free) ─────── */
    /* Bach frees the FILE BLOCKS, not the inode — the inode is reused,
     * which is what makes creat() on an existing file a truncation. */
    if (existed && ip->size != 0u) {
        fs_free_inode_blocks(ip);        /* 01_fsa: algorithm free */
        iupdate(ip);
    }

    /* ── unlock, and release the local inode reference ──────────────── */
    /* The file table entry now holds the reference; the one namei/ialloc
     * gave us is dropped here. */
    ip->locked = false;
    iput(ip);

    return fd;
}
