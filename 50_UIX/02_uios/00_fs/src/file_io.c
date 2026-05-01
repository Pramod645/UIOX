/*
 * src/file_io.c
 *
 * Core file I/O functions: open, read, write, lseek, close
 * Based on UNIX file I/O semantics.
 */

 #include "file_io.h"
 #include "file_internal.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 #include <errno.h>
 #include <time.h>
 
 /* =============================================================
  * Global state
  * ============================================================= */
 file_entry_t      file_table[FILE_TABLE_SIZE];
 vnode_t           vnode_cache[VNODE_CACHE_SIZE];
 disk_inode_t      inode_table[MAX_INODES];
 uint8_t           disk_blocks[MAX_BLOCKS][BLOCK_SIZE];
 proc_fd_table_t  *current_fd_table = NULL;
 
 file_stats_t      file_stats;
 int               file_errno = 0;
 
 /* Default process fd table for demo */
 static proc_fd_table_t default_fd_table;
 
 /* =============================================================
  * fileio_init
  * ============================================================= */
 int fileio_init(void)
 {
     memset(file_table,   0, sizeof(file_table));
     memset(vnode_cache,  0, sizeof(vnode_cache));
     memset(inode_table,  0, sizeof(inode_table));
     memset(disk_blocks,  0, sizeof(disk_blocks));
     memset(&file_stats,  0, sizeof(file_stats));
     memset(&default_fd_table, 0, sizeof(default_fd_table));
     
     /* Set up default fd table */
     current_fd_table = &default_fd_table;
     for (int i = 0; i < OPEN_MAX; i++)
         current_fd_table->pft_fds[i].fde_file = NULL;
         
     /* Create root inode */
     inode_table[1].di_mode  = S_IFDIR | 0755;
     inode_table[1].di_nlink = 2;
     inode_table[1].di_uid   = 0;
     inode_table[1].di_gid   = 0;
     inode_table[1].di_size  = BLOCK_SIZE;
     inode_table[1].di_atime = time(NULL);
     inode_table[1].di_mtime = time(NULL);
     inode_table[1].di_ctime = time(NULL);
     
     printf("[fileio] init: %d file entries, %d vnodes, %d inodes\n",
            FILE_TABLE_SIZE, VNODE_CACHE_SIZE, MAX_INODES);
     return 0;
 }
 
 /* =============================================================
  * file_open — Algorithm open
  * ============================================================= */
 int file_open(const char *path, int oflag, mode_t mode)
 {
     if (!path) {
         file_errno = ENOENT;
         return -1;
     }
     
     file_stats.fs_open_calls++;
     
     printf("[open] path='%s' flags=0x%x mode=0%o\n", path, oflag, mode);
     
     /* Handle /dev/fd/n special case */
     if (is_devfd_path(path)) {
         int ref_fd = devfd_extract_fd(path);
         if (ref_fd < 0 || !valid_fd(ref_fd, current_fd_table)) {
             file_errno = EBADF;
             return -1;
         }
         return file_dup(ref_fd);  /* equivalent to dup(ref_fd) */
     }
     
     /* Validate flags */
     if (!valid_flags(oflag)) {
         file_errno = EINVAL;
         return -1;
     }
     
     /* Look up existing file */
     vnode_t *vp = path_lookup(path, current_fd_table);
     
     /* Handle O_CREAT + O_EXCL atomically */
     if ((oflag & O_CREAT) && (oflag & O_EXCL) && vp) {
         file_errno = EEXIST;
         return -1;
     }
     
     /* Create new file if needed */
     if (!vp && (oflag & O_CREAT)) {
         if (vnode_create(path, mode) != 0) {
             file_errno = ENOSPC;
             return -1;
         }
         vp = path_lookup(path, current_fd_table);
     }
     
     if (!vp) {
         file_errno = ENOENT;
         return -1;
     }
     
     /* Check permissions */
     if (!file_access_ok(vp, oflag & O_ACCMODE, 0, 0)) {
         file_errno = EACCES;
         return -1;
     }
     
     /* Truncate if requested */
     if (oflag & O_TRUNC) {
         vp->v_size = 0;
         vp->v_mtime = vp->v_ctime = time(NULL);
     }
     
     /* Allocate file table entry */
     file_entry_t *fe = file_table_alloc();
     if (!fe) {
         file_errno = ENFILE;
         return -1;
     }
     
     fe->fe_flags    = oflag;
     fe->fe_offset   = 0;
     fe->fe_ino      = vp->v_ino;
     fe->fe_refcount = 1;
     fe->fe_active   = true;
     
     /* Allocate process fd */
     int fd = proc_fd_alloc(current_fd_table, fe);
     if (fd < 0) {
         file_table_free(fe);
         file_errno = EMFILE;
         return -1;
     }
     
     file_stats.fs_open_files++;
     if (fd > (int)file_stats.fs_max_fd_used)
         file_stats.fs_max_fd_used = (uint32_t)fd;
         
     printf("[open] success: fd=%d ino=%u offset=%ld\n",
            fd, fe->fe_ino, (long)fe->fe_offset);
     return fd;
 }
 
 /* =============================================================
  * file_creat — equivalent to open with specific flags
  * ============================================================= */
 int file_creat(const char *path, mode_t mode)
 {
     return file_open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
 }
 
 /* =============================================================
  * file_read — Algorithm read
  * ============================================================= */
 ssize_t file_read(int fd, void *buf, size_t nbytes)
 {
     if (!buf || !valid_fd(fd, current_fd_table)) {
         file_errno = EBADF;
         return -1;
     }
     
     file_stats.fs_read_calls++;
     
     fd_entry_t *fde = &current_fd_table->pft_fds[fd];
     file_entry_t *fe = fde->fde_file;
     
     /* Check read permission */
     if ((fe->fe_flags & O_ACCMODE) == O_WRONLY) {
         file_errno = EBADF;
         return -1;
     }
     
     vnode_t *vp = vnode_get(fe->fe_ino);
     if (!vp) {
         file_errno = EBADF;
         return -1;
     }
     
     /* Handle EOF */
     if (fe->fe_offset >= (off_t)vp->v_size) {
         return 0;  /* EOF */
     }
     
     /* Limit read to file size */
     if (fe->fe_offset + (off_t)nbytes > (off_t)vp->v_size)
         nbytes = vp->v_size - (size_t)fe->fe_offset;
     
     /* Read file data */
     ssize_t nread = file_data_read(vp, buf, nbytes, fe->fe_offset);
     if (nread > 0) {
         fe->fe_offset += nread;
         vp->v_atime = time(NULL);
         file_stats.fs_bytes_read += (uint64_t)nread;
     }
     
     printf("[read] fd=%d nbytes=%zu nread=%zd offset=%ld\n",
            fd, nbytes, nread, (long)fe->fe_offset);
     return nread;
 }
 
 /* =============================================================
  * file_write — Algorithm write  
  * ============================================================= */
 ssize_t file_write(int fd, const void *buf, size_t nbytes)
 {
     if (!buf || !valid_fd(fd, current_fd_table)) {
         file_errno = EBADF;
         return -1;
     }
     
     file_stats.fs_write_calls++;
     
     fd_entry_t *fde = &current_fd_table->pft_fds[fd];
     file_entry_t *fe = fde->fde_file;
     
     /* Check write permission */
     if ((fe->fe_flags & O_ACCMODE) == O_RDONLY) {
         file_errno = EBADF;
         return -1;
     }
     
     vnode_t *vp = vnode_get(fe->fe_ino);
     if (!vp) {
         file_errno = EBADF;
         return -1;
     }
     
     /* Handle O_APPEND — position to EOF before each write */
     if (fe->fe_flags & O_APPEND) {
         fe->fe_offset = (off_t)vp->v_size;
     }
     
     /* Write file data */
     ssize_t nwritten = file_data_write(vp, buf, nbytes, fe->fe_offset);
     if (nwritten > 0) {
         fe->fe_offset += nwritten;
         
         /* Extend file size if we wrote past EOF */
         if (fe->fe_offset > (off_t)vp->v_size) {
             vp->v_size = (uint32_t)fe->fe_offset;
         }
         
         vp->v_mtime = vp->v_ctime = time(NULL);
         file_stats.fs_bytes_written += (uint64_t)nwritten;
         
         /* Sync immediately if O_SYNC set */
         if (fe->fe_flags & O_SYNC) {
             file_fsync(fd);
         }
     }
     
     printf("[write] fd=%d nbytes=%zu nwritten=%zd offset=%ld size=%u\n",
            fd, nbytes, nwritten, (long)fe->fe_offset, vp->v_size);
     return nwritten;
 }
 
 /* =============================================================
  * file_lseek — Algorithm lseek
  * ============================================================= */
 off_t file_lseek(int fd, off_t offset, int whence)
 {
     if (!valid_fd(fd, current_fd_table)) {
         file_errno = EBADF;
         return -1;
     }
     
     file_stats.fs_lseek_calls++;
     
     fd_entry_t *fde = &current_fd_table->pft_fds[fd];
     file_entry_t *fe = fde->fde_file;
     
     vnode_t *vp = vnode_get(fe->fe_ino);
     if (!vp) {
         file_errno = EBADF;
         return -1;
     }
     
     /* Check if file supports seeking (pipes, FIFOs, sockets don't) */
     if (S_ISFIFO(vp->v_mode) || S_ISSOCK(vp->v_mode)) {
         file_errno = ESPIPE;
         return -1;
     }
     
     off_t new_offset;
     
     switch (whence) {
     case SEEK_SET:
         new_offset = offset;
         break;
     case SEEK_CUR:
         new_offset = fe->fe_offset + offset;
         break;
     case SEEK_END:
         new_offset = (off_t)vp->v_size + offset;
         break;
     default:
         file_errno = EINVAL;
         return -1;
     }
     
     /* Allow seeking past EOF (creates holes) */
     if (new_offset < 0) {
         file_errno = EINVAL;
         return -1;
     }
     
     off_t old_offset = fe->fe_offset;
     fe->fe_offset = new_offset;
     
     printf("[lseek] fd=%d whence=%d offset=%ld → new_offset=%ld\n",
            fd, whence, (long)offset, (long)new_offset);
     printf("[lseek] old=%ld new=%ld size=%u\n",
            (long)old_offset, (long)new_offset, vp->v_size);
     
     return new_offset;
 }
 
 /* =============================================================
  * file_close — Algorithm close
  * ============================================================= */
 int file_close(int fd)
 {
     if (!valid_fd(fd, current_fd_table)) {
         file_errno = EBADF;
         return -1;
     }
     
     file_stats.fs_close_calls++;
     
     fd_entry_t *fde = &current_fd_table->pft_fds[fd];
     file_entry_t *fe = fde->fde_file;
     
     printf("[close] fd=%d ino=%u refcount=%d\n",
            fd, fe->fe_ino, fe->fe_refcount);
     
     /* Decrement file table reference count */
     fe->fe_refcount--;
     if (fe->fe_refcount == 0) {
         /* Last reference — release file table entry */
         file_table_free(fe);
         file_stats.fs_open_files--;
     }
     
     /* Free process fd entry */
     proc_fd_free(current_fd_table, fd);
     
     return 0;
 }
 
 /* =============================================================
  * file_pread — atomic seek + read
  * ============================================================= */
 ssize_t file_pread(int fd, void *buf, size_t nbytes, off_t offset)
 {
     if (!valid_fd(fd, current_fd_table)) {
         file_errno = EBADF;
         return -1;
     }
     
     vnode_t *vp = vnode_get(current_fd_table->pft_fds[fd].fde_file->fe_ino);
     if (!vp) {
         file_errno = EBADF;
         return -1;
     }
     
     /* Check seekability */
     if (S_ISFIFO(vp->v_mode)) {
         file_errno = ESPIPE;
         return -1;
     }
     
     /* Read at specified offset without updating file pointer */
     ssize_t nread = file_data_read(vp, buf, nbytes, offset);
     if (nread > 0) {
         vp->v_atime = time(NULL);
         file_stats.fs_bytes_read += (uint64_t)nread;
     }
     
     printf("[pread] fd=%d offset=%ld nbytes=%zu nread=%zd\n",
            fd, (long)offset, nbytes, nread);
     return nread;
 }
 
 /* =============================================================
  * file_pwrite — atomic seek + write
  * ============================================================= */
 ssize_t file_pwrite(int fd, const void *buf, size_t nbytes, off_t offset)
 {
     if (!valid_fd(fd, current_fd_table)) {
         file_errno = EBADF;
         return -1;
     }
     
     vnode_t *vp = vnode_get(current_fd_table->pft_fds[fd].fde_file->fe_ino);
     if (!vp) {
         file_errno = EBADF;
         return -1;
     }
     
     /* Check seekability */
     if (S_ISFIFO(vp->v_mode)) {
         file_errno = ESPIPE;
         return -1;
     }
     
     /* Write at specified offset without updating file pointer */
     ssize_t nwritten = file_data_write(vp, buf, nbytes, offset);
     if (nwritten > 0) {
         /* Update file size if we extended it */
         if (offset + nwritten > (off_t)vp->v_size) {
             vp->v_size = (uint32_t)(offset + nwritten);
         }
         
         vp->v_mtime = vp->v_ctime = time(NULL);
         file_stats.fs_bytes_written += (uint64_t)nwritten;
     }
     
     printf("[pwrite] fd=%d offset=%ld nbytes=%zu nwritten=%zd\n",
            fd, (long)offset, nbytes, nwritten);
     return nwritten;
 }
 
 /* =============================================================
  * file_openat — open relative to directory fd
  * ============================================================= */
 int file_openat(int fd, const char *path, int oflag, mode_t mode)
 {
     /* If path is absolute, ignore fd and behave like open */
     if (path[0] == '/') {
         return file_open(path, oflag, mode);
     }
     
     /* If fd is AT_FDCWD, use current directory */
     if (fd == AT_FDCWD) {
         /* Prepend current working directory */
         char full_path[PATH_MAX];
         snprintf(full_path, sizeof(full_path), "/%s", path);
         return file_open(full_path, oflag, mode);
     }
     
     /* Validate directory fd */
     if (!valid_fd(fd, current_fd_table)) {
         file_errno = EBADF;
         return -1;
     }
     
     vnode_t *dir_vp = vnode_get(current_fd_table->pft_fds[fd].fde_file->fe_ino);
     if (!dir_vp || !S_ISDIR(dir_vp->v_mode)) {
         file_errno = ENOTDIR;
         return -1;
     }
     
     /* Build full path from directory + relative path */
     char full_path[PATH_MAX];
     snprintf(full_path, sizeof(full_path), "/dir%u/%s", 
              dir_vp->v_ino, path);
     
     return file_open(full_path, oflag, mode);
 }
 