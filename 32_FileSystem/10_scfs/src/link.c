#include "../include/fs.h"
#include "../include/inode.h"
#include <libgen.h>
#include <string.h>

/*
 * Algorithm link
 * input : existing file name, new file name
 * output: none (0 on success)
 */
int fs_link(const char *oldpath, const char *newpath)
{
    /* Get inode for existing file (algorithm namei) */
    inode_t *ip = namei(oldpath);
    if (!ip) return FS_ENOENT;

    /* Disallow linking directories (unless super user) */
    if ((ip->i_mode & IFMT) == IFDIR && u.u_uid != 0) {
        iput(ip);
        return FS_EPERM;
    }

    /* Too many links? */
    if (ip->i_nlink >= MAX_LINKS) {
        iput(ip);
        return FS_EMLINK;
    }

    /* Increment link count on inode */
    ip->i_nlink++;
    ip->i_flag |= IUPD;
    iupdate(ip);         /* update disk copy of inode */
    iunlock(ip);

    /* Get parent inode for directory containing new file name */
    char newpath_copy[256];
    strncpy(newpath_copy, newpath, sizeof(newpath_copy) - 1);
    newpath_copy[sizeof(newpath_copy) - 1] = '\0';

    inode_t *parent_ip = namei(dirname(newpath_copy));
    if (!parent_ip) {
        /* Undo increment */
        ilock(ip);
        ip->i_nlink--;
        ip->i_flag |= IUPD;
        iupdate(ip);
        iput(ip);
        return FS_ENOENT;
    }

    /* New name must not already exist */
    inode_t *exist = namei(newpath);
    if (exist) {
        iput(exist);
        iput(parent_ip);
        /* Undo */
        ilock(ip);
        ip->i_nlink--;
        ip->i_flag |= IUPD;
        iupdate(ip);
        iput(ip);
        return FS_EEXIST;
    }

    /* Must be on same file system */
    if (parent_ip->i_dev != ip->i_dev) {
        iput(parent_ip);
        ilock(ip);
        ip->i_nlink--;
        ip->i_flag |= IUPD;
        iupdate(ip);
        iput(ip);
        return FS_EXDEV;
    }

    /* Create new directory entry: new name + existing inode number */
    /* (real kernel: write dirent into parent directory blocks) */

    iput(parent_ip);
    iput(ip);
    return FS_OK;
}

/*
 * Algorithm unlink
 * input : file name
 * output: none (0 on success)
 */
int fs_unlink(const char *path)
{
    char path_copy[256];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    /* Get parent inode of file to be unlinked (algorithm namei) */
    inode_t *parent_ip = namei(dirname(path_copy));
    if (!parent_ip) return FS_ENOENT;

    /* Get inode of file to be unlinked (algorithm iget) */
    inode_t *ip = namei(path);
    if (!ip) { iput(parent_ip); return FS_ENOENT; }

    /* Directories only removable by super user */
    if ((ip->i_mode & IFMT) == IFDIR && u.u_uid != 0) {
        iput(ip);
        iput(parent_ip);
        return FS_EPERM;
    }

    /* If shared text file and link count currently 1,
     * remove from region table (simulated) */
    if ((ip->i_flag & ITEXT) && ip->i_nlink == 1) {
        /* remove_from_region_table(ip); */
    }

    /* Write parent directory: zero inode number of unlinked file */
    /* (real kernel: find dirent in parent dir blocks, zero i_ino) */

    /* Release parent directory inode (algorithm iput) */
    iput(parent_ip);

    /* Decrement file link count */
    ip->i_nlink--;
    ip->i_flag |= IUPD;

    /* Release file inode (algorithm iput)
     * iput checks if link count == 0:
     *   if so, releases file blocks (algorithm free)
     *   and frees inode (algorithm ifree)              */
    iput(ip);

    return FS_OK;
}
