#ifndef DAEMON_LOG_H
#define DAEMON_LOG_H

/*
 * daemon_log.h
 *
 * Daemon error-logging interface — based on APUE Section 13.4.
 *
 * The BSD syslog facility provides three ways to deliver log
 * messages:
 *   1. Kernel routines  → /dev/klog
 *   2. User processes   → UNIX domain socket /dev/log
 *   3. Remote hosts     → UDP port 514
 *
 * This module wraps openlog / syslog / closelog / setlogmask
 * and provides convenience macros for common log levels.
 *
 * syslog Levels (highest → lowest priority):
 *   LOG_EMERG   — system is unusable
 *   LOG_ALERT   — action must be taken immediately
 *   LOG_CRIT    — critical condition
 *   LOG_ERR     — error condition
 *   LOG_WARNING — warning condition
 *   LOG_NOTICE  — normal but significant
 *   LOG_INFO    — informational
 *   LOG_DEBUG   — debug message
 *
 * syslog Facilities:
 *   LOG_DAEMON  — system daemons
 *   LOG_USER    — user processes (default)
 *   LOG_AUTH    — authorization programs
 *   LOG_CRON    — cron and at
 *   LOG_LPR     — line printer system
 *   LOG_MAIL    — mail system
 *   LOG_KERN    — kernel messages
 *   LOG_LOCAL0..LOG_LOCAL7 — reserved for local use
 *
 * openlog() Options:
 *   LOG_CONS    — write to console if syslogd unreachable
 *   LOG_NDELAY  — open socket immediately
 *   LOG_NOWAIT  — do not wait for child processes
 *   LOG_ODELAY  — delay open until first message (default)
 *   LOG_PERROR  — also write to stderr
 *   LOG_PID     — include PID in every message
 */

#include <syslog.h>
#include <stdarg.h>

/*
 * daemon_log_open()
 *
 * Opens the syslog connection.  Equivalent to openlog().
 * Calling this is optional — the first call to daemon_log()
 * will open the connection automatically.
 *
 * @param ident    Program name prefixed to every message.
 * @param option   Bitmask of LOG_PID, LOG_CONS, etc.
 * @param facility Default facility (LOG_DAEMON, LOG_USER, ...).
 */
void daemon_log_open(const char *ident, int option, int facility);

/*
 * daemon_log_close()
 *
 * Closes the syslog file descriptor.  Optional.
 */
void daemon_log_close(void);

/*
 * daemon_log()
 *
 * Writes a formatted message to syslog.
 *
 * @param priority  LOG_EMERG | facility, or just level.
 *                  %m in format is replaced with strerror(errno).
 * @param format    printf-style format string.
 */
void daemon_log(int priority, const char *format, ...);

/*
 * daemon_vlog()
 *
 * va_list variant of daemon_log() — wraps vsyslog().
 */
void daemon_vlog(int priority, const char *format, va_list ap);

/*
 * daemon_log_setmask()
 *
 * Sets the log priority mask.  Messages whose priority is not
 * in the mask are silently discarded.  Returns previous mask.
 *
 * Example: daemon_log_setmask(LOG_UPTO(LOG_WARNING))
 *          — log only WARNING and above.
 */
int daemon_log_setmask(int maskpri);

/* ── Convenience macros ──────────────────────────────────── */
#define LOG_EMERG_MSG(fmt, ...)  daemon_log(LOG_EMERG,   fmt, ##__VA_ARGS__)
#define LOG_ALERT_MSG(fmt, ...)  daemon_log(LOG_ALERT,   fmt, ##__VA_ARGS__)
#define LOG_CRIT_MSG(fmt, ...)   daemon_log(LOG_CRIT,    fmt, ##__VA_ARGS__)
#define LOG_ERR_MSG(fmt, ...)    daemon_log(LOG_ERR,     fmt, ##__VA_ARGS__)
#define LOG_WARN_MSG(fmt, ...)   daemon_log(LOG_WARNING, fmt, ##__VA_ARGS__)
#define LOG_NOTICE_MSG(fmt, ...) daemon_log(LOG_NOTICE,  fmt, ##__VA_ARGS__)
#define LOG_INFO_MSG(fmt, ...)   daemon_log(LOG_INFO,    fmt, ##__VA_ARGS__)
#define LOG_DEBUG_MSG(fmt, ...)  daemon_log(LOG_DEBUG,   fmt, ##__VA_ARGS__)

#endif /* DAEMON_LOG_H */
