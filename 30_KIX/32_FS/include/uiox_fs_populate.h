/* 30_KIX/32_FS/include/uiox_fs_populate.h */
#ifndef UIOX_FS_POPULATE_H
#define UIOX_FS_POPULATE_H

/* Write the embedded userspace image into the mounted root, if it is not
 * already there.  Returns 0 on success, -1 on failure.
 *
 * Idempotent: a second boot finds /bin/uios present and returns 0
 * without touching the volume. */
int uiox_fs_populate_root(void);

#endif /* UIOX_FS_POPULATE_H */
