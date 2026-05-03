/*
 * main.c — demonstration of every function in files_dirs.c
 *
 * Exercises in the same banner-driven style.
 */

 #include "files_dirs.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <fcntl.h>
 
 static void banner(const char *s)
 {
     printf("\n══════════════════════════════════════════\n");
     printf("  %s\n", s);
     printf("══════════════════════════════════════════\n");
 }
 
 int main(void)
 {
     struct stat sb;
     char        buf[PATH_MAX + 1];
 
     /* =======================================================
      * §4.2  stat / lstat / fstatat
      * ======================================================= */
     banner("§4.2  stat / lstat / fstat");
 
     if (fd_stat("/etc/passwd", &sb) == 0)
         print_stat("/etc/passwd", &sb);
 
     if (fd_lstat("/etc/passwd", &sb) == 0)
         printf("lstat type: %s\n", filetype_str(sb.st_mode));
 
     /* fstatat with AT_FDCWD behaves like stat */
     if (fd_fstatat(AT_FDCWD, "/etc", &sb, 0) == 0)
         printf("fstatat /etc type: %s\n", filetype_str(sb.st_mode));
 
     /* =======================================================
      * §4.3  File types
      * ======================================================= */
     banner("§4.3  File Types");
 
     const char *paths[] = {
         "/etc/passwd",
         "/etc",
         "/dev/tty",
         "/tmp",
         NULL
     };
 
     for (int i = 0; paths[i]; i++) {
         if (lstat(paths[i], &sb) == 0) {
             char mstr[11];
             mode_to_string(sb.st_mode, mstr);
             printf("%-20s  %s  (%s)\n",
                    paths[i], mstr, filetype_str(sb.st_mode));
         }
     }
 
     /* =======================================================
      * §4.5  Permission check
      * ======================================================= */
     banner("§4.5  check_permission");
 
     if (fd_stat("/etc/passwd", &sb) == 0) {
         bool can_r = check_permission(&sb, getuid(), getgid(), R_OK);
         bool can_w = check_permission(&sb, getuid(), getgid(), W_OK);
         printf("/etc/passwd: euid=%u can_read=%s can_write=%s\n",
                (unsigned)getuid(),
                can_r ? "yes" : "no",
                can_w ? "yes" : "no");
     }
 
     /* =======================================================
      * §4.7  access
      * ======================================================= */
     banner("§4.7  access / faccessat");
 
     if (check_access("/etc/passwd", R_OK) == 0)
         printf("/etc/passwd: read access OK\n");
 
     if (check_faccessat(AT_FDCWD, "/etc/passwd", R_OK, 0) == 0)
         printf("faccessat /etc/passwd: read access OK\n");
 
     /* =======================================================
      * §4.8  umask
      * ======================================================= */
     banner("§4.8  umask");
 
     print_umask();
 
     mode_t old = set_umask(0);
     printf("set umask(0): previous was %04o\n", (unsigned)old);
 
     /* Create a file with umask=0 — all permission bits honoured */
     int fd_foo = open("/tmp/c_foo.txt",
                       O_WRONLY | O_CREAT | O_TRUNC, RWRWRW);
     if (fd_foo >= 0) {
         close(fd_foo);
         if (fd_stat("/tmp/c_foo.txt", &sb) == 0) {
             char ms[11]; mode_to_string(sb.st_mode, ms);
             printf("foo mode (umask=0): %s\n", ms);
         }
     }
 
     set_umask(S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
     int fd_bar = open("/tmp/c_bar.txt",
                       O_WRONLY | O_CREAT | O_TRUNC, RWRWRW);
     if (fd_bar >= 0) {
         close(fd_bar);
         if (fd_stat("/tmp/ch4_bar.txt", &sb) == 0) {
             char ms[11]; mode_to_string(sb.st_mode, ms);
             printf("bar mode (umask=077): %s\n", ms);
         }
     }
 
     set_umask(old);   /* restore */
 
     /* =======================================================
      * §4.9  chmod
      * ======================================================= */
     banner("§4.9  chmod / fchmod");
 
     /* foo: turn on SGID, turn off group-execute */
     if (fd_stat("/tmp/c_foo.txt", &sb) == 0) {
         mode_t newmode = (sb.st_mode & ~S_IXGRP) | S_ISGID;
         fd_chmod("/tmp/c_foo.txt", newmode);
         fd_stat("/tmp/c_foo.txt", &sb);
         char ms[11]; mode_to_string(sb.st_mode, ms);
         printf("foo after chmod (SGID on, GX off): %s\n", ms);
     }
 
     /* bar: set absolute rw-r--r-- */
     fd_chmod("/tmp/c_bar.txt",
              S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
     fd_stat("/tmp/c_bar.txt", &sb);
     {
         char ms[11]; mode_to_string(sb.st_mode, ms);
         printf("bar after chmod (644): %s\n", ms);
     }
 
     /* =======================================================
      * §4.12  File size / holes
      * ======================================================= */
     banner("§4.12  File size and holes");
 
     create_file_with_hole("/tmp/c_hole.txt", 16384,
                           "abcdefghij", 10,
                           "ABCDEFGHIJ", 10);
     report_file_size("/tmp/c_hole.txt");
 
     /* =======================================================
      * §4.13  truncate / ftruncate
      * ======================================================= */
     banner("§4.13  truncate / ftruncate");
 
     fd_truncate("/tmp/c_foo.txt", 0);
     fd_stat("/tmp/c_foo.txt", &sb);
     printf("foo after truncate(0): size=%lld\n",
            (long long)sb.st_size);
 
     int tfd = open("/tmp/c_bar.txt", O_RDWR);
     if (tfd >= 0) {
         fd_ftruncate(tfd, 100);
         close(tfd);
         fd_stat("/tmp/c_bar.txt", &sb);
         printf("bar after ftruncate(100): size=%lld\n",
                (long long)sb.st_size);
     }
 
     /* =======================================================
      * §4.15  link / unlink / open_then_unlink
      * ======================================================= */
     banner("§4.15  link / unlink / remove");
 
     fd_link("/tmp/c_bar.txt", "/tmp/c_bar_hard.txt");
     fd_stat("/tmp/c_bar.txt", &sb);
     printf("bar nlink after hard link: %lu\n",
            (unsigned long)sb.st_nlink);
 
     fd_unlink("/tmp/c_bar_hard.txt");
     fd_stat("/tmp/c_bar.txt", &sb);
     printf("bar nlink after unlink: %lu\n",
            (unsigned long)sb.st_nlink);
 
     /* Create a temp file and immediately unlink it */
     int tmp_fd = open("/tmp/c_temp.txt",
                       O_RDWR | O_CREAT | O_TRUNC, FILE_MODE);
     if (tmp_fd >= 0) {
         close(tmp_fd);
         int ufd = open_then_unlink("/tmp/c_temp.txt");
         if (ufd >= 0) {
             write(ufd, "data in unlinked file\n", 22);
             lseek(ufd, 0, SEEK_SET);
             ssize_t n = read(ufd, buf, sizeof(buf) - 1);
             if (n > 0) { buf[n] = '\0'; printf("read back: %s", buf); }
             close(ufd);   /* file now truly gone */
         }
     }
 
     /* =======================================================
      * §4.16  rename
      * ======================================================= */
     banner("§4.16  rename");
 
     fd_rename("/tmp/c_foo.txt", "/tmp/c_foo_renamed.txt");
     printf("renamed foo → foo_renamed\n");
     fd_rename("/tmp/c_foo_renamed.txt", "/tmp/c_foo.txt");
 
     /* =======================================================
      * §4.18  symlink / readlink
      * ======================================================= */
     banner("§4.18  symlink / readlink");
 
     unlink("/tmp/c_sym.txt");  /* remove old if it exists */
     fd_symlink("/tmp/c_bar.txt", "/tmp/c_sym.txt");
 
     ssize_t rn = fd_readlink("/tmp/c_sym.txt", buf, sizeof(buf));
     if (rn > 0)
         printf("readlink(/tmp/c_sym.txt) → '%s'\n", buf);
 
     /* lstat sees the link itself; stat follows it */
     if (fd_lstat("/tmp/c_sym.txt", &sb) == 0)
         printf("lstat type of symlink: %s  size=%lld\n",
                filetype_str(sb.st_mode), (long long)sb.st_size);
 
     /* =======================================================
      * §4.20  futimens / trunc_preserve_times — Figure 4.21
      * ======================================================= */
     banner("§4.20  futimens / trunc_preserve_times");
 
     trunc_preserve_times("/tmp/c_bar.txt");
 
     /* Also demonstrate explicit futimens */
     int tfd2 = open("/tmp/c_bar.txt", O_RDWR);
     if (tfd2 >= 0) {
         struct timespec ts[2];
         ts[0].tv_sec  = 1000000000;   /* set to a specific time */
         ts[0].tv_nsec = 0;
         ts[1].tv_sec  = 1000000000;
         ts[1].tv_nsec = 0;
         fd_futimens(tfd2, ts);
         fd_fstat(tfd2, &sb);
         printf("bar mtime after futimens: %ld\n",
                (long)sb.st_mtim.tv_sec);
         close(tfd2);
     }
 
     /* =======================================================
      * §4.21  mkdir / rmdir
      * ======================================================= */
     banner("§4.21  mkdir / rmdir");
 
     fd_mkdir("/tmp/c_testdir", DIR_MODE);
     fd_stat("/tmp/c_testdir", &sb);
     printf("created /tmp/c_testdir: type=%s  nlink=%lu\n",
            filetype_str(sb.st_mode),
            (unsigned long)sb.st_nlink);
 
     fd_rmdir("/tmp/c_testdir");
     printf("removed /tmp/c_testdir\n");
 
     /* =======================================================
      * §4.22  myftw — directory walk
      * ======================================================= */
     banner("§4.22  myftw — file type count for /dev");
 
     myftw("/dev", count_filetypes);
     print_filetype_counts();
 
     /* =======================================================
      * §4.23  chdir / getcwd — Figure 4.23/4.24
      * ======================================================= */
     banner("§4.23  chdir / getcwd");
 
     char *cwd = getcwd_alloc();
     if (cwd) {
         printf("initial cwd: %s\n", cwd);
         free(cwd);
     }
 
     fd_chdir("/tmp");
     cwd = getcwd_alloc();
     if (cwd) {
         printf("after chdir(/tmp): %s\n", cwd);
         free(cwd);
     }
 
     /* Use fchdir to return to original directory */
     int orig_fd = open(".", O_RDONLY);
     fd_chdir("/");
     printf("after chdir(/): ");
     cwd = getcwd_alloc();
     if (cwd) { printf("%s\n", cwd); free(cwd); }
 
     if (orig_fd >= 0) {
         fd_fchdir(orig_fd);
         close(orig_fd);
         cwd = getcwd_alloc();
         if (cwd) { printf("after fchdir back: %s\n", cwd); free(cwd); }
     }
 
     /* =======================================================
      * §4.24  Device numbers
      * ======================================================= */
     banner("§4.24  Device special files");
 
     print_dev_numbers("/");
     print_dev_numbers("/dev/tty");
 
     /* =======================================================
      * §4.25  Permission bit summary
      * ======================================================= */
     banner("§4.25  Permission bit summary");
 
     fd_stat("/etc/passwd", &sb);
     {
         char ms[11];
         mode_to_string(sb.st_mode, ms);
         printf("/etc/passwd mode: %s\n", ms);
         printf("  setuid=%d  setgid=%d  sticky=%d\n",
                has_setuid(sb.st_mode),
                has_setgid(sb.st_mode),
                has_sticky(sb.st_mode));
         printf("  user:  r=%d w=%d x=%d\n",
                perm_user_can_read(sb.st_mode),
                perm_user_can_write(sb.st_mode),
                perm_user_can_exec(sb.st_mode));
         printf("  group: r=%d w=%d x=%d\n",
                perm_group_can_read(sb.st_mode),
                perm_group_can_write(sb.st_mode),
                perm_group_can_exec(sb.st_mode));
         printf("  other: r=%d w=%d x=%d\n",
                perm_other_can_read(sb.st_mode),
                perm_other_can_write(sb.st_mode),
                perm_other_can_exec(sb.st_mode));
     }
 
     /* =======================================================
      * Cleanup
      * ======================================================= */
     banner("Cleanup");
 
     unlink("/tmp/c_foo.txt");
     unlink("/tmp/c_bar.txt");
     unlink("/tmp/c_hole.txt");
     unlink("/tmp/c_sym.txt");
     printf("temporary files removed\n");
 
     return 0;
 }
 