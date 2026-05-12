/*
 * pty_main.c — Figure 19.11 (APUE §19.5)
 *
 * The pty program: run any program under a pseudo terminal.
 *
 * Usage: pty [-d driver] [-e] [-i] [-n] [-v] program [arg ...]
 *
 * Options:
 *   -d driver  Run driver program as stdin/stdout (§19.6 Fig 19.16)
 *   -e         No echo on slave PTY line discipline (coprocess mode)
 *   -i         Ignore EOF on stdin (watch background programs)
 *   -n         Force non-interactive mode (don't set raw mode)
 *   -v         Verbose: print slave PTY name to stderr
 *
 * Algorithm:
 *   1. Parse options.
 *   2. If interactive: save termios and winsize of real terminal.
 *   3. Call pty_fork():
 *        parent: gets fdm (PTY master fd) and child PID.
 *        child:  slave PTY is stdin/stdout/stderr.
 *   4. Child:
 *        Optionally set_noecho() on slave.
 *        execvp(argv[optind], ...) — run the target program.
 *   5. Parent:
 *        Optionally print slave name.
 *        If interactive and no driver: tty_raw() + atexit(tty_atexit).
 *        If -d: do_driver() — reconnect stdin/stdout to driver pipe.
 *        loop(fdm, ignoreeof) — I/O copy loop.
 */

 #include "../include/pty.h"
 #include "../include/pty_fork.h"
 #include "../include/pty_loop.h"
 #include "../include/pty_driver.h"
 
 #ifdef __linux__
 /* On Linux, '+' prefix forces POSIX mode: stop at first non-option */
 #define OPTSTR "+d:einv"
 #else
 #define OPTSTR "d:einv"
 #endif
 
 int main(int argc, char *argv[])
 {
     int   fdm, c;
     int   ignoreeof  = 0;
     int   interactive;
     int   noecho     = 0;
     int   verbose    = 0;
     pid_t pid;
     char *driver     = NULL;
     char  slave_name[PTY_SLAVE_NAME_SZ];
     struct termios  orig_termios;
     struct winsize  size;
 
     /*
      * Determine if stdin is a terminal.
      * isatty() returns 1 if fd refers to a terminal device.
      * If stdin is not a terminal (e.g. piped input or /dev/null),
      * we skip raw mode and don't fetch termios/winsize.
      */
     interactive = isatty(STDIN_FILENO);
 
     /* Suppress getopt() writing to stderr on unknown option */
     opterr = 0;
 
     while ((c = getopt(argc, argv, OPTSTR)) != EOF) {
         switch (c) {
         case 'd':
             /* -d driver: use external driver program */
             driver = optarg;
             break;
         case 'e':
             /* -e: disable echo on slave PTY (coprocess mode) */
             noecho = 1;
             break;
         case 'i':
             /* -i: ignore EOF on stdin */
             ignoreeof = 1;
             break;
         case 'n':
             /* -n: force non-interactive */
             interactive = 0;
             break;
         case 'v':
             /* -v: print slave PTY name */
             verbose = 1;
             break;
         case '?':
             fprintf(stderr, "pty: unrecognized option: -%c\n",
                     optopt);
             exit(1);
         }
     }
 
     if (optind >= argc) {
         fprintf(stderr,
                 "usage: pty [-d driver] [-einv] program [arg ...]\n");
         exit(1);
     }
 
     if (interactive) {
         /*
          * Save current terminal settings and window size.
          * These are passed to pty_fork() to initialize the slave
          * with the same settings as the real terminal.
          * This ensures the slave's line discipline has the same
          * special characters (erase, kill, interrupt, etc.).
          */
         if (tcgetattr(STDIN_FILENO, &orig_termios) < 0) {
             perror("tcgetattr error on stdin");
             exit(1);
         }
         if (ioctl(STDIN_FILENO, TIOCGWINSZ, &size) < 0) {
             perror("TIOCGWINSZ error");
             exit(1);
         }
         pid = pty_fork(&fdm, slave_name, sizeof(slave_name),
                        &orig_termios, &size);
     } else {
         /*
          * Non-interactive: don't initialize slave termios/winsize.
          * The slave gets an implementation-defined initial state.
          */
         pid = pty_fork(&fdm, slave_name, sizeof(slave_name),
                        NULL, NULL);
     }
 
     if (pid < 0) {
         perror("pty_fork error");
         exit(1);
 
     } else if (pid == 0) {
         /* ── Child process ──────────────────────────────── */
 
         /*
          * Optionally disable echo on slave PTY.
          * STDIN_FILENO is the slave PTY after pty_fork().
          * This prevents double-echo when driving a coprocess.
          */
         if (noecho)
             set_noecho(STDIN_FILENO);
 
         /*
          * Execute the target program.
          * argv[optind] is the program name; &argv[optind] is the
          * full argv array for the new program.
          * execvp searches PATH for the program.
          * On success, this call never returns.
          */
         if (execvp(argv[optind], &argv[optind]) < 0) {
             fprintf(stderr, "pty: can't execute: %s: %s\n",
                     argv[optind], strerror(errno));
             exit(1);
         }
     }
 
     /* ── Parent process ─────────────────────────────────── */
 
     if (verbose) {
         fprintf(stderr, "slave name = %s\n", slave_name);
         if (driver != NULL)
             fprintf(stderr, "driver = %s\n", driver);
     }
 
     if (interactive && driver == NULL) {
         /*
          * Set the real terminal (beneath pty parent) to raw mode.
          * In raw mode, every keystroke is passed immediately to
          * ptym without local processing — the slave's line
          * discipline handles processing instead.
          * Register tty_atexit() to restore cooked mode on exit.
          */
         if (tty_raw(STDIN_FILENO) < 0) {
             perror("tty_raw error");
             exit(1);
         }
         if (atexit(tty_atexit) < 0) {
             perror("atexit error");
             exit(1);
         }
     }
 
     if (driver) {
         /*
          * Reconnect parent's stdin/stdout to the driver subprocess.
          * After do_driver() returns, the parent's stdin/stdout are
          * the bidirectional pipe ends.
          * loop() will then pump: driver ↔ ptym ↔ slave program.
          */
         do_driver(driver);
     }
 
     /*
      * Main I/O copy loop.
      * Copies stdin→ptym (user/driver input → slave program)
      * and ptym→stdout (slave program output → user/driver).
      * Returns when one side closes.
      */
     loop(fdm, ignoreeof);
 
     exit(0);
 }
 