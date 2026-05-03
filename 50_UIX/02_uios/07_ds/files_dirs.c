/*
 * files_dirs.c
 *
 * Implementation of the UIOX Files and Directories interface,
 * 
 */

 #define _POSIX_C_SOURCE 200809L
 #define _XOPEN_SOURCE   700
 
 #include "files_dirs.h"
 
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <errno.h>
 #include <stdarg.h>
 #include <time.h>
 
 /* =============================================================
  * Internal counters used by count_filetypes callback (§4.22)
  * ============================================================= */
 static long cnt_reg   = 0;
 static long cnt_dir   = 0;
 static long cnt_blk   = 0;
 static long cnt_chr   = 0;
 static long cnt_fifo  = 0;
 static long cnt_lnk   = 0;
 static long cnt_sock  = 0;
 
 /* =============================================================
  * Utility helpers
  * ============================================================= */
 
 int err_msg(const char *msg)
 {
     perror(msg);
     return -1;
 }
 
 char *path_alloc(size_t *sizep)
 {
     size_t sz = PATH_MAX + 1;
     char  *p  = malloc(sz);
     if (!p) {
         perror("path_alloc: malloc");
         return NULL;
     }
     if (sizep)
         *sizep = sz;
     return p;
 }
 
 /* =============================================================
  * §4.3  File type helpers
  * ============================================================= */
 
 const char *filetype_str(mode_t mode)
 {
     if (S_ISREG(mode))  return "regular";
     if (S_ISDIR(mode))  return "directory";
     if (S_ISCHR(mode))  return "character special";
     if (S_ISBLK(mode))  return "block special";
     if (S_ISFIFO(mode)) return "fifo";
     if (S_ISLNK(mode))  return "symbolic link";
     if (S_ISSOCK(mode)) return "socket";
     return "** unknown **";
 }
 
 /* =============================================================
  * §4.25  Permission bit helpers
  * ============================================================= */
 
 void mode_to_string(mode_t mode, char out[11])
 {
     /* file type character */
     char type;
     if      (S_ISREG(mode))  type = '-';
     else if (S_ISDIR(mode))  type = 'd';
     else if (S_ISCHR(mode))  type = 'c';
     else if (S_ISBLK(mode))  type = 'b';
     else if (S_ISFIFO(mode)) type = 'p';
     else if (S_ISLNK(mode))  type = 'l';
     else if (S_ISSOCK(mode)) type = 's';
     else                     type = '?';
 
     out[0] = type;
     out[1] = (mode & S_IRUSR) ? 'r' : '-';
     out[2] = (mode & S_IWUSR) ? 'w' : '-';
 
     if (mode & S_ISUID)
         out[3] = (mode & S_IXUSR) ? 's' : 'S';
     else
         out[3] = (mode & S_IXUSR) ? 'x' : '-';
 
     out[4] = (mode & S_IRGRP) ? 'r' : '-';
     out[5] = (mode & S_IWGRP) ? 'w' : '-';
 
     if (mode & S_ISGID)
         out[6] = (mode & S_IXGRP) ? 's' : 'S';
     else
         out[6] = (mode & S_IXGRP) ? 'x' : '-';
 
     out[7] = (mode & S_IROTH) ? 'r' : '-';
     out[8] = (mode & S_IWOTH) ? 'w' : '-';
 
     if (mode & S_ISVTX)
         out[9] = (mode & S_IXOTH) ? 't' : 'T';
     else
         out[9] = (mode & S_IXOTH) ? 'x' : '-';
 
     out[10] = '\0';
 }
 
 bool perm_user_can_read  (mode_t m) { return (m & S_IRUSR) != 0; }
 bool perm_user_can_write (mode_t m) { return (m & S_IWUSR) != 0; }
 bool perm_user_can_exec  (mode_t m) { return (m & S_IXUSR) != 0; }
 bool perm_group_can_read (mode_t m) { return (m & S_IRGRP) != 0; }
 bool perm_group_can_write(mode_t m) { return (m & S_IWGRP) != 0; }
 bool perm_group_can_exec (mode_t m) { return (m & S_IXGRP) != 0; }
 bool perm_other_can_read (mode_t m) { return (m & S_IROTH) != 0; }
 bool perm_other_can_write(mode_t m) { return (m & S_IWOTH) != 0; }
 bool perm_other_can_exec (mode_t m) { return (m & S_IXOTH) != 0; }
 bool has_setuid(mode_t m)  { return (m & S_ISUID) != 0; }
 bool has_setgid(mode_t m)  { return (m & S_ISGID) != 0; }
 bool has_sticky(mode_t m)  { return (m & S_ISVTX) != 0; }
 
 /*
  * check_permission — §4.5 four-step kernel permission test.
  *
  * Step 1: superuser → always allowed.
  * Step 2: euid matches owner → check user bits only.
  * Step 3: egid matches file group → check group bits only.
  * Step 4: check other bits.
  */
 bool check_permission(const struct stat *sb,
                       uid_t euid, gid_t egid, int req_mode)
 {
     mode_t m = sb->st_mode;
     int    bits;
 
     /* Step 1: superuser */
     if (euid == 0)
         return true;
 
     if (euid == sb->st_uid) {
         /* Step 2: process owns the file — look at user bits only */
         bits = (int)((m >> 6) & 7);
     } else if (egid == sb->st_gid) {
         /* Step 3: group match — look at group bits only */
         bits = (int)((m >> 3) & 7);
     } else {
         /* Step 4: other bits */
         bits = (int)(m & 7);
     }
 
     if ((req_mode & R_OK) && !(bits & 4)) return false;
     if ((req_mode & W_OK) && !(bits & 2)) return false;
     if ((req_mode & X_OK) && !(bits & 1)) return false;
 
     return true;
 }
 
 /* =============================================================
  * §4.2  stat family
  * ============================================================= */
 
 int fd_stat(const char *path, struct stat *buf)
 {
     if (stat(path, buf) < 0)
         return err_msg("stat");
     return 0;
 }
 
 int fd_fstat(int fd, struct stat *buf)
 {
     if (fstat(fd, buf) < 0)
         return err_msg("fstat");
     return 0;
 }
 
 int fd_lstat(const char *path, struct stat *buf)
 {
     if (lstat(path, buf) < 0)
         return err_msg("lstat");
     return 0;
 }
 
 int fd_fstatat(int fd, const char *path, struct stat *buf, int flag)
 {
     if (fstatat(fd, path, buf, flag) < 0)
         return err_msg("fstatat");
     return 0;
 }
 
 void print_stat(const char *path, const struct stat *sb)
 {
     char mstr[11];
     char tstr[32];
     struct tm *tm;
 
     mode_to_string(sb->st_mode, mstr);
     //tm = localtime(&sb->st_mtim.tv_sec);
     strftime(tstr, sizeof(tstr), "%b %e %H:%M", tm);
 
     printf("%s  %3lu  %5u %5u  %8lld  %s  %s\n",
            mstr,
            (unsigned long)sb->st_nlink,
            (unsigned)sb->st_uid,
            (unsigned)sb->st_gid,
            (long long)sb->st_size,
            tstr,
            path);
 }
 
 /* =============================================================
  * §4.7  access / faccessat
  * ============================================================= */
 
 int check_access(const char *path, int mode)
 {
     if (access(path, mode) < 0) {
         fprintf(stderr, "access(%s): %s\n", path, strerror(errno));
         return -1;
     }
     return 0;
 }
 
 int check_faccessat(int fd, const char *path, int mode, int flag)
 {
     if (faccessat(fd, path, mode, flag) < 0) {
         fprintf(stderr, "faccessat(%s): %s\n", path, strerror(errno));
         return -1;
     }
     return 0;
 }
 
 /* =============================================================
  * §4.8  umask
  * ============================================================= */
 
 mode_t set_umask(mode_t cmask)
 {
     return umask(cmask);
 }
 
 void print_umask(void)
 {
     mode_t old = umask(0);
     umask(old);  /* restore */
 
     printf("umask = %04o  (", (unsigned)old);
 
     /* Symbolic form */
     printf("u=%s%s%s,",
            (old & S_IRUSR) ? "" : "r",
            (old & S_IWUSR) ? "" : "w",
            (old & S_IXUSR) ? "" : "x");
     printf("g=%s%s%s,",
            (old & S_IRGRP) ? "" : "r",
            (old & S_IWGRP) ? "" : "w",
            (old & S_IXGRP) ? "" : "x");
     printf("o=%s%s%s)\n",
            (old & S_IROTH) ? "" : "r",
            (old & S_IWOTH) ? "" : "w",
            (old & S_IXOTH) ? "" : "x");
 }
 
 /* =============================================================
  * §4.9  chmod / fchmod / fchmodat
  * ============================================================= */
 
 int fd_chmod(const char *path, mode_t mode)
 {
     if (chmod(path, mode) < 0)
         return err_msg("chmod");
     return 0;
 }
 
 int fd_fchmod(int fd, mode_t mode)
 {
     if (fchmod(fd, mode) < 0)
         return err_msg("fchmod");
     return 0;
 }
 
 int fd_fchmodat(int fd, const char *path, mode_t mode, int flag)
 {
     if (fchmodat(fd, path, mode, flag) < 0)
         return err_msg("fchmodat");
     return 0;
 }
 
 /*
  * set_fl / clr_fl — Figure 3.12
  * Fetch current file status flags, OR in / AND-NOT new flags, set.
  */
 void set_fl(int fd, int flags)
 {
     int val = fcntl(fd, F_GETFL, 0);
     if (val < 0) { err_msg("set_fl: F_GETFL"); return; }
     val |= flags;
     if (fcntl(fd, F_SETFL, val) < 0)
         err_msg("set_fl: F_SETFL");
 }
 
 void clr_fl(int fd, int flags)
 {
     int val = fcntl(fd, F_GETFL, 0);
     if (val < 0) { err_msg("clr_fl: F_GETFL"); return; }
     val &= ~flags;
     if (fcntl(fd, F_SETFL, val) < 0)
         err_msg("clr_fl: F_SETFL");
 }
 
 /* =============================================================
  * §4.11  chown / fchown / fchownat / lchown
  * ============================================================= */
 
 int fd_chown(const char *path, uid_t owner, gid_t group)
 {
     if (chown(path, owner, group) < 0)
         return err_msg("chown");
     return 0;
 }
 
 int fd_fchown(int fd, uid_t owner, gid_t group)
 {
     if (fchown(fd, owner, group) < 0)
         return err_msg("fchown");
     return 0;
 }
 
 int fd_fchownat(int fd, const char *path,
                 uid_t owner, gid_t group, int flag)
 {
     if (fchownat(fd, path, owner, group, flag) < 0)
         return err_msg("fchownat");
     return 0;
 }
 
 int fd_lchown(const char *path, uid_t owner, gid_t group)
 {
     if (lchown(path, owner, group) < 0)
         return err_msg("lchown");
     return 0;
 }
 
 /* =============================================================
  * §4.12  File size and holes
  * ============================================================= */
 
 int create_file_with_hole(const char *path,
                            off_t       hole_end,
                            const char *head, size_t head_len,
                            const char *tail, size_t tail_len)
 {
     int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, FILE_MODE);
     if (fd < 0)
         return err_msg("create_file_with_hole: open");
 
     /* Write head bytes at offset 0 */
     if (write(fd, head, head_len) != (ssize_t)head_len) {
         err_msg("create_file_with_hole: write head");
         close(fd);
         return -1;
     }
 
     /* Seek to hole_end — creates a hole */
     if (lseek(fd, hole_end, SEEK_SET) == (off_t)-1) {
         err_msg("create_file_with_hole: lseek");
         close(fd);
         return -1;
     }
 
     /* Write tail bytes */
     if (write(fd, tail, tail_len) != (ssize_t)tail_len) {
         err_msg("create_file_with_hole: write tail");
         close(fd);
         return -1;
     }
 
     printf("[hole] created '%s': size=%lld  hole at bytes %zu..%lld\n",
            path,
            (long long)(hole_end + (off_t)tail_len),
            head_len,
            (long long)hole_end - 1);
 
     close(fd);
     return 0;
 }
 
 void report_file_size(const char *path)
 {
     struct stat sb;
     if (lstat(path, &sb) < 0) { err_msg("report_file_size"); return; }
 
     printf("[size] %s: apparent=%lld bytes  blocks=%lld × 512 = %lld bytes\n",
            path,
            (long long)sb.st_size,
            (long long)sb.st_blocks,
            (long long)sb.st_blocks * 512LL);
 }
 
 /* =============================================================
  * §4.13  truncate / ftruncate
  * ============================================================= */
 
 int fd_truncate(const char *path, off_t length)
 {
     if (truncate(path, length) < 0)
         return err_msg("truncate");
     return 0;
 }
 
 int fd_ftruncate(int fd, off_t length)
 {
     if (ftruncate(fd, length) < 0)
         return err_msg("ftruncate");
     return 0;
 }
 
 /* =============================================================
  * §4.15  link / linkat / unlink / unlinkat / remove
  * ============================================================= */
 
 int fd_link(const char *existing, const char *newpath)
 {
     if (link(existing, newpath) < 0)
         return err_msg("link");
     return 0;
 }
 
 int fd_linkat(int efd, const char *existing,
               int nfd, const char *newpath, int flag)
 {
     if (linkat(efd, existing, nfd, newpath, flag) < 0)
         return err_msg("linkat");
     return 0;
 }
 
 int fd_unlink(const char *path)
 {
     if (unlink(path) < 0)
         return err_msg("unlink");
     return 0;
 }
 
 int fd_unlinkat(int fd, const char *path, int flag)
 {
     if (unlinkat(fd, path, flag) < 0)
         return err_msg("unlinkat");
     return 0;
 }
 
 int fd_remove(const char *path)
 {
     if (remove(path) < 0)
         return err_msg("remove");
     return 0;
 }
 
 /*
  * open_then_unlink — Figure 4.16 pattern.
  *
  * Open the file for read/write, then immediately unlink its
  * directory entry.  The file data persists until the fd is
  * closed — useful for safe temporary files.
  */
 int open_then_unlink(const char *path)
 {
     int fd = open(path, O_RDWR);
     if (fd < 0) {
         err_msg("open_then_unlink: open");
         return -1;
     }
 
     if (unlink(path) < 0) {
         err_msg("open_then_unlink: unlink");
         close(fd);
         return -1;
     }
 
     printf("[unlink] '%s' unlinked; data persists on fd=%d "
            "until close\n", path, fd);
     return fd;
 }
 
 /* =============================================================
  * §4.16  rename / renameat
  * ============================================================= */
 
 int fd_rename(const char *oldname, const char *newname)
 {
     if (rename(oldname, newname) < 0)
         return err_msg("rename");
     return 0;
 }
 
 int fd_renameat(int oldfd, const char *oldname,
                 int newfd, const char *newname)
 {
     if (renameat(oldfd, oldname, newfd, newname) < 0)
         return err_msg("renameat");
     return 0;
 }
 
 /* =============================================================
  * §4.18  symlink / symlinkat / readlink / readlinkat
  * ============================================================= */
 
 int fd_symlink(const char *actualpath, const char *sympath)
 {
     if (symlink(actualpath, sympath) < 0)
         return err_msg("symlink");
     return 0;
 }
 
 int fd_symlinkat(const char *actualpath, int fd, const char *sympath)
 {
     if (symlinkat(actualpath, fd, sympath) < 0)
         return err_msg("symlinkat");
     return 0;
 }
 
 ssize_t fd_readlink(const char *path, char *buf, size_t bufsz)
 {
     ssize_t n = readlink(path, buf, bufsz - 1);
     if (n < 0) {
         err_msg("readlink");
         return -1;
     }
     buf[n] = '\0';  /* readlink does NOT null-terminate */
     return n;
 }
 
 ssize_t fd_readlinkat(int fd, const char *path,
                       char *buf, size_t bufsz)
 {
     ssize_t n = readlinkat(fd, path, buf, bufsz - 1);
     if (n < 0) {
         err_msg("readlinkat");
         return -1;
     }
     buf[n] = '\0';
     return n;
 }
 
 /* =============================================================
  * §4.20  File time update functions
  * ============================================================= */
 
 int fd_futimens(int fd, const struct timespec times[2])
 {
     if (futimens(fd, times) < 0)
         return err_msg("futimens");
     return 0;
 }
 
 int fd_utimensat(int fd, const char *path,
                  const struct timespec times[2], int flag)
 {
     if (utimensat(fd, path, times, flag) < 0)
         return err_msg("utimensat");
     return 0;
 }
 
 int fd_utimes(const char *path, const struct timeval times[2])
 {
     if (utimes(path, times) < 0)
         return err_msg("utimes");
     return 0;
 }
 
 /*
  * trunc_preserve_times — Figure 4.21.
  *
  * 1. stat the file to save current access and modification times.
  * 2. Open and truncate it to zero (O_TRUNC).
  * 3. Restore the saved times with futimens.
  *
  * Note: st_ctim (inode change time) WILL be updated because we
  * changed the file — that is expected and unavoidable.
  */
 int trunc_preserve_times(const char *path)
 {
     struct stat      sb;
     struct timespec  times[2];
     int              fd, ret = 0;
 
     /* Step 1: fetch current times */
     if (stat(path, &sb) < 0)
         return err_msg("trunc_preserve_times: stat");
 
     /* Step 2: open and truncate */
     fd = open(path, O_RDWR | O_TRUNC);
     if (fd < 0)
         return err_msg("trunc_preserve_times: open");
 
     /* Step 3: restore access time and modification time */
    // times[0] = sb.st_atim;
     //times[1] = sb.st_mtim;
     if (futimens(fd, times) < 0) {
         err_msg("trunc_preserve_times: futimens");
         ret = -1;
     }
 
     close(fd);
 
     if (ret == 0)
         printf("[times] '%s' truncated; atime and mtime preserved\n",
                path);
     return ret;
 }
 
 /* =============================================================
  * §4.21  mkdir / mkdirat / rmdir
  * ============================================================= */
 
 int fd_mkdir(const char *path, mode_t mode)
 {
     if (mkdir(path, mode) < 0)
         return err_msg("mkdir");
     return 0;
 }
 
 int fd_mkdirat(int fd, const char *path, mode_t mode)
 {
     if (mkdirat(fd, path, mode) < 0)
         return err_msg("mkdirat");
     return 0;
 }
 
 int fd_rmdir(const char *path)
 {
     if (rmdir(path) < 0)
         return err_msg("rmdir");
     return 0;
 }
 
 /* =============================================================
  * §4.22  Directory walking — Figure 4.22
  * ============================================================= */
 
 /* Internal recursive worker */
 static char  *g_fullpath = NULL;
 static size_t g_pathlen  = 0;
 
 static int dopath(ftw_func_t func)
 {
     struct stat    sb;
     struct dirent *dirp;
     DIR           *dp;
     int            ret = 0;
     size_t         n;
 
     if (lstat(g_fullpath, &sb) < 0)
         return func(g_fullpath, &sb, FTW_NS);
 
     if (!S_ISDIR(sb.st_mode))
         return func(g_fullpath, &sb, FTW_F);
 
     /* It's a directory — call func for the dir itself first */
     ret = func(g_fullpath, &sb, FTW_D);
     if (ret != 0)
         return ret;
 
     n = strlen(g_fullpath);
 
     /* Grow path buffer if needed */
     if (n + NAME_MAX + 2 > g_pathlen) {
         g_pathlen *= 2;
         char *tmp = realloc(g_fullpath, g_pathlen);
         if (!tmp) {
             perror("dopath: realloc");
             return -1;
         }
         g_fullpath = tmp;
     }
 
     g_fullpath[n++] = '/';
     g_fullpath[n]   = '\0';
 
     dp = opendir(g_fullpath);
     if (!dp)
         return func(g_fullpath, &sb, FTW_DNR);
 
     while ((dirp = readdir(dp)) != NULL) {
         if (strcmp(dirp->d_name, ".") == 0 ||
             strcmp(dirp->d_name, "..") == 0)
             continue;
 
         /* Append the entry name */
         strcpy(g_fullpath + n, dirp->d_name);
 
         ret = dopath(func);
         if (ret != 0)
             break;
     }
 
     /* Erase everything from the trailing slash onward */
     g_fullpath[n - 1] = '\0';
 
     if (closedir(dp) < 0)
         fprintf(stderr, "dopath: closedir(%s): %s\n",
                 g_fullpath, strerror(errno));
 
     return ret;
 }
 
 int myftw(const char *pathname, ftw_func_t func)
 {
     g_pathlen  = PATH_MAX + 1;
     g_fullpath = malloc(g_pathlen);
     if (!g_fullpath) {
         perror("myftw: malloc");
         return -1;
     }
 
     /* Grow if the starting path already exceeds PATH_MAX */
     size_t plen = strlen(pathname);
     if (plen >= g_pathlen) {
         g_pathlen = plen * 2 + 1;
         char *tmp = realloc(g_fullpath, g_pathlen);
         if (!tmp) {
             perror("myftw: realloc");
             free(g_fullpath);
             g_fullpath = NULL;
             return -1;
         }
         g_fullpath = tmp;
     }
 
     strcpy(g_fullpath, pathname);
     int ret = dopath(func);
 
     free(g_fullpath);
     g_fullpath = NULL;
     g_pathlen  = 0;
 
     return ret;
 }
 
 /* Built-in callback: count file types */
 int count_filetypes(const char *path, const struct stat *sb, int type)
 {
     (void)path;
 
     if (type == FTW_D) {
         cnt_dir++;
         return 0;
     }
 
     if (type == FTW_DNR || type == FTW_NS)
         return 0;   /* FTW_F covers all non-directory files */
 
     switch (sb->st_mode & S_IFMT) {
     case S_IFREG:  cnt_reg++;  break;
     case S_IFBLK:  cnt_blk++;  break;
     case S_IFCHR:  cnt_chr++;  break;
     case S_IFIFO:  cnt_fifo++; break;
     case S_IFLNK:  cnt_lnk++;  break;
     case S_IFSOCK: cnt_sock++; break;
     default:
         break;
     }
     return 0;
 }
 
 void print_filetype_counts(void)
 {
     long tot = cnt_reg + cnt_dir + cnt_blk + cnt_chr +
                cnt_fifo + cnt_lnk + cnt_sock;
     if (tot == 0) tot = 1;
 
     printf("regular files  = %7ld, %5.2f %%\n", cnt_reg,
            cnt_reg  * 100.0 / (double)tot);
     printf("directories    = %7ld, %5.2f %%\n", cnt_dir,
            cnt_dir  * 100.0 / (double)tot);
     printf("block special  = %7ld, %5.2f %%\n", cnt_blk,
            cnt_blk  * 100.0 / (double)tot);
     printf("char special   = %7ld, %5.2f %%\n", cnt_chr,
            cnt_chr  * 100.0 / (double)tot);
     printf("FIFOs          = %7ld, %5.2f %%\n", cnt_fifo,
            cnt_fifo * 100.0 / (double)tot);
     printf("symbolic links = %7ld, %5.2f %%\n", cnt_lnk,
            cnt_lnk  * 100.0 / (double)tot);
     printf("sockets        = %7ld, %5.2f %%\n", cnt_sock,
            cnt_sock * 100.0 / (double)tot);
 }
 
 /* =============================================================
  * §4.23  chdir / fchdir / getcwd
  * ============================================================= */
 
 int fd_chdir(const char *path)
 {
     if (chdir(path) < 0)
         return err_msg("chdir");
     return 0;
 }
 
 int fd_fchdir(int fd)
 {
     if (fchdir(fd) < 0)
         return err_msg("fchdir");
     return 0;
 }
 
 char *fd_getcwd(char *buf, size_t size)
 {
     char *p = getcwd(buf, size);
     if (!p)
         err_msg("getcwd");
     return p;
 }
 
 char *getcwd_alloc(void)
 {
     size_t sz;
     char  *buf = path_alloc(&sz);
     if (!buf) return NULL;
 
     if (!getcwd(buf, sz)) {
         err_msg("getcwd_alloc");
         free(buf);
         return NULL;
     }
     return buf;
 }
 
 /* =============================================================
  * §4.24  Device special files
  * ============================================================= */
 
 unsigned int dev_major(dev_t dev)
 {
     //return (unsigned int)major(dev);
 }
 
 unsigned int dev_minor(dev_t dev)
 {
     //return (unsigned int)minor(dev);
 }
 
 void print_dev_numbers(const char *path)
 {
     struct stat sb;
     if (stat(path, &sb) < 0) {
         fprintf(stderr, "print_dev_numbers(%s): %s\n",
                 path, strerror(errno));
         return;
     }
 
     printf("%s: dev = %u/%u",
            path,
            dev_major(sb.st_dev),
            dev_minor(sb.st_dev));
 
     if (S_ISCHR(sb.st_mode) || S_ISBLK(sb.st_mode)) {
         printf("  (%s) rdev = %u/%u",
                S_ISCHR(sb.st_mode) ? "character" : "block",
                dev_major(sb.st_rdev),
                dev_minor(sb.st_rdev));
     }
     printf("\n");
 }
 