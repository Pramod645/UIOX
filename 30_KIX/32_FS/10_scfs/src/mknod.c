/*
 *  30_KIX/32_FS/10_scfs/src/mknod.c  — freestanding fix v1.1
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

/*
 * Algorithm mknod
 * input : file name, file type/permissions, device numbers
 * output: 0 on success
 */
int fs_mknod(const char *path, uint16_t mode, uint8_t major, uint8_t minor)
{
    char     parent_buf[256];
    inode_t *parent_ip;
    inode_t *exist;
    inode_t *ip;

    if (!path) return FS_ENOENT;
    if (u.u_uid != 0) return FS_EPERM;   /* only superuser */

    fs_dirname(path, parent_buf, sizeof parent_buf);
    parent_ip = namei(parent_buf);
    if (!parent_ip) return FS_ENOENT;

    exist = namei(path);
    if (exist) {
        iput(exist);
        iput(parent_ip);
        return FS_EEXIST;
    }

    if (!iaccess(parent_ip, 2)) { iput(parent_ip); return FS_EACCES; }

    ip = ialloc(parent_ip->i_dev);
    if (!ip) { iput(parent_ip); return FS_ENFILE; }

    ip->i_mode  = mode;
    ip->i_nlink = 1;
    ip->i_uid   = u.u_uid;
    ip->i_gid   = u.u_gid;
    ip->i_major = major;
    ip->i_minor = minor;
    ip->i_flag |= IUPD;

    iupdate(ip);
    iput(ip);
    iput(parent_ip);
    printf("[mknod] '%s' mode=0%o major=%u minor=%u\n",
           path, mode, major, minor);
    return FS_OK;
}
