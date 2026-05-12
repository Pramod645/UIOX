| File | Section | What it implements |
| --- | --- | --- |
| tty.h | §18.2 | Master header: termios overview, includes, constants |
| tty_mode.h/.c | §18.11 Fig 18.20 | tty_raw(), tty_cbreak(), tty_reset(), tty_atexit(), tty_termios(), tty_charsize(), tty_set8bit(), tty_disable_special() |
| tty_id.h/.c | §18.9 Fig 18.12–18.15 | tty_ctermid(), tty_isatty(), tty_ttyname() (full /dev recursive search) |
| tty_pass.h/.c | §18.10 Fig 18.17 | tty_getpass() — canonical mode password reading with echo-off and signal blocking |
| tty_winsize.h/.c | §18.12 Fig 18.22 | tty_get_winsize(), tty_set_winsize(), tty_print_winsize(), tty_watch_winsize() with SIGWINCH handler |