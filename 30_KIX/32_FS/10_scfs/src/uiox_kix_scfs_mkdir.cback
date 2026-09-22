/*
 *  30_KIX/32_FS/10_scfs/src/mkdir.c  — freestanding fix v1.1
 *    FIXED: ../../33_PCS path, dirname(), fprintf(stderr,...)
 */
#include "../include/fs.h"
#include "../include/inode.h"
#include "uiox_klibc.h"

/* ── fs_dirname: freestanding dirname replacement ────────────── */
static char *fs_dirname(const char *path, char *out, size_t outsz)
{
    size_t len; const char *last = NULL; const char *p;
    if (!path || !*path) { strncpy(out, ".", outsz-1); out[outsz-1]='\0'; return out; }
    strncpy(out, path, outsz-1); out[outsz-1]='\0';
    len = strlen(out);
    while (len > 1 && out[len-1] == '/') out[--len] = '\0';
    for (p = out; *p; p++) if (*p == '/') last = p;
    if (!last)                    { strncpy(out, ".", outsz-1); out[outsz-1]='\0'; }
    else if (last == out)         { out[1] = '\0'; }
    else                          { out[last - out] = '\0'; }
    return out;
}
/* ── fs_basename: freestanding basename replacement ──────────── */
static const char *fs_basename(const char *path)
{
    const char *last = path; const char *p;
    if (!path || !*path) return ".";
    for (p = path; *p; p++) if (*p == '/' && *(p+1) != '\0') last = p+1;
    return last;
}

/*
 * Algorithm mkdir
 * input : directory name, permissions
 * output: 0 on success
 */
int fs_mkdir(const char *path, uint16_t mode)
{
    char        parent_buf[256];
    const char *newname;
    inode_t    *parent_ip;
    inode_t    *ip;

    if (!path) return FS_ENOENT;

    fs_dirname(path, parent_buf, sizeof parent_buf);
    newname   = fs_basename(path);
    parent_ip = namei(parent_buf);
    if (!parent_ip) return FS_ENOENT;

    if ((parent_ip->i_mode & IFMT) != IFDIR) {
        iput(parent_ip); return FS_ENOTDIR;
    }
    if (!iaccess(parent_ip, 2)) { iput(parent_ip); return FS_EACCES; }

    ip = ialloc(parent_ip->i_dev);
    if (!ip) { iput(parent_ip); return FS_ENFILE; }

    ip->i_mode  = (uint16_t)(IFDIR | (mode & ~(uint16_t)u.u_umask));
    ip->i_nlink = 2;   /* '.' and parent */
    ip->i_uid   = u.u_uid;
    ip->i_gid   = u.u_gid;
    ip->i_flag |= IUPD;

    iupdate(ip);

    printf("[mkdir] '%s' (ino=%u, parent='%s')\n",
           newname, ip->i_number, parent_buf);

    iput(ip);
    iput(parent_ip);
    return FS_OK;
}

int fs_rmdir(const char *path)
{
    inode_t *ip;
    if (!path) return FS_ENOENT;
    ip = namei(path);
    if (!ip) return FS_ENOENT;
    if ((ip->i_mode & IFMT) != IFDIR) { iput(ip); return FS_ENOTDIR; }
    if (u.u_uid != 0 && u.u_uid != ip->i_uid) { iput(ip); return FS_EPERM; }
    /* In a real kernel: verify directory is empty first */
    ip->i_nlink = 0;
    ip->i_flag |= IUPD;
    iupdate(ip);
    iput(ip);
    printf("[rmdir] '%s' removed\n", path);
    return FS_OK;
}
