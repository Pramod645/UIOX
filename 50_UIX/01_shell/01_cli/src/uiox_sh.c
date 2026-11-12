/**
 * @file    uiox_sh.c
 * @brief   The command-line front-end — a shell over the console tty.
 *
 * PATH    50_UIX/01_shell/01_cli/src/uiox_sh.c
 *
 * A USER program.  It runs at EL0 and reaches the kernel through the
 * the uixlibs wrapper layer — uix_read()/uix_write() in PoStd, which call
 * my_syscallN in 40_SCIX underneath.  A front-end uses the wrappers, not
 * the trap layer: 40_SCIX is reached by the libs, not by programs.
 *
 * ── fd 0/1/2 ARE the console ────────────────────────────────────────
 * The console is bound to descriptors 0, 1 and 2 at boot by
 * uiox_console_bind(), called from uiox_uart_probe().  Nothing here
 * opens a path: /dev/ttyS0 is not a node this program can resolve, and
 * under this design it does not need to be.
 *
 *   write(1, ...)  →  uiox_console_write()      tx_char applies OPOST|ONLCR
 *   read (0, ...)  →  uiox_console_readline()   canon_feed edits the line
 *
 * Both are intercepted in uiox_kix_router.c by descriptor number, ahead
 * of the SCFS table — so a CMD inode never has to answer SCFS_ENOTTY.
 * That is why the shell does not call uiox_kix_scfs_*() and why it must
 * not: those are the kernel's file-level entry points, one layer below
 * the boundary this program sits above.
 *
 * ── readline is almost free ─────────────────────────────────────────
 * The line discipline in uiox_uart_if.c already does the hard part:
 * ICANON assembles the line, ERASE emits BS-SP-BS, KILL drops it, VEOF
 * ends it, and ECHO writes every accepted byte back.  So
 * sh_readline() is a read() loop until the line completes — the editor
 * lives in the kernel, where it belongs.
 *
 * ── what this shell does NOT have, and says so ──────────────────────
 * No pipes, no redirection, no job control, no $PATH search, no
 * external programs.  Every command is a builtin implemented here.
 * "Not built in" is reported as such rather than silently ignored —
 * the same rule the rest of the tree follows.
 *
 * @version 1.1.0  @date 2026-11-08
 *           1.1.0 moves this to userspace: uix_sys.h replaces
 *                 uiox_kix_scfs.h, output goes through uix_write(),
 *                 fd 0/1/2 are used as the console rather than opened,
 *                 and the builtins call the 40_SCIX wrappers.
 *           1.0.0 was a boot-context shell calling 10_scfs directly.
 */

/* The userspace syscall surface.  SYS_READ, SYS_WRITE and the sys_*()
 * inline wrappers live here — NOT in 30_KIX/32_FS/10_scfs, which holds
 * the kernel's implementations and is the wrong side of the boundary. */
#include "uix_types.h"         /* uix_size_t, uix_ssize_t, uix_bool_t */
#include "uix_posix_io.h"      /* uix_read / uix_write / uix_open */
#include "uix_posix_proc.h"    /* uix_chdir / uix_getcwd           */
#include "uix_dirent.h"        /* uix_fdopendir / uix_readdir      */

/* ── forward declarations ─────────────────────────────────────────────
 * main() calls these; their definitions sit below it. */
static int  sh_readline(char *buf, uix_size_t cap);
static void sh_puts(const char *s);
static void sh_putc(char c);

/* ── limits ─────────────────────────────────────────────────────────── */
#define SH_LINE_MAX    256u
#define SH_ARGV_MAX     32u
#define SH_PROMPT      "uiox> "

/* ═════════════════════════════════════════════════════════════════════
 * Output
 *
 * Everything leaves through write() on fd 1.  NOT through early_puts(),
 * which is the boot path's direct UART sink, and NOT through a 10_scfs
 * function call.  The console is bound to fd 1, so the output is
 * attributed to this program and tx_char() applies OPOST|ONLCR on the
 * way out — a "\n" here reaches the wire as CRLF without the shell
 * knowing about it.
 * ═════════════════════════════════════════════════════════════════════ */

