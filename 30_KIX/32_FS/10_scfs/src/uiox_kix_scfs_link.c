/*
 *  30_KIX/32_FS/10_scfs/src/link.c
 *
 *  Freestanding fixes (v1.1):
 *    FIXED: #include "../../33_PCS/..."  →  #include "uiox_klibc.h"
 *    FIXED: dirname()  →  fs_dirname() — inline freestanding implementation
 *    FIXED: fprintf(stderr, ...)  →  printf(...)
 *    FIXED: for (int i = ...)     →  int i; before loop (strict C11)
 */
#include "../include/fs.h"
#include "../include/inode.h"
#include "uiox_klibc.h"

/* ─────────────────────────────────────────────────────────────
 * fs_dirname — freestanding replacement for POSIX dirname().
 * Identical copy to creat.c — kept local (static) so no header change needed.
 * ───────────────────────────────────────────────────────────── */
static char *fs_dirname(const char *path, char *out, size_t outsz)
{
    size_t      len;
    const char *last_slash;
    const char *p;

    if (!path || path[0] == '\0') {
        strncpy(out, ".", outsz - 1);
        out[outsz - 1] = '\0';
        return out;
    }

    strncpy(out, path, outsz - 1);
    out[outsz - 1] = '\0';
    len = strlen(out);

    /* strip trailing slashes */
    while (len > 1 && out[len - 1] == '/') out[--len] = '\0';

    /* find last slash */
    last_slash = NULL;
    for (p = out; *p; p++)
        if (*p == '/') last_slash = p;

    if (!last_slash) {
        strncpy(out, ".", outsz - 1);
        out[outsz - 1] = '\0';
    } else if (last_slash == out) {
        out[1] = '\0';
    } else {
        out[last_slash - out] = '\0';
    }
    return out;
}

/* ─────────────────────────────────────────────────────────────
 * fs_basename — freestanding replacement for POSIX basename().
 * Returns a pointer into 'path' (no allocation).
 * ───────────────────────────────────────────────────────────── */
static const char *fs_basename(const char *path)
{
    const char *last = path;
    const char *p;
    if (!path || !*path) return ".";
    for (p = path; *p; p++)
        if (*p == '/' && *(p + 1) != '\0') last = p + 1;
    return last;
}

/*
 * Algorithm link
 * input : existing file name, new file name
 * output: 0 on success, negative error code on failure
 */
int fs_link(const char *oldpath, const char *newpath)
{
    char     newdir_buf[256];
    inode_t *ip;
    inode_t *parent_ip;

    if (!oldpath || !newpath) return FS_ENOENT;

    /* Get inode of existing file (algorithm namei) */
    ip = namei(oldpath);
    if (!ip) return FS_ENOENT;

    /* Cannot link to a directory (unless super user) */
    if ((ip->i_mode & IFMT) == IFDIR && u.u_uid != 0) {
        iput(ip);
        return FS_EPERM;
    }

    /* Check link count won't overflow */
    if (ip->i_nlink >= MAX_LINKS) {
        iput(ip);
        return FS_EMLINK;
    }

    /* Get parent directory of new path */
    fs_dirname(newpath, newdir_buf, sizeof newdir_buf);
    parent_ip = namei(newdir_buf);
    if (!parent_ip) { iput(ip); return FS_ENOENT; }

    /* Parent must be a directory on the same device */
    if ((parent_ip->i_mode & IFMT) != IFDIR) {
        iput(ip); iput(parent_ip);
        return FS_ENOTDIR;
    }
    if (parent_ip->i_dev != ip->i_dev) {
        iput(ip); iput(parent_ip);
        return FS_EXDEV;
    }
    if (!iaccess(parent_ip, 2)) {
        iput(ip); iput(parent_ip);
        return FS_EACCES;
    }

    /* Write new directory entry (simulated) */
    {
        const char *newname = fs_basename(newpath);
        printf("[link] '%s' -> '%s' (ino=%u)\n",
               newpath, oldpath, ip->i_number);
        (void)newname;
    }

    ip->i_nlink++;
    ip->i_flag |= IUPD;
    iupdate(ip);

    iput(parent_ip);
    iput(ip);
    return FS_OK;
}

/*
 * Algorithm unlink
 * input : file name
 * output: 0 on success, negative error code on failure
 */
int fs_unlink(const char *path)
{
    char     parent_buf[256];
    inode_t *parent_ip;
    inode_t *ip;

    if (!path) return FS_ENOENT;

    /* Get parent directory inode */
    fs_dirname(path, parent_buf, sizeof parent_buf);
    parent_ip = namei(parent_buf);
    if (!parent_ip) return FS_ENOENT;

    /* Check write permission on parent */
    if (!iaccess(parent_ip, 2)) {
        iput(parent_ip);
        return FS_EACCES;
    }

    /* Get inode of file to be unlinked */
    ip = namei(path);
    if (!ip) { iput(parent_ip); return FS_ENOENT; }

    /* Directories only removable by super user */
    if ((ip->i_mode & IFMT) == IFDIR && u.u_uid != 0) {
        iput(ip); iput(parent_ip);
        return FS_EPERM;
    }

    /* If shared text file and link count currently 1,
     * remove from region table (simulated) */
    if ((ip->i_flag & ITEXT) && ip->i_nlink == 1) {
        /* remove_from_region_table(ip); */
    }

    /* Remove directory entry (simulated) */
    printf("[unlink] removing '%s' (ino=%u)\n", path, ip->i_number);

    /* Decrement link count */
    ip->i_nlink--;
    ip->i_flag |= IUPD;
    iupdate(ip);

    iput(parent_ip);
    iput(ip);
    return FS_OK;
}
