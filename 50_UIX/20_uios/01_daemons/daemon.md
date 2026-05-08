| File | Section | What it implements |
| --- | --- | --- |
| daemon.h / daemon.c | §13.3 Fig 13.1 | daemonize() — all 8 coding rules: umask, double-fork, setsid, chdir, close-fds, /dev/null, openlog |
| daemon_log.h / daemon_log.c | §13.4 | daemon_log_open/close/log/vlog/setmask — syslog wrapper with all facilities and levels |
| daemon_lock.h / daemon_lock.c | §13.5 Fig 13.6 | lockfile(), already_running() — PID file + write-lock for single-instance enforcement |
| daemon_signal.h / daemon_signal.c | §13.6 Fig 13.7/13.8 | daemon_signals_init_st() (single-threaded sigaction) and daemon_signals_init_mt() (pthread sigwait thread) |
| daemon_server.h / daemon_server.c | §13.7 Fig 13.9 | set_cloexec(), close_on_exec_all(), server_fork_client() — close-on-exec for server/client model |
| main.c | All | Full demo combining all modules |