/* The console descriptors.  These are not opened — they ARE the console,
 * bound at boot by uiox_console_bind().  sh_fd1 was 0 in v1.0.0, which
 * routed every prompt through SYS_READ; that only worked because the
 * boot-context shell reached 10_scfs directly and never went through a
 * descriptor at all. */
static int sh_fd0 = 0, sh_fd1 = 1, sh_fd2 = 2;

static void sh_puts(const char *s)
{
    uix_size_t n = 0u;
    while (s[n]) n++;
    if (n) (void)uix_write(sh_fd1, s, (uix_size_t)n);
}

static void sh_putc(char c)
{
    (void)uix_write(sh_fd1, &c, (uix_size_t)1u);
}

static void sh_putu(uix_size_t v)
{
    char b[11];
    int  i = 0;
    if (v == 0u) { sh_putc('0'); return; }
    while (v > 0u && i < 10) { b[i++] = (char)('0' + (v % 10u)); v /= 10u; }
    while (i > 0) sh_putc(b[--i]);
}

/* ═════════════════════════════════════════════════════════════════════
 * readline — a read() loop.  The editing is the line discipline's.
 *
 * uix_read() on fd 0 reaches uiox_console_readline(), which drives
 * canon_feed(): ICANON assembles the line, ERASE backs the cursor up,
 * KILL drops the line, VEOF ends it, ECHO writes every accepted byte
 * back.  So this is a loop until a complete line arrives — there is no
 * editor here to get wrong.
 *
 * Return:  byte count excluding the NUL, 0 on VEOF at an empty line,
 *          -3 when the discipline consumed a ^C as a signal.
 * ═════════════════════════════════════════════════════════════════════ */

/* The line arrives already assembled, so one call is enough.  The -3 for
 * VINTR is uiox_uart_if.c's convention: canon_feed() consumes the byte
 * and hands it to uiox_tty_signal_from_char() without delivering it. */
static int sh_readline(char *buf, uix_size_t cap)
{
    if (!buf || cap == 0u) return -1;

    for (;;) {
        uix_ssize_t r = uix_read(sh_fd0, buf, (uix_size_t)(cap - 1u));

        if (r > 0) {
            buf[r] = '\0';
            return (int)r;
        }
        if (r == 0)  return 0;      /* VEOF */
        if (r == -3) return -3;     /* VINTR — consumed as a signal */
        /* -2 is the discipline's EAGAIN: nothing had arrived.  Loop
         * rather than report it — the caller asked for a line. */
        if (r != -2) return (int)r;
    }
}

/* ═════════════════════════════════════════════════════════════════════
 * Tokenize
 * ═════════════════════════════════════════════════════════════════════ */
static int sh_tokenize(char *line, char **argv, int max)
{
    int argc = 0;
    char *p = line;

    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\n') p++;
        if (!*p) break;
        if (argc >= max) break;

        argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n') p++;
        if (*p) *p++ = '\0';
    }
    return argc;
}

static uix_bool_t sh_streq(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return (*a == *b) ? 1 : 0;
}

/* ═════════════════════════════════════════════════════════════════════
 * Builtins
 * ═════════════════════════════════════════════════════════════════════ */

/* ── echo ─────────────────────────────────────────────────────────── */
static int bi_echo(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (i > 1) sh_putc(' ');
        sh_puts(argv[i]);
    }
    sh_putc('\n');
    return 0;
}

/* ── help ─────────────────────────────────────────────────────────── */
static int bi_help(int argc, char **argv)
{
    (void)argc; (void)argv;
    sh_puts("uiox shell — builtins:\n");
    sh_puts("  help            this list\n");
    sh_puts("  echo ARG...     print the arguments\n");
    sh_puts("  cd [DIR]        change directory (no arg: /)\n");
    sh_puts("  pwd             print the working directory\n");
    sh_puts("  ls [DIR]        list a directory\n");
    sh_puts("  cat FILE...     print file contents\n");
    sh_puts("  ps              list processes\n");
    sh_puts("  exit            leave the shell\n");
    sh_puts("\nNo pipes, redirection, job control or external programs.\n");
    return 0;
}

/* ── cd / pwd ─────────────────────────────────────────────────────────
 * Both go through the 40_SCIX wrappers, which trap.  v1.0.0 called
 * uiox_kix_scfs_chdir()/getcwd() directly, which is a kernel function
 * call — the wrong side of the boundary this program lives above. */
