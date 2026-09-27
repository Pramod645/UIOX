/*
 * 30_KIX/32_FS/10_unfs/include/unfs_errno.h
 *
 * UNFS error codes for the kernel-side implementation files.
 *
 * ── why this file exists ──────────────────────────────────────────────
 * unfs_format.c, unfs_io.h and the xattr file all include it, and it was
 * missing from the tree — the mkfs writer could not compile without it.
 *
 * ── TWO SOURCES, ONE SET OF NAMES ─────────────────────────────────────
 * unfs_format.h already defines the on-disk-side codes:
 *
 *     UNFS_EOK        0
 *     UNFS_EIO     -100
 *     UNFS_EINVAL  -101
 *     UNFS_ENOENT  -102
 *     UNFS_ENOTSUP -103
 *     UNFS_ECORRUPT-117
 *
 * The implementation files, however, are written against a DIFFERENT
 * success name: unfs_format.c returns UNFS_OK at line 278 and tests
 * "!= UNFS_OK" at eight call sites, while the format header offers
 * UNFS_EOK.  Both spellings have to work, and neither may be defined
 * twice — hence the #ifndef on every name below, and UNFS_OK as an alias
 * of UNFS_EOK rather than a second definition with its own value.
 *
 * The format codes are NOT redefined.  Only what is missing is added:
 * UNFS_OK, and UNFS_ENOSPC (which the format header does not carry and
 * the mkfs writer needs for its two out-of-space returns).
 *
 * ── the numbering ─────────────────────────────────────────────────────
 * The -100 block continues upward from unfs_format.h so a code can never
 * mean two things.  -104 is the next free value; ENOSPC takes it.
 *
 * These are INTERNAL codes, not POSIX errno values.  A syscall body in
 * 10_scfs translates them at the boundary; nothing in the kernel should
 * assume UNFS_EINVAL equals -EINVAL.
 *
 * @version 1.0.0  @date 2026-09-27
 */
#ifndef UNFS_ERRNO_H
#define UNFS_ERRNO_H

/* unfs_format.h carries the on-disk-side codes.  Include it so this
 * header is self-sufficient — a caller that needs UNFS_EIO should not
 * have to know that it lives in the layout header. */
#include "unfs_format.h"

/* ── Success ──────────────────────────────────────────────────────────
 * unfs_format.c uses UNFS_OK; unfs_format.h defines UNFS_EOK.  One value,
 * two spellings, so a caller can use either without a mismatch. */
#ifndef UNFS_EOK
#define UNFS_EOK   0
#endif
#ifndef UNFS_OK
#define UNFS_OK    UNFS_EOK
#endif

/* ── The codes the format header already defines ──────────────────────
 * Guarded, not restated: if unfs_format.h is edited to change a value,
 * this header follows rather than fighting it. */
#ifndef UNFS_EIO
#define UNFS_EIO       -100
#endif
#ifndef UNFS_EINVAL
#define UNFS_EINVAL    -101
#endif
#ifndef UNFS_ENOENT
#define UNFS_ENOENT    -102
#endif
#ifndef UNFS_ENOTSUP
#define UNFS_ENOTSUP   -103
#endif

/* ── Added here ───────────────────────────────────────────────────────
 * UNFS_ENOSPC: no free blocks, no free inode, or the requested geometry
 * does not fit.  unfs_format.c returns it from two places (a volume too
 * small for its metadata, and a full group while hunting for the root
 * data block). */
#ifndef UNFS_ENOSPC
#define UNFS_ENOSPC    -104
#endif

/* ── Others the format header carries, guarded for the same reason ──── */
#ifndef UNFS_ECORRUPT
#define UNFS_ECORRUPT  -117   /* checksum mismatch                       */
#endif

/* ── Reserved for the parts of 10_unfs not yet ported ─────────────────
 * Named so a future file does not invent its own value in a different
 * range.  Nothing returns these today. */
#ifndef UNFS_ENOMEM
#define UNFS_ENOMEM    -105   /* allocator could not satisfy a request   */
#endif
#ifndef UNFS_EBADF
#define UNFS_EBADF     -106   /* bad device or inode number              */
#endif
#ifndef UNFS_EROFS
#define UNFS_EROFS     -107   /* write attempted on a read-only mount    */
#endif

#endif /* UNFS_ERRNO_H */
