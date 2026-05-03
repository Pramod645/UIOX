#ifndef FILES_DIRS_H
#define FILES_DIRS_H

/*
 * files_dirs.h
 *
 * UNIX Files and Directories
 *
 * Covers:
 *   §4.2   stat, fstat, lstat, fstatat
 *   §4.3   File types and type-testing macros
 *   §4.4   Set-user-ID / Set-group-ID
 *   §4.5   File access permissions
 *   §4.7   access, faccessat
 *   §4.8   umask
 *   §4.9   chmod, fchmod, fchmodat
 *   §4.10  Sticky bit
 *   §4.11  chown, fchown, fchownat, lchown
 *   §4.12  File size, holes
 *   §4.13  truncate, ftruncate
 *   §4.15  link, linkat, unlink, unlinkat, remove
 *   §4.16  rename, renameat
 *   §4.18  symlink, symlinkat, readlink, readlinkat
 *   §4.20  futimens, utimensat, utimes
 *   §4.21  mkdir, mkdirat, rmdir
 *   §4.22  Directory walk (myftw / dopath)
 *   §4.23  chdir, fchdir, getcwd
 *   §4.24  Device numbers (st_dev / st_rdev)
 *   §4.25  Permission bit summary helpers
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <limits.h>

/* =============================================================
 * Constants
 * ============================================================= */

#ifndef PATH_MAX
#define PATH_MAX            4096
#endif

#ifndef NAME_MAX
#define NAME_MAX            255
#endif

/* File mode creation mask (typical default) */
#define UMASK_DEFAULT       022

/* Common permission shorthand */
#define FILE_MODE           (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define DIR_MODE            (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)

/* Permission sets */
#define RWRWRW  (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)

/* =============================================================
 * File type string table (for printing)
 * ============================================================= */

/* Returns a human-readable string for the file type in st_mode */
const char *filetype_str(mode_t mode);

/* =============================================================
 * §4.2  stat family wrappers
 *
 * These thin wrappers print a diagnostic on error and return
 * -1, matching the behaviour described in the text.
 * ============================================================= */

int  fd_stat    (const char *path,  struct stat *buf);
int  fd_fstat   (int fd,            struct stat *buf);
int  fd_lstat   (const char *path,  struct stat *buf);
int  fd_fstatat (int fd, const char *path,
                 struct stat *buf,  int flag);

/* Print all stat fields for a path (similar to ls -l output) */
void print_stat (const char *path,  const struct stat *buf);

/* =============================================================
 * §4.7  access / faccessat
 * ============================================================= */

/*
 * check_access — test real-user-ID access permission.
 * mode: F_OK, R_OK, W_OK, X_OK (or OR combination of last three)
 * Returns 0 if access is allowed, -1 otherwise.
 */
int  check_access   (const char *path, int mode);
int  check_faccessat(int fd, const char *path, int mode, int flag);

/* =============================================================
 * §4.8  umask
 * ============================================================= */

/* Set the file mode creation mask; return the previous mask */
mode_t set_umask(mode_t cmask);

/* Print the current umask in octal and symbolic form */
void   print_umask(void);

/* =============================================================
 * §4.9  chmod / fchmod / fchmodat
 * ============================================================= */

int  fd_chmod   (const char *path, mode_t mode);
int  fd_fchmod  (int fd,           mode_t mode);
int  fd_fchmodat(int fd, const char *path, mode_t mode, int flag);

/* set_fl / clr_fl — Figure 3.12: turn file status flags on/off */
void set_fl(int fd, int flags);
void clr_fl(int fd, int flags);

/* =============================================================
 * §4.11  chown / fchown / fchownat / lchown
 * ============================================================= */

int  fd_chown   (const char *path, uid_t owner, gid_t group);
int  fd_fchown  (int fd,           uid_t owner, gid_t group);
int  fd_fchownat(int fd, const char *path,
                 uid_t owner, gid_t group, int flag);
int  fd_lchown  (const char *path, uid_t owner, gid_t group);

/* =============================================================
 * §4.12  File size and holes
 * ============================================================= */

/*
 * create_file_with_hole — Figure 3.2 equivalent.
 * Writes 'head' bytes at offset 0, seeks to 'hole_end', then
 * writes 'tail' bytes.  Returns 0 on success, -1 on error.
 */
int  create_file_with_hole(const char *path,
                            off_t       hole_end,
                            const char *head, size_t head_len,
                            const char *tail, size_t tail_len);

/* Report apparent size vs. actual disk usage */
void report_file_size(const char *path);

/* =============================================================
 * §4.13  truncate / ftruncate
 * ============================================================= */

int  fd_truncate (const char *path, off_t length);
int  fd_ftruncate(int fd,           off_t length);

/* =============================================================
 * §4.15  link / linkat / unlink / unlinkat / remove
 * ============================================================= */

int  fd_link    (const char *existing, const char *newpath);
int  fd_linkat  (int efd, const char *existing,
                 int nfd, const char *newpath, int flag);
