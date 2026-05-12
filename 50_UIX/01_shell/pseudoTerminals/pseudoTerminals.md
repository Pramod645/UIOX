| File | Section | What it implements |
| --- | --- | --- |
| pty.h | §19.1–19.2 | Master header: constants, pty_writen(), utility prototypes |
| pty_open.h/.c | §19.3 Fig 19.9 | ptym_open() (posix_openpt+grantpt+unlockpt+ptsname), ptys_open() (open + Solaris STREAMS push) |
| pty_fork.h/.c | §19.4 Fig 19.10 | pty_fork() — combined fork+setsid+ptys_open+dup2, FreeBSD TIOCSCTTY, termios/winsize init |
| pty_loop.h/.c | §19.5 Fig 19.12 | loop() (two-process I/O pump), tty_raw/reset/atexit(), set_noecho(), pty_writen() |
| pty_driver.h/.c | §19.6 Fig 19.16 | do_driver() (driver subprocess via fd_pipe), fd_pipe() (full-duplex socketpair) |
| pty_main.c | §19.5 Fig 19.11 | main() — option parsing, interactive detection, pty_fork, execvp, loop |
| pty_sigtest.c | Exercise 19.9 | SIGTERM+SIGWINCH signal test via TIOCSIG/TIOCSWINSZ on PTY master |