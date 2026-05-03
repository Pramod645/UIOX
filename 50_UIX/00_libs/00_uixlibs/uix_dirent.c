#include "uix_dirent.h"
#include "uix_stdlib.h"
#include "uix_string.h"
#include "uix_fcntl.h"
#include "uix_unistd.h"
#include "uix_errno.h"

uix_DIR *uix_opendir(const char *name)
{
    int fd = uix_open(name, UIX_O_RDONLY | UIX_O_DIRECTORY, 0);
    if (fd < 0) return NULL;
    uix_DIR *dir = (uix_DIR *)uix_malloc(sizeof(uix_DIR));
    if (!dir) { uix_close(fd); return NULL; }
    dir->fd      = fd;
    dir->buf_pos = 0;
    dir->buf_len = 0;
    dir->offset  = 0;
    return dir;
}

uix_DIR *uix_fdopendir(int fd)
{
    uix_DIR *dir = (uix_DIR *)uix_malloc(sizeof(uix_DIR));
    if (!dir) return NULL;
    dir->fd = fd; dir->buf_pos = dir->buf_len = 0; dir->offset = 0;
    return dir;
}

uix_dirent_t *uix_readdir(uix_DIR *dirp)
{
    if (!dirp) { uix_errno = UIX_EBADF; return NULL; }
    extern long sys_getdents(int, void *, uix_size_t)
        __attribute__((weak));
    if (!sys_getdents) return NULL;

    if (dirp->buf_pos >= dirp->buf_len) {
        uix_ssize_t n = sys_getdents(dirp->fd,
                                     dirp->buf, sizeof(dirp->buf));
        if (n <= 0) return NULL;
        dirp->buf_len = (int)n;
        dirp->buf_pos = 0;
    }
    uix_dirent_t *d = (uix_dirent_t *)(dirp->buf + dirp->buf_pos);
    dirp->buf_pos  += d->d_reclen;
    dirp->offset++;
    dirp->entry     = *d;
    return &dirp->entry;
}

int uix_readdir_r(uix_DIR *dirp, uix_dirent_t *entry,
                  uix_dirent_t **result)
{
    if (!dirp || !entry) { uix_errno = UIX_EINVAL; return -1; }
    uix_dirent_t *d = uix_readdir(dirp);
    if (d) { *entry = *d; *result = entry; }
    else   *result = NULL;
    return 0;
}

int uix_closedir(uix_DIR *dirp)
{
    if (!dirp) { uix_errno = UIX_EBADF; return -1; }
    int r = uix_close(dirp->fd);
    uix_free(dirp);
    return r;
}

void uix_rewinddir(uix_DIR *dirp)
{
    if (!dirp) return;
    uix_lseek(dirp->fd, 0, UIX_SEEK_SET);
    dirp->buf_pos = dirp->buf_len = 0;
    dirp->offset  = 0;
}

long uix_telldir(uix_DIR *dirp)
{
    return dirp ? (long)dirp->offset : -1;
}

void uix_seekdir(uix_DIR *dirp, long loc)
{
    if (!dirp) return;
    uix_rewinddir(dirp);
    while (dirp->offset < (uix_off_t)loc)
        if (!uix_readdir(dirp)) break;
}

int uix_alphasort(const uix_dirent_t **a, const uix_dirent_t **b)
{
    return uix_strcmp((*a)->d_name, (*b)->d_name);
}

int uix_scandir(const char *dirp, uix_dirent_t ***namelist,
                int (*filter)(const uix_dirent_t *),
                int (*compar)(const uix_dirent_t **,
                              const uix_dirent_t **))
{
    uix_DIR *d = uix_opendir(dirp);
    if (!d) return -1;

    uix_dirent_t **list = NULL;
    int count = 0, cap = 0;
    uix_dirent_t *entry;

    while ((entry = uix_readdir(d)) != NULL) {
        if (filter && !filter(entry)) continue;
        if (count >= cap) {
            cap  = cap ? cap * 2 : 16;
            list = (uix_dirent_t **)uix_realloc(list,
                        (uix_size_t)cap * sizeof(uix_dirent_t *));
            if (!list) { uix_closedir(d); return -1; }
        }
        uix_dirent_t *copy =
            (uix_dirent_t *)uix_malloc(sizeof(uix_dirent_t));
        *copy = *entry;
        list[count++] = copy;
    }
    uix_closedir(d);

    if (compar && count > 1)
        uix_qsort(list, (uix_size_t)count,
                  sizeof(uix_dirent_t *),
                  (int (*)(const void *, const void *))compar);

    *namelist = list;
    return count;
}
