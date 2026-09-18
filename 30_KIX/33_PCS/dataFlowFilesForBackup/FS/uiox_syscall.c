/*
 * Add to s_syscall_table[] in uiox_syscall.c:
 * (replace the existing ENOSYS stubs for read/write/open/close)
 */

/* Additional includes at top */
#include "uiox_sys_fd.h"    /* sys_read, sys_write, sys_open, sys_close */

/* Updated dispatch table entries */
[SYS_READ]        = _sys_read,
[SYS_WRITE]       = _sys_write,
[SYS_OPEN]        = _sys_open,
[SYS_CLOSE]       = _sys_close,
[SYS_LSEEK]       = _sys_lseek,    /* add SYS_LSEEK = 8 */
[SYS_STAT]        = _sys_stat,     /* add SYS_STAT  = 4 */
[SYS_FSTAT]       = _sys_fstat,    /* add SYS_FSTAT = 5 */
[SYS_GETDENTS]    = _sys_getdents, /* add SYS_GETDENTS = 78 */
[SYS_FSYNC]       = _sys_fsync,

/* Updated frame-unpacking wrappers */
static uiox_syscall_ret_t _sys_read(const uiox_syscall_frame_t *f)
{
    return sys_read((int)f->a0, (void *)f->a1, (size_t)f->a2);
}

static uiox_syscall_ret_t _sys_write(const uiox_syscall_frame_t *f)
{
    return sys_write((int)f->a0, (const void *)f->a1, (size_t)f->a2);
}

static uiox_syscall_ret_t _sys_open(const uiox_syscall_frame_t *f)
{
    return sys_open((const char *)f->a0, (int)f->a1, (int)f->a2);
}

static uiox_syscall_ret_t _sys_close(const uiox_syscall_frame_t *f)
{
    return sys_close((int)f->a0);
}

static uiox_syscall_ret_t _sys_lseek(const uiox_syscall_frame_t *f)
{
    return sys_lseek((int)f->a0, (int64_t)f->a1, (int)f->a2);
}

static uiox_syscall_ret_t _sys_stat(const uiox_syscall_frame_t *f)
{
    return sys_stat((const char *)f->a0, (void *)f->a1);
}

static uiox_syscall_ret_t _sys_getdents(const uiox_syscall_frame_t *f)
{
    return sys_getdents((int)f->a0, (void *)f->a1, (size_t)f->a2);
}

static uiox_syscall_ret_t _sys_fsync(const uiox_syscall_frame_t *f)
{
    return sys_fsync((int)f->a0);
}