static int bi_cd(int argc, char **argv)
{
    const char *path = (argc > 1) ? argv[1] : "/";
    uix_ssize_t r = uix_chdir(path);

    if (r < 0) { sh_puts("cd: "); sh_puts(path); sh_puts(": no such directory\n"); }
    return (int)r;
}

static int bi_pwd(int argc, char **argv)
{
    (void)argc; (void)argv;
    char cwd[SH_LINE_MAX];
    /* uix_posix_proc.h:102 — char *uix_getcwd(char *buf, uix_size_t size) */
    if (!uix_getcwd(cwd, (uix_size_t)sizeof cwd)) {
        sh_puts("pwd: cannot read cwd\n");
        return -1;
    }
    sh_puts(cwd);
    sh_putc('\n');
    return 0;
}

/* ── ls ───────────────────────────────────────────────────────────────
 * uix_opendir/uix_readdir walk the stream; uix_dirent_t carries d_name. */
static int bi_ls(int argc, char **argv)
{
    const char *path = (argc > 1) ? argv[1] : ".";

    uix_ssize_t fd = uix_open(path, 0);
    if (fd < 0) {
        /* The FS layer answers ENOTSUP for an unimplemented open.  Say
         * which layer is missing rather than reporting a bogus errno. */
        sh_puts("ls: "); sh_puts(path); sh_puts(": cannot open\n");
        return -1;
    }

    /* The path is opened by uix_opendir(), not by hand: uix_DIR is a
     * struct ({fd, buf[4096], buf_pos, buf_len, offset, entry}), so the
     * raw-offset walk an earlier draft used was never right for this
     * tree — that was Linux dirent layout, not uix_dirent_t. */
    uix_DIR *dir = uix_fdopendir((int)fd);
    if (!dir) {
        sh_puts("ls: "); sh_puts(path); sh_puts(": cannot read directory\n");
        (void)uix_close((int)fd);
        return -1;
    }

    for (;;) {
        uix_dirent_t *de = uix_readdir(dir);
        if (!de) break;

        const char *name = de->d_name;
        if (!name || !name[0]) continue;
        if (sh_streq(name, ".") || sh_streq(name, "..")) continue;

        sh_puts(name);
        sh_putc('\n');
    }

    (void)uix_closedir(dir);   /* closes the fd too */
    return 0;
}

/* ── cat ─────────────────────────────────────────────────────────────
 * One file per argument; the bytes go straight to fd 1, so the
 * discipline's OPOST|ONLCR applies and a text file with bare \n lines
 * prints correctly. */
static int bi_cat(int argc, char **argv)
{
    if (argc < 2) { sh_puts("cat: missing file\n"); return -1; }

    for (int i = 1; i < argc; i++) {
        uix_ssize_t fd = uix_open(argv[i], 0);
        if (fd < 0) { sh_puts("cat: "); sh_puts(argv[i]); sh_puts(": cannot open\n"); continue; }

        char buf[256];
        for (;;) {
            uix_ssize_t n = uix_read((int)fd, buf, (uix_size_t)sizeof buf);
            if (n <= 0) break;
            (void)uix_write(sh_fd1, buf, (uix_size_t)n);
        }
        (void)uix_close((int)fd);
        sh_putc('\n');
    }
    return 0;
}

/* ── ps ──────────────────────────────────────────────────────────────
 * No process table to walk yet — 33_PCS has a scheduler but does not
 * publish an enumeration to userspace.  Reported rather than faked. */
static int bi_ps(int argc, char **argv)
{
    (void)argc; (void)argv;
    sh_puts("ps: no process enumeration exposed yet\n");
    return 0;
}

/* ── exit ────────────────────────────────────────────────────────────
 * There is no exit() to call — 33_PCS has no syscall for it in this
 * build — so this halts the machine, the honest end state.  When this
 * becomes pid 1, this is the call that becomes uiox_proc_exit(0). */
static void bi_exit(int argc, char **argv)
{
    (void)argc; (void)argv;
    sh_puts("shell exiting — halting\n");
    for (;;) {
#if defined(__x86_64__)
        __asm__ volatile("hlt");
#else
        __asm__ volatile("wfi");
#endif
    }
}