int  fd_unlink  (const char *path);
int  fd_unlinkat(int fd, const char *path, int flag);
int  fd_remove  (const char *path);

/*
 * open_then_unlink — 
 * Opens path for reading/writing then immediately unlinks it.
 * The file persists until the returned fd is closed.
 * Returns the open fd, or -1 on error.
 */
int  open_then_unlink(const char *path);

/* =============================================================
 * §4.16  rename / renameat
 * ============================================================= */

int  fd_rename   (const char *oldname, const char *newname);
int  fd_renameat (int oldfd, const char *oldname,
                  int newfd, const char *newname);

/* =============================================================
 * §4.18  symlink / symlinkat / readlink / readlinkat
 * ============================================================= */

int     fd_symlink   (const char *actualpath, const char *sympath);
int     fd_symlinkat (const char *actualpath,
                      int fd, const char *sympath);
ssize_t fd_readlink  (const char *path, char *buf, size_t bufsz);
ssize_t fd_readlinkat(int fd, const char *path,
                      char *buf, size_t bufsz);

/* =============================================================
 * §4.20  File time update functions
 * ============================================================= */

/* futimens — change times using an open file descriptor */
int  fd_futimens (int fd, const struct timespec times[2]);

/* utimensat — change times using pathname + directory fd */
int  fd_utimensat(int fd, const char *path,
                  const struct timespec times[2], int flag);

/* utimes — legacy microsecond-resolution version */
int  fd_utimes   (const char *path, const struct timeval times[2]);

/*
 * trunc_preserve_times — :
 * Truncate a file to zero but preserve its access and
 * modification times.  Returns 0 on success, -1 on error.
 */
int  trunc_preserve_times(const char *path);

/* =============================================================
 * §4.21  mkdir / mkdirat / rmdir
 * ============================================================= */

int  fd_mkdir   (const char *path, mode_t mode);
int  fd_mkdirat (int fd, const char *path, mode_t mode);
int  fd_rmdir   (const char *path);

/* =============================================================
 * §4.22  Directory walking  (Figure 4.22)
 * ============================================================= */

/* Flags passed to the user callback */
#define FTW_F   1   /* non-directory file          */
#define FTW_D   2   /* directory                   */
#define FTW_DNR 3   /* directory that can't be read */
#define FTW_NS  4   /* file we can't stat           */

/* Callback signature */
typedef int (*ftw_func_t)(const char *path,
                          const struct stat *sb,
                          int type);

/*
 * myftw — recursively descend from 'pathname', calling func
 * for every entry.  Uses lstat (does NOT follow symlinks).
 * Returns whatever func returns, or -1 on allocation error.
 */
int  myftw(const char *pathname, ftw_func_t func);

/* Built-in callback: count file types, print summary */
int  count_filetypes(const char *path,
                     const struct stat *sb, int type);

/* Print the counts collected by count_filetypes */
void print_filetype_counts(void);

/* =============================================================
 * §4.23  chdir / fchdir / getcwd
 * ============================================================= */

int   fd_chdir (const char *path);
int   fd_fchdir(int fd);
char *fd_getcwd(char *buf, size_t size);

/* Allocate a PATH_MAX+1 buffer and fill it with the cwd */
char *getcwd_alloc(void);

/* =============================================================
 * §4.24  Device special files
 * ============================================================= */

/* Print st_dev and (if special file) st_rdev for path */
void print_dev_numbers(const char *path);

/* Extract major/minor portably */
unsigned int dev_major(dev_t dev);
unsigned int dev_minor(dev_t dev);

/* =============================================================
 * §4.25  Permission bit helpers
 * ============================================================= */

/* Return a 10-character ls-style mode string, e.g. "-rwxr-xr-x" */
void mode_to_string(mode_t mode, char out[11]);

/* Test individual permission categories */
bool perm_user_can_read   (mode_t mode);
bool perm_user_can_write  (mode_t mode);
bool perm_user_can_exec   (mode_t mode);
bool perm_group_can_read  (mode_t mode);
bool perm_group_can_write (mode_t mode);
bool perm_group_can_exec  (mode_t mode);
bool perm_other_can_read  (mode_t mode);
bool perm_other_can_write (mode_t mode);
bool perm_other_can_exec  (mode_t mode);
bool has_setuid            (mode_t mode);
bool has_setgid            (mode_t mode);
bool has_sticky            (mode_t mode);

/*
 * check_permission — kernel-style four-step access test
 * (§4.5): returns true if euid/egid may access the file
 * described by sb with the requested mode (R_OK/W_OK/X_OK).
 */
bool check_permission(const struct stat *sb,
                      uid_t euid, gid_t egid, int req_mode);

/* =============================================================
 * Utility / diagnostics
 * ============================================================= */

/* Print a perror-style message and return -1 */
int  err_msg(const char *msg);

/* Allocate a path buffer of PATH_MAX+1 bytes */
char *path_alloc(size_t *sizep);

#endif /* FILES_DIRS_H */
