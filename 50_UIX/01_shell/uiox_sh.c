/**
 * @file    uiox_sh.c
 * @brief   The first user program — a shell over the console tty.
 *
 * PATH    50_UIX/01_shell/src/uiox_sh.c
 *
 * Replaces the stub at 30_KIX/common/uiox_kernel_main.c:206, which today
 * prints "[kernel] uiox_shell_start: stub (50_UIX/01_shell not built)".
 *
 * ── why this file is the same in boot context and as pid 1 ──────────
 * uiox_sh_main() opens its tty, runs the loop, and never returns.  Who
 * calls it is the only difference:
 *
 *   today    uiox_shell_start() → uiox_sh_main()      (boot context)
 *   later    arch_context_init(&ctx, uiox_sh_main, 0)
 *            arch_context_switch(&boot_ctx, &ctx)     (a process)
 *
 * Nothing below reaches for a pcb, so no line changes when that swap
 * happens.  That is deliberate: the shell is the piece that should not
 * have to be rewritten when the process layer lands.
 *
 * ── readline is almost free ─────────────────────────────────────────
 * The line discipline in uiox_uart_if.c already does the hard part of a
 * readline: ICANON assembles the line, ERASE emits BS-SP-BS, KILL drops
 * it, VEOF ends it, and ECHO writes every accepted byte back.  So
 * uiox_sh_readline() is a read() loop until the line completes — the
 * editor lives in the kernel, where it belongs.
 *
 * ── what this shell does NOT have, and says so ──────────────────────
 * No pipes, no redirection, no job control, no $PATH search, no
 * external programs.  Every command is a builtin implemented here.
 * "Not built in" is reported as such rather than silently ignored —
 * the same rule the rest of the tree follows.
 *
 * @version 1.0.0  @date 2026-10-21
 */

 #include "uiox_klibc.h"
 #include "uiox_kix_scfs.h"        /* SCFS_*, the syscall surface      */
 #include "uiox_termios.h"         /* UIOX_ICANON, UIOX_ICRNL and so on */
 
 /* ── limits ─────────────────────────────────────────────────────────── */
 #define SH_LINE_MAX    256u
 #define SH_ARGV_MAX     32u
 #define SH_PROMPT      "uiox> "
 
 /* ═════════════════════════════════════════════════════════════════════
  * Output
  *
  * Everything leaves through write() on fd 1 — NOT through early_puts(),
  * which is the boot path's direct UART sink.  Once the console driver
  * is bound, the shell must print through the tty so OPOST|ONLCR applies
  * and so the output is attributed to the process, not to the kernel.
  * ═════════════════════════════════════════════════════════════════════ */
 
 static int sh_fd0 = 0, sh_fd1 = 0, sh_fd2 = 2;
 
 static void sh_puts(const char *s)
 {
     uint32_t n = 0u;
     while (s[n]) n++;
     if (n) (void)uiox_kix_scfs_write(sh_fd1, (void *)s, n);
 }
 
 static void sh_putc(char c)
 {
     (void)uiox_kix_scfs_write(sh_fd1, &c, 1u);
 }
 
 static void sh_putu(uint32_t v)
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
  * Returns the byte count, 0 on EOF (VEOF), or -1 on error.
  * The returned line is NUL-terminated and has the terminating newline
  * stripped, so every caller gets a clean command.
  * ═════════════════════════════════════════════════════════════════════ */
 static int sh_readline(char *buf, uint32_t cap)
 {
     uint32_t n = 0u;
 
     while (n + 1u < cap) {
         char    c;
         int32_t r = uiox_kix_scfs_read(sh_fd0, &c, 1u);
 
         if (r < 0)  return -1;
         if (r == 0) return 0;              /* EOF                      */
         if (r == -2) continue;             /* EAGAIN — nothing yet     */
 
         /* The discipline returns the line INCLUDING its terminator:
          * canon_feed() appends NL (or VEOF) before completing.  Both
          * end the command, and the character itself is not part of it. */
         if (c == '\n' || c == '\r') { buf[n] = '\0'; return (int)n; }
         if (c == 4) { buf[n] = '\0'; return (int)n; }   /* VEOF        */
         if (c == 3) { buf[n] = '\0'; return -3; }       /* ^C — see below */
 
         buf[n++] = c;
     }
 
     buf[n] = '\0';
     return (int)n;                          /* line hit the cap        */
 }
 
 /* ═════════════════════════════════════════════════════════════════════
  * Tokenizer — split on runs of whitespace.  No quoting yet; it is
  * listed as absent rather than half-done.
  * ═════════════════════════════════════════════════════════════════════ */
 static int sh_tokenize(char *line, char **argv, int max)
 {
     int argc = 0;
     char *p  = line;
 
     while (*p && argc < max) {
         while (*p == ' ' || *p == '\t') p++;
         if (!*p) break;
         argv[argc++] = p;
         while (*p && *p != ' ' && *p != '\t') p++;
         if (*p) *p++ = '\0';
     }
     argv[argc] = (char *)0;
     return argc;
 }
 
 static bool sh_streq(const char *a, const char *b)
 {
     while (*a && *a == *b) { a++; b++; }
     return *a == *b;
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
  * cd hands the path to 10_scfs's chdir, which resolves and updates the
  * process's cwd.  No argument means "/", the same as a bare `cd` in a
  * shell with no $HOME. */
 static int bi_cd(int argc, char **argv)
 {
     const char *path = (argc > 1) ? argv[1] : "/";
     int32_t r = uiox_kix_scfs_chdir(path);
     if (r < 0) { sh_puts("cd: "); sh_puts(path); sh_puts(": no such directory\n"); }
     return (int)r;
 }
 
 static int bi_pwd(int argc, char **argv)
 {
     (void)argc; (void)argv;
     char cwd[256];
     int32_t r = uiox_kix_scfs_getcwd(cwd, sizeof cwd);
     if (r < 0) { sh_puts("pwd: cannot read cwd\n"); return -1; }
     sh_puts(cwd);
     sh_putc('\n');
     return 0;
 }
 
 /* ── ls ───────────────────────────────────────────────────────────────
  * getdents64 fills a raw dirent buffer; we walk it and print d_name.
  * The record layout is the kernel's — d_ino, d_off, d_reclen, d_type,
  * then the name — and the name is NUL-terminated in the last byte of
  * the record, which is why the pointer arithmetic is explicit. */
 static int bi_ls(int argc, char **argv)
 {
     const char *path = (argc > 1) ? argv[1] : ".";
     int32_t fd = uiox_kix_scfs_open(path, 0, 0);
     if (fd < 0) { sh_puts("ls: "); sh_puts(path); sh_puts(": cannot open\n"); return (int)fd; }
 
     char    buf[512];
     int32_t n;
     while ((n = uiox_kix_scfs_getdents64((int)fd, buf, sizeof buf)) > 0) {
         int32_t off = 0;
         while (off < n) {
             struct sh_dirent {
                 uint64_t d_ino;
                 int64_t  d_off;
                 uint16_t d_reclen;
                 uint8_t  d_type;
                 char     d_name[];
             } *d = (struct sh_dirent *)(void *)(buf + off);
 
             if (d->d_reclen == 0u) break;         /* corrupt: stop */
             if (!(d->d_name[0] == '.' &&
                   (d->d_name[1] == '\0' ||
                    (d->d_name[1] == '.' && d->d_name[2] == '\0')))) {
                 sh_puts(d->d_name);
                 sh_putc('\n');
             }
             off += (int32_t)d->d_reclen;
         }
     }
     (void)uiox_kix_scfs_close((int)fd);
     return 0;
 }
 
 /* ── cat ──────────────────────────────────────────────────────────── */
 static int bi_cat(int argc, char **argv)
 {
     if (argc < 2) { sh_puts("cat: no file\n"); return -1; }
 
     for (int i = 1; i < argc; i++) {
         int32_t fd = uiox_kix_scfs_open(argv[i], 0, 0);
         if (fd < 0) {
             sh_puts("cat: "); sh_puts(argv[i]); sh_puts(": cannot open\n");
             continue;
         }
         char    buf[256];
         int32_t n;
         while ((n = uiox_kix_scfs_read((int)fd, buf, sizeof buf)) > 0)
             (void)uiox_kix_scfs_write(sh_fd1, buf, (uint32_t)n);
         (void)uiox_kix_scfs_close((int)fd);
     }
     return 0;
 }
 
 /* ── ps ───────────────────────────────────────────────────────────────
  * Walks uiox_kix_psa_proc_table[] — the real table 40_psa owns.  This
  * is the first thing in the tree that lists processes from userspace,
  * and it works today because the table is real even though nothing
  * switches registers yet.
  *
  * state names mirror uiox_kix_psa_proc_state_t's closed enum. */
 #if defined(UIOX_SHELL_HAVE_PSA)
 #include "uiox_kix_psa_process.h"
 
 static const char *sh_state_name(uiox_kix_psa_proc_state_t s)
 {
     switch (s) {
     case UIOX_KIX_PSA_PROC_UNUSED:          return "UNUSED";
     case UIOX_KIX_PSA_PROC_USER_RUNNING:    return "RUN(u)";
     case UIOX_KIX_PSA_PROC_KERNEL_RUNNING:  return "RUN(k)";
     default:                                return "OTHER";
     }
 }
 
 static int bi_ps(int argc, char **argv)
 {
     (void)argc; (void)argv;
     sh_puts("  PID  STATE     CTX\n");
     for (uint32_t i = 0u; i < UIOX_KIX_PSA_NPROC; i++) {
         uiox_kix_psa_proc_t *p = &uiox_kix_psa_proc_table[i];
         if (p->p_state == UIOX_KIX_PSA_PROC_UNUSED) continue;
         sh_puts("  ");
         sh_putu(i);
         sh_puts("    ");
         sh_puts(sh_state_name(p->p_state));
         sh_puts("\n");
     }
     return 0;
 }
 #else
 static int bi_ps(int argc, char **argv)
 {
     (void)argc; (void)argv;
     /* Reported, not faked: the process table is real (40_psa) but this
      * build did not pull its header in, so the shell cannot walk it. */
     sh_puts("ps: process table not compiled in"
             " (build with -DUIOX_SHELL_HAVE_PSA)\n");
     return 0;
 }
 #endif
 
 /* ── exit ─────────────────────────────────────────────────────────────
  * In boot context there is nothing to return to, so exit halts the
  * machine — the honest end state.  When this becomes pid 1, this is the
  * call that becomes uiox_proc_exit(0). */
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
     if (sh_streq(argv[0], "exit")) bi_exit(argc, argv);
 
     /* Not a builtin.  There is no $PATH search and no exec yet, so this
      * is reported rather than silently ignored — the same rule the rest
      * of the tree follows for an absent layer. */
     sh_puts(argv[0]);
     sh_puts(": not a builtin (no external programs yet)\n");
     return -1;
 }
 
 /* ═════════════════════════════════════════════════════════════════════
  * The program
  *
  * Opens its tty and loops forever.  It never returns, which is why the
  * boot body's "FATAL: shell returned — halting" at uiox_kernel_main.c:450
  * is unreachable while this is the real shell.
  * ═════════════════════════════════════════════════════════════════════ */
 int uiox_sh_main(void)
 {
     sh_fd0 = uiox_kix_scfs_open("/dev/ttyS0", 0, 0);           /* O_RDONLY */
     if (sh_fd0 < 0) {
         /* The console device is not in the registry yet.  Say which
          * layer is missing instead of looping on a dead fd. */
         sh_puts("shell: /dev/ttyS0 not openable — console not bound\n");
         return -1;
     }
     sh_fd1 = uiox_kix_scfs_open("/dev/ttyS0", 1, 0);           /* O_WRONLY */
     sh_fd2 = sh_fd1;
 
     sh_puts("\nUIOX shell — type 'help'\n");
 
     for (;;) {
         char line[SH_LINE_MAX];
         sh_puts(SH_PROMPT);
 
         int n = sh_readline(line, sizeof line);
 
         if (n == -3) {                 /* ^C: the discipline consumed it
                                         * as a signal; nothing to run. */
             sh_putc('\n');
             continue;
         }
         if (n == 0) {                  /* VEOF: ^D on an empty line   */
             sh_puts("EOF\n");
             bi_exit(0, (char **)0);
         }
         if (n < 0) {
             sh_puts("read error\n");
             continue;
         }
 
         (void)sh_execute(line);
     }
 }
 
 /* ═════════════════════════════════════════════════════════════════════
  * uiox_shell_start — what the kernel calls today
  *
  * 30_KIX/common/uiox_kernel_main.c:448 calls this from the boot body.
  * The stub it replaces lived at :206; delete that stub and keep this.
  *
  * When the process layer is wired, this function is the ONLY thing that
  * changes: it becomes arch_context_init(&ctx, uiox_sh_main, 0) plus a
  * switch.  uiox_sh_main() itself is untouched — which is the point of
  * building it this way.
  * ═════════════════════════════════════════════════════════════════════ */
 void uiox_shell_start(void)
 {
     (void)uiox_sh_main();      /* does not return */
 }
 