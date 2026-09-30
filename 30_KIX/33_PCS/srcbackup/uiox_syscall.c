/*
 * 30_KIX/33_PCS/src/uiox_syscall.c
 *
 * UIOX Syscall dispatch and handler implementations.
 *
 * This is the bridge between userspace requests and
 * kernel/SoC/FwHal data. Every handler that returns
 * data to user space uses uiox_copy_to_user().
 * Every handler that receives data from user space
 * uses uiox_copy_from_user().
 *
 * @version 1.0.0
 * @date    2026-07-28
 */

 #include "uiox_syscall.h"
 #include "uiox_uaccess.h"
 #include "uiox_soc_stdio.h"
 
 /* ── Error codes ───────────────────────────────────────────────────── */
 #define ENOSYS   38
 #define EBADF     9
 #define EINVAL   22
 #define EFAULT   14
 #define ENOMEM   12
 
 /* ── Syscall dispatch table ────────────────────────────────────────── */
 /*
  * Function pointer type for a generic syscall handler.
  * All handlers receive the raw frame; specific wrappers
  * unpack args with the correct types below.
  */
 typedef uiox_syscall_ret_t (*uiox_syscall_fn_t)(const uiox_syscall_frame_t *);
 
 /* Forward declarations of frame-unpacking wrappers */
 static uiox_syscall_ret_t _sys_read    (const uiox_syscall_frame_t *f);
 static uiox_syscall_ret_t _sys_write   (const uiox_syscall_frame_t *f);
 static uiox_syscall_ret_t _sys_open    (const uiox_syscall_frame_t *f);
 static uiox_syscall_ret_t _sys_close   (const uiox_syscall_frame_t *f);
 static uiox_syscall_ret_t _sys_ioctl   (const uiox_syscall_frame_t *f);
 static uiox_syscall_ret_t _sys_mmap    (const uiox_syscall_frame_t *f);
 static uiox_syscall_ret_t _sys_munmap  (const uiox_syscall_frame_t *f);
 static uiox_syscall_ret_t _sys_getpid  (const uiox_syscall_frame_t *f);
 static uiox_syscall_ret_t _sys_exit    (const uiox_syscall_frame_t *f);
 static uiox_syscall_ret_t _sys_sync    (const uiox_syscall_frame_t *f);
 static uiox_syscall_ret_t _sys_fsync   (const uiox_syscall_frame_t *f);
 static uiox_syscall_ret_t _sys_enosys  (const uiox_syscall_frame_t *f);
 
 /* Dispatch table — indexed by syscall number */
 static const uiox_syscall_fn_t s_syscall_table[UIOX_NR_SYSCALLS] = {
     [SYS_READ]        = _sys_read,
     [SYS_WRITE]       = _sys_write,
     [SYS_OPEN]        = _sys_open,
     [SYS_CLOSE]       = _sys_close,
     [SYS_IOCTL]       = _sys_ioctl,
     [SYS_MMAP]        = _sys_mmap,
     [SYS_MUNMAP]      = _sys_munmap,
     [SYS_GETPID]      = _sys_getpid,
     [SYS_EXIT]        = _sys_exit,
     [SYS_SYNC]        = _sys_sync,
     [SYS_FSYNC]       = _sys_fsync,
     /* all others default to NULL → _sys_enosys in dispatch */
 };
 

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





/*
 * Add to uiox_syscall.h — missing syscall numbers:
 */
#define SYS_LSEEK          8
#define SYS_STAT           4
#define SYS_FSTAT          5
#define SYS_MKDIR         83
#define SYS_RMDIR         84
#define SYS_UNLINK        87
#define SYS_GETDENTS      78
#define SYS_CHDIR         80
#define SYS_GETCWD       179
#define SYS_PIPE           22
#define SYS_DUP            32
#define SYS_DUP2           33
#define SYS_NANOSLEEP      35
#define SYS_CLOCK_GETTIME 228

/*
 * Add to s_syscall_table[] in uiox_syscall.c:
 */
[SYS_LSEEK]       = _sys_lseek,
[SYS_STAT]        = _sys_stat,
[SYS_MKDIR]       = _sys_mkdir,
[SYS_RMDIR]       = _sys_rmdir,
[SYS_UNLINK]      = _sys_unlink,
[SYS_GETDENTS]    = _sys_getdents,
[SYS_MMAP]        = _sys_mmap,
[SYS_MUNMAP]      = _sys_munmap,
[SYS_SYNC]        = _sys_sync,
[SYS_FSYNC]       = _sys_fsync,