/* ═════════════════════════════════════════════════════════════════════
 * Dispatch
 *
 * With no fork/exec and no process table, a command IS a function
 * linked into this image — this chain is the whole command language.
 * Linear comparison rather than a table: eight entries cost less than
 * the indirection would, and the chain reads as a command list.
 *
 * Return:  0  handled, continue
 *         -1  not a builtin
 *          1  the user asked to leave
 * ═════════════════════════════════════════════════════════════════════ */
static int sh_execute(char *line)
{
    char *argv[SH_ARGV_MAX];
    int   argc = sh_tokenize(line, argv, SH_ARGV_MAX - 1);
    if (argc == 0) return 0;                       /* blank line */

    if (sh_streq(argv[0], "help")) return bi_help(argc, argv);
    if (sh_streq(argv[0], "echo")) return bi_echo(argc, argv);
    if (sh_streq(argv[0], "cd"))   return bi_cd(argc, argv);
    if (sh_streq(argv[0], "pwd"))  return bi_pwd(argc, argv);
    if (sh_streq(argv[0], "ls"))   return bi_ls(argc, argv);
    if (sh_streq(argv[0], "cat"))  return bi_cat(argc, argv);
    if (sh_streq(argv[0], "ps"))   return bi_ps(argc, argv);

    /* exit must break the loop, and a builtin's int return cannot do
     * that on its own — so it is reported to the caller as 1.  The
     * fallthrough bug this replaces printed "exit: not a builtin"
     * because there was no return here. */
    if (sh_streq(argv[0], "exit")) { bi_exit(argc, argv); return 1; }

    /* Not a builtin.  There is no $PATH search and no exec yet, so this
     * is reported rather than silently ignored — the same rule the rest
     * of the tree follows for an absent layer. */
    sh_puts(argv[0]);
    sh_puts(": not a builtin (no external programs yet)\n");
    return -1;
}

/* ═════════════════════════════════════════════════════════════════════
 * The builtin table
 *
 * This IS the shell.  With no fork/exec and no process table, a command
 * is a function linked into this image — the table is the whole command
 * language.  Lookup is a linear scan: eight entries cost less than the
 * hashing would.
 * ═════════════════════════════════════════════════════════════════════ */
typedef struct {
    const char *name;
    int (*fn)(int argc, char **argv);
} sh_builtin_t;

static const sh_builtin_t sh_builtins[] = {
    { "help", bi_help },
    { "echo", bi_echo },
    { "cd",   bi_cd   },
    { "pwd",  bi_pwd  },
    { "ls",   bi_ls   },
    { "cat",  bi_cat  },
    { "ps",   bi_ps   },
    { "exit", NULL    },   /* handled in the loop: it must break out */
    { NULL,   NULL    }
};


/* ═════════════════════════════════════════════════════════════════════
 * main — the loop
 *
 * prompt → read a line → split it → dispatch → repeat.
 *
 * sh_readline() gets a COMPLETE line: uiox_uart_if.c's canon_feed()
 * already ran ICANON, ERASE, KILL and ECHO in the kernel, so there is
 * no editor here and there must not be one.  The three returns that
 * matter:
 *   > 0   a line, NUL-terminated
 *   0     VEOF on an empty line — exit, like a login shell
 *   -3    VINTR: the discipline consumed the ^C as a signal
 * ═════════════════════════════════════════════════════════════════════ */
#define SH_LINE_MAX 256
#define SH_ARG_MAX  32
int uix_sh_main(void)
{
    char line[SH_LINE_MAX];

    sh_puts("uiox shell — type 'help' for builtins\n");

    for (;;) {
        int r;

        sh_puts("uiox> ");

        r = sh_readline(line, sizeof(line));
        if (r == 0)  { sh_putc('\n'); break; }       /* VEOF */
        if (r == -3) { sh_puts("^C\n"); continue; }  /* VINTR, consumed */
        if (r < 0)   { sh_puts("read error\n"); continue; }

        if (sh_execute(line) == 1) break;            /* exit */
    }

    sh_puts("bye\n");
    return 0;
}

#ifndef UIOX_SH_NO_MAIN
int main(void) { return uix_sh_main(); }
#endif
