/*
 * src/file_utils.c
 *
 * Utility functions: validation, table management, /dev/fd support,
 * vnode operations, path lookup.
 */

 #include "file_io.h"
 #include "file_internal.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 #include <time.h>
 
 /* =============================================================
  * Validation functions
  * ============================================================= */
 bool valid_fd(int fd, proc_fd_table_t *pft)
 {
     return fd >= 0 && fd < OPEN_MAX && 
            pft->pft_fds[fd].fde_file != NULL;
 }
 
 bool valid_flags(int oflag)
 {
     int access_mode = oflag & O_ACCMODE;
     return access_mode <= O_SEARCH;
 }
 
 bool valid_mode(mode_t mode)
 {
     return (mode & ~07777) == 0;
 }
 
 /* =============================================================
  * File table management
  * ============================================================= */
 file_entry_t *file_table_alloc(void)
 {
     for (int i = 0; i < FILE_TABLE_SIZE; i++) {
         if (!file_table[i].fe_active) {
             memset(&file_table[i], 0, sizeof(file_entry_t));
             file_table[i].fe_active = true;
             return &file_table[i];
         }
     }
     return NULL;
 }
 
 void file_table_free(file_entry_t *fe)
 {
     if (fe) {
         fe->fe_active = false;
         fe->fe_refcount = 0;
     }
 }
 
 file_entry_t *file_table_get(int index)
 {
     if (index >= 0 && index < FILE_TABLE_SIZE)
         return &file_table[index];
     return NULL;
 }
 
 /* =============================================================
  * Process fd table operations
  * ============================================================= */
 int proc_fd_alloc(proc_fd_table_t *pft, file_entry_t *fe)
 {
     int fd = proc_fd_lowest_available(pft);
     if (fd < 0) return -1;
     
     pft->pft_fds[fd].fde_file = fe;
     pft->pft_fds[fd].fde_flags = 0;
     
     if (fd > pft->pft_max_fd)
         pft->pft_max_fd = fd;
         
     return fd;
 }
 
 void proc_fd_free(proc_fd_table_t *pft, int fd)
 {
     if (fd >= 0 && fd < OPEN_MAX) {
         pft->pft_fds[fd].fde_file = NULL;
         pft->pft_fds[fd].fde_flags = 0;
     }
 }
 
 int proc_fd_lowest_available(proc_fd_table_t *pft)
 {
     for (int i = 0; i < OPEN_MAX; i++) {
         if (!pft->pft_fds[i].fde_file) {
             return i;
         }
     }
     return -1;
 }
 
 /* =============================================================
  * /dev/fd support
  * ============================================================= */
 bool is_devfd_path(const char *path)
 {
     return strncmp(path, "/dev/fd/", 8) == 0 ||
            strcmp(path, "/dev/stdin")  == 0 ||
            strcmp(path, "/dev/stdout") == 0 ||
            strcmp(path, "/dev/stderr") == 0;
 }
 
 int devfd_extract_fd(const char *path)
 {
     if (strcmp(path, "/dev/stdin")  == 0) return STDIN_FILENO;
     if (strcmp(path, "/dev/stdout") == 0) return STDOUT_FILENO;
     if (strcmp(path, "/dev/stderr") == 0) return STDERR_FILENO;
     
     if (strncmp(path, "/dev/fd/", 8) == 0) {
         return atoi(path + 8);
     }
     
     return -1;
 }
 
 /* =============================================================
  * Vnode cache management
  * ============================================================= */
 vnode_t *vnode_get(uint32_t ino)
 {
     for (int i = 0; i < VNODE_CACHE_SIZE; i++) {
         if (vnode_cache[i].v_ino == ino) {
             return &vnode_cache[i];
         }
     }
     
     /* Load from inode table */
     if (ino > 0 && ino < MAX_INODES && inode_table[ino].di_mode != 0) {
         vnode_t *vp = vnode_alloc();
         if (vp) {
             disk_inode_t *dip = &inode_table[ino];
             vp->v_ino    = ino;
             vp->v_mode   = dip->di_mode;
             vp->v_size   = dip->di_size;
             vp->v_blocks = dip->di_blocks;
             vp->v_atime  = dip->di_atime;
             vp->v_mtime  = dip->di_mtime;
             vp->v_ctime  = dip->di_ctime;
             vp->v_uid    = dip->di_uid;
             vp->v_gid    = dip->di_gid;
             vp->v_nlink  = dip->di_nlink;
         }
         return vp;
     }
     
     return NULL;
 }
 
 vnode_t *vnode_alloc(void)
 {
     for (int i = 0; i < VNODE_CACHE_SIZE; i++) {
         if (vnode_cache[i].v_ino == 0) {
             memset(&vnode_cache[i], 0, sizeof(vnode_t));
             return &vnode_cache[i];
         }
     }
     return NULL;
 }
 
 void vnode_free(vnode_t *vp)
 {
     if (vp) {
         vp->v_ino = 0;
     }
 }
 
 /* =============================================================
  * Path lookup (simplified)
  * ============================================================= */
 vnode_t *path_lookup(const char *path, proc_fd_table_t *pft)
 {
     (void)pft;
     
     /* Simplified path lookup */
     if (strcmp(path, "/") == 0) {
         return vnode_get(1);  /* root inode */
     }
     
     /* Look for existing files in inode table */
     for (uint32_t ino = 1; ino < MAX_INODES; ino++) {
         if (inode_table[ino].di_mode != 0) {
             return vnode_get(ino);
         }
     }
     
     return NULL;
 }
 
 /* =============================================================
  * Vnode operations
  * ============================================================= */
 int vnode_create(const char *path, mode_t mode)
 {
     /* Find free inode */
     for (uint32_t ino = 2; ino < MAX_INODES; ino++) {
         if (inode_table[ino].di_mode == 0) {
             disk_inode_t *dip = &inode_table[ino];
             dip->di_mode  = (uint16_t)(S_IFREG | (mode & 07777));
             dip->di_nlink = 1;
             dip->di_uid   = 0;
             dip->di_gid   = 0;
             dip->di_size  = 0;
             dip->di_atime = dip->di_mtime = dip->di_ctime = time(NULL);
             
             printf("[vnode_create] path='%s' ino=%u mode=0%o\n",
                    path, ino, mode);
             return 0;
         }
     }
     return -1;  /* no free inodes */
 }
 
 /* =============================================================
  * File data I/O (simplified direct block access)
  * ============================================================= */
 ssize_t file_data_read(vnode_t *vp, void *buf, size_t count, off_t offset)
 {
     if (offset < 0 || offset >= (off_t)vp->v_size) {
         return 0;
     }
     
     size_t to_read = count;
     if (offset + (off_t)to_read > (off_t)vp->v_size) {
         to_read = vp->v_size - (size_t)offset;
     }
     
     /* Simplified: read from first disk block */
     uint32_t blk_offset = (uint32_t)offset % BLOCK_SIZE;
     size_t chunk = to_read;
     if (chunk > BLOCK_SIZE - blk_offset) {
         chunk = BLOCK_SIZE - blk_offset;
     }
     
     memcpy(buf, &disk_blocks[vp->v_ino][blk_offset], chunk);
     
     printf("[file_data_read] ino=%u offset=%ld count=%zu → %zu\n",
            vp->v_ino, (long)offset, count, chunk);
     return (ssize_t)chunk;
 }
 
 ssize_t file_data_write(vnode_t *vp, const void *buf, size_t count, off_t offset)
 {
     if (offset < 0) {
         file_errno = EINVAL;
         return -1;
     }
     
     /* Simplified: write to first disk block */
     uint32_t blk_offset = (uint32_t)offset % BLOCK_SIZE;
     size_t chunk = count;
     if (chunk > BLOCK_SIZE - blk_offset) {
         chunk = BLOCK_SIZE - blk_offset;
     }
     
     memcpy(&disk_blocks[vp->v_ino][blk_offset], buf, chunk);
     
     printf("[file_data_write] ino=%u offset=%ld count=%zu → %zu\n",
            vp->v_ino, (long)offset, count, chunk);
     return (ssize_t)chunk;
 }
 
 /* =============================================================
  * File access permission check
  * ============================================================= */
 bool file_access_ok(vnode_t *vp, int flags, uint16_t uid, uint16_t gid)
 {
     (void)uid; (void)gid;  /* simplified: always allow access */
     
     switch (flags & O_ACCMODE) {
     case O_RDONLY:
         return true;
     case O_WRONLY:
         return true;
     case O_RDWR:
         return true;
     default:
         return false;
     }
 }
 
 /* =============================================================
  * ioctl support
  * ============================================================= */
 int file_ioctl(int fd, unsigned long request, void *argp)
 {
     if (!valid_fd(fd, current_fd_table)) {
         file_errno = EBADF;
         return -1;
     }
     
     printf("[ioctl] fd=%d request=0x%lx\n", fd, request);
     
     /* Device-specific ioctl operations would go here */
     switch (request) {
     case 0x5401:  /* TCGETS — get terminal attributes */
         printf("  [ioctl] TCGETS (terminal get attributes)\n");
         return 0;
     case 0x5402:  /* TCSETS — set terminal attributes */
         printf("  [ioctl] TCSETS (terminal set attributes)\n");
         return 0;
     default:
         printf("  [ioctl] unknown request 0x%lx\n", request);
         file_errno = ENOTTY;
         return -1;
     }
 }
 
 /* =============================================================
  * Debug functions
  * ============================================================= */
 void fileio_print_tables(void)
 {
     printf("[fileio] File table:\n");
     printf("  Index  Active  Ino     Flags   Offset   RefCnt\n");
     for (int i = 0; i < FILE_TABLE_SIZE; i++) {
         if (file_table[i].fe_active) {
             printf("  %-6d %-7s %-7u 0x%-5x %-8ld %d\n",
                    i, "yes",
                    file_table[i].fe_ino,
                    file_table[i].fe_flags,
                    (long)file_table[i].fe_offset,
                    file_table[i].fe_refcount);
         }
     }
     
     printf("[fileio] Process fd table:\n");
     printf("  FD  File  Flags\n");
     for (int i = 0; i <= current_fd_table->pft_max_fd; i++) {
         if (current_fd_table->pft_fds[i].fde_file) {
             printf("  %-3d %-5d 0x%x\n",
                    i,
                    (int)(current_fd_table->pft_fds[i].fde_file - file_table),
                    current_fd_table->pft_fds[i].fde_flags);
         }
     }
 }
 
 void fileio_print_stats(void)
 {
     printf("[fileio] Statistics:\n");
     printf("  open   calls: %llu\n", (unsigned long long)file_stats.fs_open_calls);
     printf("  read   calls: %llu  bytes: %llu\n", 
            (unsigned long long)file_stats.fs_read_calls,
            (unsigned long long)file_stats.fs_bytes_read);
     printf("  write  calls: %llu  bytes: %llu\n",
            (unsigned long long)file_stats.fs_write_calls,
            (unsigned long long)file_stats.fs_bytes_written);
     printf("  lseek  calls: %llu\n", (unsigned long long)file_stats.fs_lseek_calls);
     printf("  close  calls: %llu\n", (unsigned long long)file_stats.fs_close_calls);
     printf("  open   files: %u\n", file_stats.fs_open_files);
     printf("  max fd  used: %u\n", file_stats.fs_max_fd_used);
 }
 
 /* =============================================================
  * Error handling
  * ============================================================= */
 const char *file_strerror(int errnum)
 {
     switch (errnum) {
     case ENOENT:       return "No such file or directory";
     case EBADF:        return "Bad file descriptor";
     case ENAMETOOLONG: return "File name too long";
     case ESPIPE:       return "Illegal seek";
     case ENOSYS:       return "Function not implemented";
     case EACCES:       return "Permission denied";
     case EEXIST:       return "File exists";
     case ENFILE:       return "Too many open files in system";
     case EMFILE:       return "Too many open files";
     case ENOTDIR:      return "Not a directory";
     case EINVAL:       return "Invalid argument";
     case ENOTTY:       return "Inappropriate ioctl for device";
     default:           return "Unknown error";
     }
 }
 