/*
 * Frame-unpacking wrappers for new syscalls:
 */
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
static uiox_syscall_ret_t _sys_mkdir(const uiox_syscall_frame_t *f)
{
    /* TODO: copy path from user, call vfs_mkdir */
    (void)f; return UIOX_SYSCALL_ERR(38);  /* ENOSYS stub */
}
static uiox_syscall_ret_t _sys_rmdir(const uiox_syscall_frame_t *f)
{
    (void)f; return UIOX_SYSCALL_ERR(38);
}
static uiox_syscall_ret_t _sys_unlink(const uiox_syscall_frame_t *f)
{
    (void)f; return UIOX_SYSCALL_ERR(38);
}
static uiox_syscall_ret_t _sys_mmap(const uiox_syscall_frame_t *f)
{
    return sys_mmap((void *)f->a0, (size_t)f->a1,
                    (int)f->a2,   (int)f->a3,
                    (int)f->a4,   (long)f->a5);
}
static uiox_syscall_ret_t _sys_munmap(const uiox_syscall_frame_t *f)
{
    return sys_munmap((void *)f->a0, (size_t)f->a1);
}
static uiox_syscall_ret_t _sys_sync(const uiox_syscall_frame_t *f)
{
    (void)f;
    extern int uiox_jr_force_commit(void);
    extern int uiox_jr_checkpoint(void);
    uiox_jr_force_commit();
    uiox_jr_checkpoint();
    return 0;
}
static uiox_syscall_ret_t _sys_fsync(const uiox_syscall_frame_t *f)
{
    return sys_fsync((int)f->a0);
}











 /* ── Main dispatcher ───────────────────────────────────────────────── */
 uiox_syscall_ret_t uiox_syscall_dispatch(const uiox_syscall_frame_t *f)
 {
     uiox_syscall_fn_t fn;
 
     if (!f)
         return UIOX_SYSCALL_ERR(EINVAL);
 
     if (f->nr >= UIOX_NR_SYSCALLS)
         return UIOX_SYSCALL_ERR(ENOSYS);
 
     fn = s_syscall_table[f->nr];
     if (!fn)
         return _sys_enosys(f);
 
     return fn(f);
 }
 
 /* ── _sys_enosys ───────────────────────────────────────────────────── */
 static uiox_syscall_ret_t _sys_enosys(const uiox_syscall_frame_t *f)
 {
     (void)f;
     return UIOX_SYSCALL_ERR(ENOSYS);
 }
 
 /* =========================================================================
  * SYS_READ — read from file descriptor into user buffer
  *
  * Kernel data flow:
  *   fd → file → driver/FwHal → kernel_buf
  *   uiox_copy_to_user(user_buf, kernel_buf, count)
  * ====================================================================== */
 static uiox_syscall_ret_t _sys_read(const uiox_syscall_frame_t *f)
 {
     int    fd    = (int)f->a0;
     void  *ubuf  = (void *)f->a1;
     size_t count = (size_t)f->a2;
 
     return sys_read(fd, ubuf, count);
 }
 
 uiox_syscall_ret_t sys_read(int fd, void *ubuf, size_t count)
 {
     /* Validate user buffer before touching any kernel state */
     if (!uiox_uaccess_ok(ubuf, count))
         return UIOX_SYSCALL_ERR(EFAULT);
     if (fd < 0)
         return UIOX_SYSCALL_ERR(EBADF);
     if (count == 0u)
         return 0;
 
     /*
      * TODO: look up fd in process file table (33_PCS/40_procStruct)
      * and dispatch to the file's read operation.
      * For now return ENOSYS to indicate not yet wired.
      *
      * When wired, the pattern is:
      *
      *   uint8_t kbuf[count];           // kernel-side buffer
      *   ssize_t n = file->ops->read(file, kbuf, count, &pos);
      *   if (n < 0) return UIOX_SYSCALL_ERR(-n);
      *   if (uiox_copy_to_user(ubuf, kbuf, n) != 0)
      *       return UIOX_SYSCALL_ERR(EFAULT);
      *   return (uiox_syscall_ret_t)n;
      */
     return UIOX_SYSCALL_ERR(ENOSYS);
 }
 
 /* =========================================================================
  * SYS_WRITE — write from user buffer into file descriptor
  *
  * Kernel data flow:
  *   uiox_copy_from_user(kernel_buf, user_buf, count)
  *   kernel_buf → driver/FwHal → device
  * ====================================================================== */
 static uiox_syscall_ret_t _sys_write(const uiox_syscall_frame_t *f)
 {
     int         fd    = (int)f->a0;
     const void *ubuf  = (const void *)f->a1;
     size_t      count = (size_t)f->a2;
 
     return sys_write(fd, ubuf, count);
 }
 
 uiox_syscall_ret_t sys_write(int fd, const void *ubuf, size_t count)
 {
     if (!uiox_uaccess_ok(ubuf, count))
         return UIOX_SYSCALL_ERR(EFAULT);
     if (fd < 0)
         return UIOX_SYSCALL_ERR(EBADF);
     if (count == 0u)
         return 0;
 
     /*
      * TODO: look up fd → file → file->ops->write()
      *
      *   uint8_t kbuf[count];
      *   if (uiox_copy_from_user(kbuf, ubuf, count) != 0)
      *       return UIOX_SYSCALL_ERR(EFAULT);
      *   ssize_t n = file->ops->write(file, kbuf, count, &pos);
      *   return (n < 0) ? UIOX_SYSCALL_ERR(-n) : (uiox_syscall_ret_t)n;
      */
     return UIOX_SYSCALL_ERR(ENOSYS);
 }
 
 /* =========================================================================
  * SYS_OPEN
  * ====================================================================== */
 static uiox_syscall_ret_t _sys_open(const uiox_syscall_frame_t *f)
 {
     const char *upath = (const char *)f->a0;
     int flags = (int)f->a1;
     int mode  = (int)f->a2;
     return sys_open(upath, flags, mode);
 }
 
 uiox_syscall_ret_t sys_open(const char *upath, int flags, int mode)
 {
     /*
      * Validate user path pointer.
      * We check a minimum of 1 byte — real check would walk
      * the string up to PATH_MAX validating each byte is in user VA.
      */
     if (!uiox_uaccess_ok(upath, 1u))
         return UIOX_SYSCALL_ERR(EFAULT);
     (void)flags; (void)mode;
 
     /*
      * TODO: copy path from user via uiox_copy_from_user(),
      * look up in 32_FS VFS, allocate fd in process file table.
      *
      *   char kpath[PATH_MAX];
      *   size_t len = uiox_strnlen_user(upath, PATH_MAX);
      *   if (uiox_copy_from_user(kpath, upath, len) != 0)
      *       return UIOX_SYSCALL_ERR(EFAULT);
      *   return uiox_vfs_open(kpath, flags, mode);
      */
     return UIOX_SYSCALL_ERR(ENOSYS);
 }
 
 /* =========================================================================
  * SYS_CLOSE
  * ====================================================================== */
 static uiox_syscall_ret_t _sys_close(const uiox_syscall_frame_t *f)
 {
     return sys_close((int)f->a0);
 }
 
 uiox_syscall_ret_t sys_close(int fd)
 {
     if (fd < 0)
         return UIOX_SYSCALL_ERR(EBADF);
     /* TODO: release fd from process file table */
     return UIOX_SYSCALL_ERR(ENOSYS);
 }
 
 /* =========================================================================
  * SYS_IOCTL — device control
  *
  * Key path for SoC / FwHal data → userspace:
  *   user calls ioctl(fd, UIOX_IOC_GET_SENSOR_DATA, &user_struct)
  *   kernel reads from FwHal / SoC / driver
  *   copy_to_user(&user_struct, &kernel_struct, sizeof(kernel_struct))
  * ====================================================================== */
 static uiox_syscall_ret_t _sys_ioctl(const uiox_syscall_frame_t *f)
 {
     return sys_ioctl((int)f->a0,
                      (unsigned long)f->a1,
                      (unsigned long)f->a2);
 }
 
 uiox_syscall_ret_t sys_ioctl(int fd, unsigned long cmd, unsigned long arg)
 {
     if (fd < 0)
         return UIOX_SYSCALL_ERR(EBADF);
 
     /*
      * TODO: dispatch to device-specific ioctl handler.
      * Pattern for returning SoC data to user:
      *
      *   uiox_sensor_data_t kdata;
      *   driver->ioctl(fd, cmd, &kdata);     // fill from SoC/FwHal
      *   if (uiox_copy_to_user((void *)arg,
      *                          &kdata,
      *                          sizeof(kdata)) != 0)
      *       return UIOX_SYSCALL_ERR(EFAULT);
      *   return 0;
      */
     (void)cmd; (void)arg;
     return UIOX_SYSCALL_ERR(ENOSYS);
 }
 
 /* =========================================================================
  * SYS_MMAP — map kernel / device memory into user VA (zero-copy)
  *
  * Key path for buffer pool data → userspace without copy:
  *   31_BufferCache buffer PA → mapped into user page table
  *   user reads directly — no copy_to_user needed
  * ====================================================================== */
 static uiox_syscall_ret_t _sys_mmap(const uiox_syscall_frame_t *f)
 {
     return sys_mmap((void *)f->a0,
                     (size_t)f->a1,
                     (int)f->a2,
                     (int)f->a3,
                     (int)f->a4,
                     (long)f->a5);
 }
 
 uiox_syscall_ret_t sys_mmap(void *addr, size_t len, int prot,
                               int flags, int fd, long off)
 {
     (void)addr; (void)prot; (void)flags; (void)fd; (void)off;
     if (len == 0u)
         return UIOX_SYSCALL_ERR(EINVAL);
 
     /*
      * TODO: allocate user VA range, map physical pages
      * from 33_PCS/02_MemMngnt into the process page table
      * via uiox_mm_map_user().
      *
      *   uintptr_t pa = uiox_buf_get_paddr(fd, off);
      *   uintptr_t va = uiox_mm_map_user(current_proc, pa, len, prot);
      *   return (va == 0) ? UIOX_SYSCALL_ERR(ENOMEM)
      *                    : (uiox_syscall_ret_t)va;
      */
     return UIOX_SYSCALL_ERR(ENOSYS);
 }
 
 /* =========================================================================
  * SYS_MUNMAP
  * ====================================================================== */
 static uiox_syscall_ret_t _sys_munmap(const uiox_syscall_frame_t *f)
 {
     return sys_munmap((void *)f->a0, (size_t)f->a1);
 }
 
 uiox_syscall_ret_t sys_munmap(void *addr, size_t len)
 {
     if (!uiox_uaccess_ok(addr, len))
         return UIOX_SYSCALL_ERR(EINVAL);
     /* TODO: unmap from process page table */
     return UIOX_SYSCALL_ERR(ENOSYS);
 }
 
 /* =========================================================================
  * SYS_GETPID
  * ====================================================================== */
 static uiox_syscall_ret_t _sys_getpid(const uiox_syscall_frame_t *f)
 {
     (void)f;
     return sys_getpid();
 }
 
 uiox_syscall_ret_t sys_getpid(void)
 {
     /*
      * TODO: return current->pid from 33_PCS/40_procStruct
      */
     return 1; /* stub: PID 1 */
 }
 
 /* =========================================================================
  * SYS_EXIT
  * ====================================================================== */
 static uiox_syscall_ret_t _sys_exit(const uiox_syscall_frame_t *f)
 {
     return sys_exit((int)f->a0);
 }
 
 uiox_syscall_ret_t sys_exit(int code)
 {
     (void)code;
     /*
      * TODO: call uiox_proc_exit(current, code)
      * which cleans up page tables, fds, signals scheduler.
      */
     for (;;) {
 #if defined(__x86_64__)
         __asm__ volatile("hlt");
 #else
         __asm__ volatile("wfi");
 #endif
     }
 }
 
 /* =========================================================================
  * SYS_SYNC / SYS_FSYNC — journal flush (32_FS/02_journal)
  * ====================================================================== */
 static uiox_syscall_ret_t _sys_sync(const uiox_syscall_frame_t *f)
 {
     (void)f;
     return sys_sync();
 }
 
 uiox_syscall_ret_t sys_sync(void)
 {
     /* TODO: uiox_jr_force_commit() + checkpoint — 32_FS/02_journal */
     return 0;
 }
 
 static uiox_syscall_ret_t _sys_fsync(const uiox_syscall_frame_t *f)
 {
     return sys_fsync((int)f->a0);
 }
 
 uiox_syscall_ret_t sys_fsync(int fd)
 {
     if (fd < 0) return UIOX_SYSCALL_ERR(EBADF);
     /* TODO: uiox_jr_force_commit() for specific fd */
     return 0;
 }
 


 /*
 File 10 — 33_PCS/src/uiox_syscall.c (update SYS_SYNC handlers)
Replace the current SYS_SYNC stubs to call through the page cache sync:
 */
