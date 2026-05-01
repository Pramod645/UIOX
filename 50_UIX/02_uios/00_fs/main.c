/*
 * main.c — File I/O system demonstration
 *
 * Exercises all files functions 
 *
 *   Core:        open, read, write, lseek, close, creat
 *   Atomic:      pread, pwrite, O_APPEND behavior
 *   Descriptor:  dup, dup2, fcntl (F_DUPFD, F_GETFL, F_SETFL)
 *   Sync:        sync, fsync, fdatasync
 *   Special:     openat, /dev/fd, ioctl
 *   Sharing:     multiple processes with same file open
 *   Holes:       sparse files (lseek past EOF + write)
 */

 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include "file_io.h"
 
 static void banner(const char *s)
 {
     printf("\n══════════════════════════════════════════\n");
     printf("  %s\n", s);
     printf("════════════════════════════════════════════\n");
 }
 
 /* Test data */
 static const char *test_data1 = "Hello, World!";
 static const char *test_data2 = "UNOIX File I/O";
 
 int main(void)
 {
     printf("╔══════════════════════════════════════════╗\n");
     printf("║  Uiox File I/O System Test               ║\n");
     printf("║  Based on file system                    ║\n");
     printf("╚══════════════════════════════════════════╝\n");
     
     /* Initialize */
     fileio_init();
     
     /* ==========================================================
      * Core Functions: open, read, write, lseek, close
      * ========================================================== */
     banner("Core Functions — open, read, write, lseek, close");
     
     /* Create and write to a test file */
     int fd1 = file_creat("/tmp/test.txt", 0644);
     printf("[main] created test file: fd=%d\n", fd1);
     
     ssize_t n = file_write(fd1, test_data1, strlen(test_data1));
     printf("[main] wrote %zd bytes\n", n);
     
     /* Seek back to beginning and read */
     off_t pos = file_lseek(fd1, 0, SEEK_SET);
     printf("[main] seeked to position %ld\n", (long)pos);
     
     char read_buf[64] = {0};
     n = file_read(fd1, read_buf, sizeof(read_buf) - 1);
     read_buf[n] = '\0';
     printf("[main] read %zd bytes: '%s'\n", n, read_buf);
     
     file_close(fd1);
     
     /* ==========================================================
      * File Holes — seek past EOF and write
      * ========================================================== */
     banner("File Holes — lseek past EOF");
     
     int fd_hole = file_creat("/tmp/hole.txt", 0644);
     file_write(fd_hole, "start", 5);
     
     /* Create hole by seeking way past EOF */
     file_lseek(fd_hole, 16384, SEEK_SET);
     file_write(fd_hole, "end", 3);
     
     /* Verify hole contents */
     file_lseek(fd_hole, 0, SEEK_SET);
     memset(read_buf, 'X', sizeof(read_buf));
     n = file_read(fd_hole, read_buf, 20);
     printf("[main] hole file first 20 bytes:\n");
     for (int i = 0; i < n; i++) {
         printf(" %02x", (unsigned char)read_buf[i]);
     }
     printf("\n");
     
     file_close(fd_hole);
     
     /* ==========================================================
      * dup and dup2 — file descriptor sharing
      * ========================================================== */
     banner("File Descriptor Manipulation — dup, dup2");
     
     int fd2 = file_open("/tmp/test.txt", O_RDWR, 0);
     printf("[main] opened for read/write: fd=%d\n", fd2);
     
     int fd3 = file_dup(fd2);
     printf("[main] dup(%d) → %d\n", fd2, fd3);
     
     int fd5 = file_dup2(fd2, 5);
     printf("[main] dup2(%d, 5) → %d\n", fd2, fd5);
     
     /* Both fd2 and fd3 share the same file table entry */
     file_write(fd2, "shared", 6);
     off_t offset2 = file_lseek(fd3, 0, SEEK_CUR);
     printf("[main] after write on fd2, fd3 offset = %ld\n", (long)offset2);
     
     /* ==========================================================
      * fcntl — file control operations
      * ========================================================== */
     banner("fcntl — File Control Operations");
     
     /* Get file flags */
     int flags = file_fcntl(fd2, F_GETFL, 0);
     printf("[main] fd=%d flags=0x%x ", fd2, flags);
     switch (flags & O_ACCMODE) {
     case O_RDONLY: printf("(read-only)"); break;
     case O_WRONLY: printf("(write-only)"); break;
     case O_RDWR:   printf("(read-write)"); break;
     }
     if (flags & O_APPEND)   printf(" append");
     if (flags & O_NONBLOCK) printf(" nonblock");
     if (flags & O_SYNC)     printf(" sync");
     printf("\n");
     
     /* Set O_APPEND flag */
     set_fl(fd2, O_APPEND);
     flags = file_fcntl(fd2, F_GETFL, 0);
     printf("[main] after set_fl(O_APPEND): flags=0x%x\n", flags);
     
     /* Test append behavior */
     file_write(fd2, " appended", 9);
     file_lseek(fd2, 0, SEEK_SET);
     file_write(fd2, " more", 5);  /* should append despite lseek */
     
     /* ==========================================================
      * Atomic I/O — pread, pwrite
      * ========================================================== */
     banner("Atomic I/O — pread, pwrite");
     
     /* pread does not change file offset */
     memset(read_buf, 0, sizeof(read_buf));
     n = file_pread(fd2, read_buf, 10, 0);
     read_buf[n] = '\0';
     printf("[main] pread(fd2, buf, 10, 0) → '%s'\n", read_buf);
     
     off_t curr_pos = file_lseek(fd2, 0, SEEK_CUR);
     printf("[main] file offset after pread: %ld (should be unchanged)\n",
            (long)curr_pos);
     
     /* pwrite does not change file offset */
     file_pwrite(fd2, "[PWRITE]", 8, 50);
     curr_pos = file_lseek(fd2, 0, SEEK_CUR);
     printf("[main] file offset after pwrite: %ld (should be unchanged)\n",
            (long)curr_pos);
     
     /* ==========================================================
      * Synchronization — sync, fsync, fdatasync
      * ========================================================== */
     banner("Synchronization — sync, fsync, fdatasync");
     
     file_sync();
     file_fsync(fd2);
     file_fdatasync(fd2);
     
     /* ==========================================================
      * /dev/fd support
      * ========================================================== */
     banner("/dev/fd Support");
     
     int fd_stdin = file_open("/dev/stdin", O_RDONLY, 0);
     printf("[main] /dev/stdin → fd=%d\n", fd_stdin);
     
     int fd_dup = file_open("/dev/fd/2", O_RDWR, 0);
     printf("[main] /dev/fd/2 → fd=%d (should dup fd 2)\n", fd_dup);
     
     if (fd_stdin >= 0) file_close(fd_stdin);
     if (fd_dup >= 0) file_close(fd_dup);
     
     /* ==========================================================
      * ioctl operations
      * ========================================================== */
     banner("ioctl Operations");
     
     file_ioctl(fd2, 0x5401, NULL);  /* TCGETS */
     file_ioctl(fd2, 0x12345, NULL); /* unknown command */
     
     /* ==========================================================
      * Cleanup and final state
      * ========================================================== */
     banner("Cleanup and Final State");
     
     file_close(fd2);
     file_close(fd3);
     file_close(fd5);
     
     fileio_print_tables();
     fileio_print_stats();
     
     printf("\n[main] File I/O system test complete\n");
     return 0;
 }
 