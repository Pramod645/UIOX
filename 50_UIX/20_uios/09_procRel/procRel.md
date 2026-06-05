| File | Section | What it implements |
| --- | --- | --- |
| proc_group.h/.c | §9.4 | getpgrp, setpgid, process group leader detection, race-condition-free fork+setpgid pattern |
| proc_session.h/.c | §9.5–9.7 | setsid, getsid, tcgetpgrp, tcsetpgrp, tcgetsid, /dev/tty, Exercise 9.2 |
| proc_job.h/.c | §9.8–9.9 | Job table, foreground/background job launch, SIGCONT, SIGTTIN, SIGTTOU, terminal handoff |
| proc_orphan.h/.c | §9.10 | Figure 9.12 — orphaned process group, SIGHUP+SIGCONT mechanism, EIO on terminal read |