/* Replace the _sys_sync and _sys_fsync wrappers in uiox_syscall.c */

extern int uiox_pc_sync_all(void);
extern int uiox_pc_sync_inode(uint32_t ino);

static uiox_syscall_ret_t _sys_sync(const uiox_syscall_frame_t *f)
{
    (void)f;
    /*
     * Full sync order:
     *   1. uiox_pc_writeback_all() — dirty pages → block device
     *   2. uiox_jr_force_commit()  — journal commit block
     *   3. uiox_jr_checkpoint()    — advance journal head
     * All three steps are inside uiox_pc_sync_all().
     */
    uiox_pc_sync_all();
    return 0;
}

static uiox_syscall_ret_t _sys_fsync(const uiox_syscall_frame_t *f)
{
    int fd = (int)f->a0;
    /*
     * Get inode number from fd → file → inode.
     * For now: sync all (single-process boot stage).
     * TODO: fd_lookup(fd)->f_inode->i_ino → uiox_pc_sync_inode(ino)
     */
    (void)fd;
    uiox_pc_sync_inode(0u);   /* 0 = sync all inodes (stub) */
    return 0;
}

static uiox_syscall_ret_t _sys_fdatasync(const uiox_syscall_frame_t *f)
{
    return _sys_fsync(f);   /* data-only sync = same as fsync here */
}

static uiox_syscall_ret_t _sys_syncfs(const uiox_syscall_frame_t *f)
{
    (void)f;
    uiox_pc_sync_all();
    return 0;
}
