#include "../include/tty_id.h"

/* ── tty_ctermid ─────────────────────────────────────────────
 *
 * Figure 18.12 ( §18.9) — POSIX.1 ctermid() implementation.
 *
 * The controlling terminal is always "/dev/tty" on all four
 * platforms (FreeBSD, Linux, macOS, Solaris).
 *
 * If str == NULL: use internal static buffer.
 * If str != NULL: store name in caller's buffer (must be
 *                 at least L_ctermid bytes).
 *
 * We cannot protect against overflow of the caller's buffer
 * because we have no way to determine its size — this is a
 * known limitation of the POSIX specification.
 */
char *tty_ctermid(char *str)
{
    static char ctermid_buf[L_ctermid];

    if (str == NULL)
        str = ctermid_buf;

    return strcpy(str, "/dev/tty");
}

/* ── tty_isatty ──────────────────────────────────────────────
 *
 * Figure 18.13 (APUE §18.9) — POSIX.1 isatty() implementation.
 *
 * Strategy: call a terminal-specific function that succeeds
 * only on terminal devices and does not change any state.
 * tcgetattr() is ideal:
 *   - Returns 0 on success (fd is a terminal).
 *   - Returns -1 with errno=ENOTTY if fd is not a terminal.
 *   - Does not modify any terminal settings (read-only).
 *
 * Returns 1 (true) if fd is a terminal, 0 (false) otherwise.
 */
int tty_isatty(int fd)
{
    struct termios ts;
    return tcgetattr(fd, &ts) != -1;
}

/* ── Internal linked list for subdirectory tracking ─────────
 *
 * Figure 18.15 — ttyname() uses a singly-linked list to
 * queue subdirectories found during the /dev scan.
 * This avoids deep recursion.
 */
struct devdir {
    struct devdir *d_next;
    char          *d_name;
};

static struct devdir *head = NULL;
static struct devdir *tail = NULL;

/* Static pathname buffer — shared by searchdir and ttyname */
static char tty_pathname[_POSIX_PATH_MAX + 1];

/* ── add() — enqueue a subdirectory path ─────────────────────
 *
 * Skips:
 *   /dev/.   — current directory entry
 *   /dev/..  — parent directory entry
 *   /dev/fd  — /dev/fd contains synthetic file descriptors
 */
static void add(char *dirname)
{
    struct devdir *ddp;
    int            len;

    len = (int)strlen(dirname);

    /* Skip "." and ".." entries (last char is '.') */
    if (dirname[len-1] == '.' &&
        (dirname[len-2] == '/' ||
         (dirname[len-2] == '.' && dirname[len-3] == '/')))
        return;

    /* Skip /dev/fd — it contains stdin/stdout/stderr aliases */
    if (strcmp(dirname, "/dev/fd") == 0)
        return;

    ddp = malloc(sizeof(struct devdir));
    if (ddp == NULL)
        return;

    ddp->d_name = strdup(dirname);
    if (ddp->d_name == NULL) {
        free(ddp);
        return;
    }

    ddp->d_next = NULL;
    if (tail == NULL) {
        head = ddp;
        tail = ddp;
    } else {
        tail->d_next = ddp;
        tail         = ddp;
    }
}

/* ── cleanup() — free the devdir linked list ─────────────────
 *
 * Called after ttyname() finishes searching.
 */
static void cleanup(void)
{
    struct devdir *ddp, *nddp;

    ddp = head;
    while (ddp != NULL) {
        nddp = ddp->d_next;
        free(ddp->d_name);
        free(ddp);
        ddp = nddp;
    }
    head = NULL;
    tail = NULL;
}

/* ── searchdir() — search one directory for matching device ──
 *
 * For each entry in dirname:
 *   - Skip /dev/stdin, /dev/stdout, /dev/stderr (symbolic links
 *     into /dev/fd that would give misleading results).
 *   - If entry is a directory, add it to the pending list.
 *   - If entry is a regular or special file, compare its
 *     st_ino and st_dev with the target fd's stat.
 *   - A matching inode+device pair on a UNIX system uniquely
 *     identifies the file — no further checks needed.
 *
 * Returns pathname of matching entry, or NULL if not found.
 */
static char *searchdir(char *dirname, struct stat *fdstatp)
{
    struct stat    devstat;
    DIR           *dp;
    int            devlen;
    struct dirent *dirp;

    strcpy(tty_pathname, dirname);

    dp = opendir(dirname);
    if (dp == NULL)
        return NULL;

    strcat(tty_pathname, "/");
    devlen = (int)strlen(tty_pathname);

    while ((dirp = readdir(dp)) != NULL) {
        strncpy(tty_pathname + devlen, dirp->d_name,
                _POSIX_PATH_MAX - devlen);
        tty_pathname[_POSIX_PATH_MAX] = '\0';

        /* Skip stdin/stdout/stderr aliases in /dev */
        if (strcmp(tty_pathname, "/dev/stdin")  == 0 ||
            strcmp(tty_pathname, "/dev/stdout") == 0 ||
            strcmp(tty_pathname, "/dev/stderr") == 0)
            continue;

        if (stat(tty_pathname, &devstat) < 0)
            continue;

        if (S_ISDIR(devstat.st_mode)) {
            /* Enqueue subdirectory for later search */
            add(tty_pathname);
            continue;
        }

        /*
         * Match: same inode AND same device.
         * Each (st_dev, st_ino) pair is unique on a UNIX system.
         */
        if (devstat.st_ino == fdstatp->st_ino &&
            devstat.st_dev == fdstatp->st_dev) {
            closedir(dp);
            return tty_pathname;
        }
    }

    closedir(dp);
    return NULL;
}

/* ── tty_ttyname ─────────────────────────────────────────────
 *
 * Figure 18.15 (APUE §18.9) — POSIX.1 ttyname() implementation.
 *
 * Algorithm:
 *   1. isatty(fd)           — must be a terminal device
 *   2. fstat(fd, &fdstat)   — get st_ino, st_dev, st_mode
 *   3. S_ISCHR(st_mode)     — must be character special file
 *   4. searchdir("/dev")    — scan /dev for matching entry
 *   5. For each subdirectory found during scan, search it too.
 *   6. cleanup()            — free subdirectory list
 *
 * Returns pointer to static buffer (not reentrant).
 * Returns NULL if fd is not a terminal or no match found.
 */
char *tty_ttyname(int fd)
{
    struct stat    fdstat;
    struct devdir *ddp;
    char          *rval;

    /* Must be a terminal */
    if (tty_isatty(fd) == 0)
        return NULL;

    /* Get inode and device number */
    if (fstat(fd, &fdstat) < 0)
        return NULL;

    /* Must be a character special file */
    if (S_ISCHR(fdstat.st_mode) == 0)
        return NULL;

    /* Search /dev first — most terminal entries are here */
    rval = searchdir("/dev", &fdstat);

    /* Search any subdirectories found during /dev scan */
    if (rval == NULL) {
        for (ddp = head; ddp != NULL; ddp = ddp->d_next) {
            rval = searchdir(ddp->d_name, &fdstat);
            if (rval != NULL)
                break;
        }
    }

    /* Free the subdirectory list */
    cleanup();

    return rval;
}
