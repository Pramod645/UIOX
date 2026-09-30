/*
 * 30_KIX/33_PCS/src/uiox_sys_fd.c(new — the wiring layer)
 *
 * File descriptor syscall handlers.
 *
 * These are the crossing-point functions — they sit at the
 * exact boundary between kernel space and user space.
 *
 * Data flow — READ (DRAM → userspace):
 * ─────────────────────────────────────
 *   Physical DRAM page (page cache in 32_FS/10_scfs)
 *     → scfs_read() memcpy → kbuf  (kernel stack buffer)
 *     → vfs_read()  returns kbuf to sys_read()
 *     → uiox_copy_to_user(ubuf, kbuf, n)   ← CROSSING POINT
 *     → user process buffer  ← data arrives
 *
 * Data flow — WRITE (userspace → DRAM):
 * ──────────────────────────────────────
 *   User process buffer
 *     → uiox_copy_from_user(kbuf, ubuf, n)  ← CROSSING POINT
 *     → kbuf (kernel stack buffer)
 *     → vfs_write() → scfs_write() / journal
 *     → DRAM page cache → block device
 *
 * @version 1.0.0  @date 2026-07-29
 */

 #include "uiox_syscall.h"
 #include "uiox_uaccess.h"
 #include "uiox_vfs.h"
 #include "uiox_soc_string.h"
 
 /* ── Per-process file descriptor table ─────────────────────────────── */
 /*
  * In a full kernel this lives in 33_PCS/40_procStruct per process.
  * For now: single global table (single process / boot stage).
  */
 #define UIOX_PROC_FD_MAX   64u
 
 typedef struct {
     uiox_file_t *file;
     uint8_t      inuse;
 } uiox_fd_entry_t;
 
 static uiox_fd_entry_t s_fd_table[UIOX_PROC_FD_MAX];
 
 /* Stdin/stdout/stderr stubs (fd 0,1,2) */
 static void fd_table_init(void)
 {
     memset(s_fd_table, 0, sizeof(s_fd_table));
     /* fd 0,1,2 reserved — null for now */
     s_fd_table[0].inuse = 1u;
     s_fd_table[1].inuse = 1u;
     s_fd_table[2].inuse = 1u;
 }
 
 static int fd_alloc(uiox_file_t *file)
 {
     /* Start from 3 — skip stdin/stdout/stderr */
     for (uint32_t i = 3u; i < UIOX_PROC_FD_MAX; i++) {
         if (!s_fd_table[i].inuse) {
             s_fd_table[i].file  = file;
             s_fd_table[i].inuse = 1u;
             return (int)i;
         }
     }
     return UIOX_FS_ENOMEM;
 }
 
 static uiox_file_t *fd_lookup(int fd)
 {
     if (fd < 0 || (uint32_t)fd >= UIOX_PROC_FD_MAX)
         return (uiox_file_t *)0;
     if (!s_fd_table[fd].inuse)
         return (uiox_file_t *)0;
     return s_fd_table[fd].file;
 }
 
 static void fd_free(int fd)
 {
     if (fd < 0 || (uint32_t)fd >= UIOX_PROC_FD_MAX) return;
     s_fd_table[fd].file  = (uiox_file_t *)0;
     s_fd_table[fd].inuse = 0u;
 }
 
 /* ── uiox_fd_init — called from uiox_sys_call_init() ──────────────── */
 void uiox_fd_init(void)
 {
     fd_table_init();
 }
 
 /* =========================================================================
  * sys_open
  *
  * Data flow:
  *   user passes path string in user VA
  *   copy_from_user(kpath, upath, len)  ← crossing point
  *   vfs_open(kpath, flags, mode)       ← VFS resolves path
  *   fd_alloc(file)                     ← assign fd
  *   return fd to user
  * ====================================================================== */
 uiox_syscall_ret_t sys_open(const char *upath, int flags, int mode)
 {
     /* Validate user path pointer */
     if (!uiox_uaccess_ok(upath, 1u))
         return UIOX_SYSCALL_ERR(-UIOX_FS_EFAULT);
 
     /*
      * Copy path from user space into kernel buffer.
      * We must not touch upath directly in kernel code —
      * it is a user virtual address.
      */
     char kpath[UIOX_PATH_MAX];
     memset(kpath, 0, sizeof(kpath));
 
     /* Find length (bounded by PATH_MAX) */
     size_t len = 0u;
     while (len < UIOX_PATH_MAX - 1u) {
         uint8_t c = 0u;
         if (uiox_get_user_u8(&c, (const uint8_t *)upath + len) != 0)
             return UIOX_SYSCALL_ERR(-UIOX_FS_EFAULT);
         kpath[len] = (char)c;
         if (c == '\0') break;
         len++;
     }
 
     /* Open via VFS */
     uiox_file_t *file = (uiox_file_t *)0;
     int rc = vfs_open(kpath, (uint32_t)flags, (uint32_t)mode, &file);
     if (rc != 0) return UIOX_SYSCALL_ERR(-rc);
 
     /* Assign file descriptor */
     int fd = fd_alloc(file);
     if (fd < 0) {
         vfs_close(file);
         return UIOX_SYSCALL_ERR(-UIOX_FS_ENOMEM);
     }
 
     return (uiox_syscall_ret_t)fd;
 }
 
 /* =========================================================================
  * sys_close
  * ====================================================================== */
 uiox_syscall_ret_t sys_close(int fd)
 {
     uiox_file_t *file = fd_lookup(fd);
     if (!file) return UIOX_SYSCALL_ERR(-UIOX_FS_EBADF);
 
     int rc = vfs_close(file);
     fd_free(fd);
     return (rc == 0) ? 0 : UIOX_SYSCALL_ERR(-rc);
 }
 
 /* =========================================================================
  * sys_read
  *
  * This is the primary DRAM → userspace crossing function.
  *
  * Step 1: validate user buffer pointer
  * Step 2: look up fd → file
  * Step 3: vfs_read(file, kbuf, count)  — fills kernel buffer from DRAM
  * Step 4: uiox_copy_to_user(ubuf, kbuf, n)  — crosses privilege boundary
  * Step 5: return bytes read to user
  * ====================================================================== */
 uiox_syscall_ret_t sys_read(int fd, void *ubuf, size_t count)
 {
     /* Step 1 — validate user destination pointer */
     if (!uiox_uaccess_ok(ubuf, count))
         return UIOX_SYSCALL_ERR(-UIOX_FS_EFAULT);
 
     if (count == 0u) return 0;
 
     /* Step 2 — look up file */
     uiox_file_t *file = fd_lookup(fd);
     if (!file) return UIOX_SYSCALL_ERR(-UIOX_FS_EBADF);
 
     /*
      * Step 3 — read into kernel buffer.
      *
      * kbuf lives on the kernel stack (or in kmalloc'd memory).
      * It is in kernel VA space — safe for the filesystem to write.
      * Maximum single read: one page (4 KB) to keep stack usage bounded.
      */
 #define SYS_READ_CHUNK  4096u
     uint8_t kbuf[SYS_READ_CHUNK];
     size_t  total_read = 0u;
     uint8_t *udst      = (uint8_t *)ubuf;
 
     while (total_read < count) {
         size_t   chunk = count - total_read;
         if (chunk > SYS_READ_CHUNK) chunk = SYS_READ_CHUNK;
 
         /* Fill kbuf from VFS/SCFS/page cache */
         ssize_t n = vfs_read(file, kbuf, chunk);
         if (n < 0)  return UIOX_SYSCALL_ERR((int)n);
         if (n == 0) break;   /* EOF */
 
         /*
          * Step 4 — copy kernel buffer to user space.
          * This is THE crossing point for read data.
          * uiox_copy_to_user validates udst is in user VA range
          * then copies bytes across the privilege boundary.
          */
         if (uiox_copy_to_user(udst + total_read, kbuf, (size_t)n) != 0)
             return UIOX_SYSCALL_ERR(-UIOX_FS_EFAULT);
 
         total_read += (size_t)n;
     }
 
     return (uiox_syscall_ret_t)total_read;
 }
 
 /* =========================================================================
  * sys_write
  *
  * This is the primary userspace → DRAM crossing function.
  *
  * Step 1: validate user source pointer
  * Step 2: look up fd → file
  * Step 3: uiox_copy_from_user(kbuf, ubuf, n) — crosses privilege boundary
  * Step 4: vfs_write(file, kbuf, count) — writes kernel buffer to DRAM
  * Step 5: return bytes written to user
  * ====================================================================== */
 uiox_syscall_ret_t sys_write(int fd, const void *ubuf, size_t count)
 {
     /* Step 1 — validate user source pointer */
     if (!uiox_uaccess_ok(ubuf, count))
         return UIOX_SYSCALL_ERR(-UIOX_FS_EFAULT);
 
     if (count == 0u) return 0;
 
     /* Step 2 — look up file */
     uiox_file_t *file = fd_lookup(fd);
     if (!file) return UIOX_SYSCALL_ERR(-UIOX_FS_EBADF);
 
     uint8_t     kbuf[SYS_READ_CHUNK];
     size_t      total_written = 0u;
     const uint8_t *usrc = (const uint8_t *)ubuf;
 
     while (total_written < count) {
         size_t chunk = count - total_written;
         if (chunk > SYS_READ_CHUNK) chunk = SYS_READ_CHUNK;
 
         /*
          * Step 3 — copy from user space into kernel buffer.
          * This is THE crossing point for write data.
          * After this, kbuf is safe for the filesystem to read.
          */
         if (uiox_copy_from_user(kbuf,
                                  usrc + total_written,
                                  chunk) != 0)
             return UIOX_SYSCALL_ERR(-UIOX_FS_EFAULT);
 
         /* Step 4 — write kernel buffer to VFS/SCFS/DRAM */
         ssize_t n = vfs_write(file, kbuf, chunk);
         if (n < 0)  return UIOX_SYSCALL_ERR((int)n);
         if (n == 0) break;
 
         total_written += (size_t)n;
     }
 
     return (uiox_syscall_ret_t)total_written;
 }
 
 /* =========================================================================
  * sys_lseek
  * ====================================================================== */
 uiox_syscall_ret_t sys_lseek(int fd, int64_t offset, int whence)
 {
     uiox_file_t *file = fd_lookup(fd);
     if (!file) return UIOX_SYSCALL_ERR(-UIOX_FS_EBADF);
 
     int rc = vfs_seek(file, offset, whence);
     if (rc != 0) return UIOX_SYSCALL_ERR(-rc);
     return (uiox_syscall_ret_t)file->f_pos;
 }
 
 /* =========================================================================
  * sys_stat
  *
  * Data flow:
  *   vfs_stat(kpath, &kstat)   — fills kernel stat struct
  *   copy_to_user(ustat, &kstat, sizeof(kstat))  ← crossing point
  * ====================================================================== */
 uiox_syscall_ret_t sys_stat(const char *upath, void *ustat)
 {
     if (!uiox_uaccess_ok(upath, 1u))
         return UIOX_SYSCALL_ERR(-UIOX_FS_EFAULT);
     if (!uiox_uaccess_ok(ustat, sizeof(uiox_stat_t)))
         return UIOX_SYSCALL_ERR(-UIOX_FS_EFAULT);
 
     /* Copy path from user */
     char kpath[UIOX_PATH_MAX];
     size_t len = 0u;
     while (len < UIOX_PATH_MAX - 1u) {
         uint8_t c = 0u;
         if (uiox_get_user_u8(&c, (const uint8_t *)upath + len) != 0)
             return UIOX_SYSCALL_ERR(-UIOX_FS_EFAULT);
         kpath[len] = (char)c;
         if (c == '\0') break;
         len++;
     }
 
     /* Get stat from VFS */
     uiox_stat_t kstat;
     int rc = vfs_stat(kpath, &kstat);
     if (rc != 0) return UIOX_SYSCALL_ERR(-rc);
 
     /* Copy stat struct to user */
     if (uiox_copy_to_user(ustat, &kstat, sizeof(kstat)) != 0)
         return UIOX_SYSCALL_ERR(-UIOX_FS_EFAULT);
 
     return 0;
 }
 
 /* =========================================================================
  * sys_getdents — read directory entries into user buffer
  *
  * Data flow:
  *   vfs_readdir(file, &kde)    — fills kernel dirent
  *   copy_to_user(ude, &kde, sizeof(kde))  ← crossing point
  * ====================================================================== */
 uiox_syscall_ret_t sys_getdents(int fd, void *ubuf, size_t count)
 {
     if (!uiox_uaccess_ok(ubuf, count))
         return UIOX_SYSCALL_ERR(-UIOX_FS_EFAULT);
 
     uiox_file_t *file = fd_lookup(fd);
     if (!file) return UIOX_SYSCALL_ERR(-UIOX_FS_EBADF);
 
     size_t        written = 0u;
     uint32_t      idx     = (uint32_t)(file->f_pos);
     uint8_t      *udst    = (uint8_t *)ubuf;
     uiox_dirent_t kde;
 
     while (written + sizeof(kde) <= count) {
         int rc = vfs_readdir(file, &kde, idx);
         if (rc == UIOX_FS_ENOENT) break;   /* end of directory */
         if (rc != 0) return UIOX_SYSCALL_ERR(-rc);
 
         /* Copy kernel dirent to user buffer */
         if (uiox_copy_to_user(udst + written,
                                &kde,
                                sizeof(kde)) != 0)
             return UIOX_SYSCALL_ERR(-UIOX_FS_EFAULT);
 
         written += sizeof(kde);
         idx++;
     }
 
     file->f_pos = idx;
     return (uiox_syscall_ret_t)written;
 }
 
 /* =========================================================================
  * sys_fsync
  * ====================================================================== */
 uiox_syscall_ret_t sys_fsync(int fd)
 {
     uiox_file_t *file = fd_lookup(fd);
     if (!file) return UIOX_SYSCALL_ERR(-UIOX_FS_EBADF);
 
     int rc = vfs_fsync(file);
     return (rc == 0) ? 0 : UIOX_SYSCALL_ERR(-rc);
 }
 