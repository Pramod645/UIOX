/*
 * src/file_ops.c
 *
 * File descriptor manipulation: dup, dup2, fcntl
 * Synchronization: sync, fsync, fdatasync
 */

 #include "file_io.h"
 #include "file_internal.h"
 #include <stdio.h>
 #include <string.h>
 #include <unistd.h>
 #include <errno.h>
 #include <time.h>
 
 /* =============================================================
  * file_dup — duplicate file descriptor (lowest available)
  * ============================================================= */
 int file_dup(int fd)
 {
     if (!valid_fd(fd, current_fd_table)) {
         file_errno = EBADF;
         return -1;
     }
     
     fd_entry_t *src_fde = &current_fd_table->pft_fds[fd];
     file_entry_t *fe = src_fde->fde_file;
     
     /* Find lowest available fd */
     int new_fd = proc_fd_lowest_available(current_fd_table);
     if (new_fd < 0) {
         file_errno = EMFILE;
         return -1;
     }
     
     /* Share the same file table entry */
     current_fd_table->pft_fds[new_fd].fde_file = fe;
     current_fd_table->pft_fds[new_fd].fde_flags = 0; /* clear FD_CLOEXEC */
     fe->fe_refcount++;
     
     if (new_fd > current_fd_table->pft_max_fd)
         current_fd_table->pft_max_fd = new_fd;
     
     printf("[dup] fd=%d → new_fd=%d (refcount=%d)\n",
            fd, new_fd, fe->fe_refcount);
     return new_fd;
 }
 
 /* =============================================================
  * file_dup2 — duplicate to specific fd number
  * ============================================================= */
 int file_dup2(int fd, int fd2)
 {
     if (!valid_fd(fd, current_fd_table) || fd2 < 0 || fd2 >= OPEN_MAX) {
         file_errno = EBADF;
         return -1;
     }
     
     /* If fd == fd2, return fd2 without closing it */
     if (fd == fd2) {
         return fd2;
     }
     
     /* Close fd2 if it's already open */
     if (current_fd_table->pft_fds[fd2].fde_file) {
         file_close(fd2);
     }
     
     /* Copy fd to fd2 */
     fd_entry_t *src_fde = &current_fd_table->pft_fds[fd];
     file_entry_t *fe = src_fde->fde_file;
     
     current_fd_table->pft_fds[fd2].fde_file = fe;
     current_fd_table->pft_fds[fd2].fde_flags = 0;  /* clear FD_CLOEXEC */
     fe->fe_refcount++;
     
     if (fd2 > current_fd_table->pft_max_fd)
         current_fd_table->pft_max_fd = fd2;
     
     printf("[dup2] fd=%d → fd2=%d (refcount=%d)\n",
            fd, fd2, fe->fe_refcount);
     return fd2;
 }
 
 /* =============================================================
  * file_fcntl — file control operations
  * ============================================================= */
 int file_fcntl(int fd, int cmd, long arg)
 {
     if (!valid_fd(fd, current_fd_table)) {
         file_errno = EBADF;
         return -1;
     }
     
     fd_entry_t *fde = &current_fd_table->pft_fds[fd];
     file_entry_t *fe = fde->fde_file;
     
     printf("[fcntl] fd=%d cmd=%d arg=%ld\n", fd, cmd, arg);
     
     switch (cmd) {
     case F_DUPFD:
         {
             /* Find lowest available fd >= arg */
             int start = (int)arg;
             if (start < 0) start = 0;
             
             for (int i = start; i < OPEN_MAX; i++) {
                 if (!current_fd_table->pft_fds[i].fde_file) {
                     current_fd_table->pft_fds[i].fde_file = fe;
                     current_fd_table->pft_fds[i].fde_flags = 0;
                     fe->fe_refcount++;
                     if (i > current_fd_table->pft_max_fd)
                         current_fd_table->pft_max_fd = i;
                     printf("[fcntl] F_DUPFD → fd=%d\n", i);
                     return i;
                 }
             }
             file_errno = EMFILE;
             return -1;
         }
         
     case F_DUPFD_CLOEXEC:
         {
             int new_fd = file_fcntl(fd, F_DUPFD, arg);
             if (new_fd >= 0) {
                 current_fd_table->pft_fds[new_fd].fde_flags |= FD_CLOEXEC;
             }
             return new_fd;
         }
         
     case F_GETFD:
         return fde->fde_flags;
         
     case F_SETFD:
         fde->fde_flags = (int)arg;
         return 0;
         
     case F_GETFL:
         return fe->fe_flags;
         
     case F_SETFL:
         /* Only certain flags can be changed */
         fe->fe_flags = (fe->fe_flags & ~(O_APPEND | O_NONBLOCK | O_SYNC | 
                                          O_DSYNC | O_RSYNC | O_ASYNC)) |
                        ((int)arg & (O_APPEND | O_NONBLOCK | O_SYNC | 
                                    O_DSYNC | O_RSYNC | O_ASYNC));
         return 0;
         
     case F_GETOWN:
         return 0;  /* stub */
         
     case F_SETOWN:
         return 0;  /* stub */
         
     default:
         file_errno = EINVAL;
         return -1;
     }
 }
 
 /* =============================================================
  * file_sync — flush all modified buffers  
  * ============================================================= */
 void file_sync(void)
 {
     printf("[sync] flushing all modified buffers\n");
     /* In a real system: iterate through buffer cache, write dirty blocks */
     
     for (int i = 0; i < FILE_TABLE_SIZE; i++) {
         if (file_table[i].fe_active) {
             vnode_t *vp = vnode_get(file_table[i].fe_ino);
             if (vp) {
                 /* Write back any cached data */
                 printf("  [sync] writing inode %u\n", vp->v_ino);
             }
         }
     }
 }
 
 /* =============================================================
  * file_fsync — flush specific file
  * ============================================================= */
 int file_fsync(int fd)
 {
     if (!valid_fd(fd, current_fd_table)) {
         file_errno = EBADF;
         return -1;
     }
     
     file_entry_t *fe = current_fd_table->pft_fds[fd].fde_file;
     vnode_t *vp = vnode_get(fe->fe_ino);
     if (!vp) {
         file_errno = EBADF;
         return -1;
     }
     
     printf("[fsync] fd=%d ino=%u — wait for disk writes\n", fd, vp->v_ino);
     
     /* In a real system: 
      * 1. Find all dirty buffers for this inode
      * 2. Write them to disk synchronously
      * 3. Update inode on disk
      * 4. Wait for all I/O to complete
      */
     
     /* Update access time */
     vp->v_atime = time(NULL);
     
     return 0;
 }
 
 /* =============================================================
  * file_fdatasync — flush file data (not attributes)
  * ============================================================= */
 int file_fdatasync(int fd)
 {
     if (!valid_fd(fd, current_fd_table)) {
         file_errno = EBADF;
         return -1;
     }
     
     file_entry_t *fe = current_fd_table->pft_fds[fd].fde_file;
     vnode_t *vp = vnode_get(fe->fe_ino);
     if (!vp) {
         file_errno = EBADF;
         return -1;
     }
     
     printf("[fdatasync] fd=%d ino=%u — flush data only\n", fd, vp->v_ino);
     
     /* Similar to fsync but skip attribute updates unless they
      * affect data readability (e.g., file size) */
     
     return 0;
 }
 
 /* =============================================================
  * Helper functions for flag manipulation
  * ============================================================= */
 void set_fl(int fd, int flags)
 {
     int val = file_fcntl(fd, F_GETFL, 0);
     if (val < 0) {
         printf("[set_fl] fcntl F_GETFL error\n");
         return;
     }
     
     val |= flags;
     if (file_fcntl(fd, F_SETFL, val) < 0) {
         printf("[set_fl] fcntl F_SETFL error\n");
     }
 }
 
 void clr_fl(int fd, int flags)
 {
     int val = file_fcntl(fd, F_GETFL, 0);
     if (val < 0) {
         printf("[clr_fl] fcntl F_GETFL error\n");
         return;
     }
     
     val &= ~flags;
     if (file_fcntl(fd, F_SETFL, val) < 0) {
         printf("[clr_fl] fcntl F_SETFL error\n");
     }
 }
 