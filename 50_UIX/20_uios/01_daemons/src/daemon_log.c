#include "../include/daemon_log.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>

/*
 * daemon_log_open()
 *
 * Wrapper around openlog(3).
 *
 * @param ident    Prepended to every log message.
 * @param option   LOG_PID | LOG_CONS | LOG_NDELAY | etc.
 * @param facility LOG_DAEMON | LOG_USER | LOG_LPR | etc.
 *
 * Calling this is optional — syslog() calls it automatically
 * on the first invocation.  Calling it explicitly lets us
 * specify a facility and control when the socket is opened.
 */
void daemon_log_open(const char *ident, int option, int facility)
{
    openlog(ident, option, facility);
}

/*
 * daemon_log_close()
 *
 * Wrapper around closelog(3).
 * Closes the descriptor used to communicate with syslogd.
 * Calling this is optional and rarely necessary.
 */
void daemon_log_close(void)
{
    closelog();
}

/*
 * daemon_log()
 *
 * Formatted log message.  %m in the format string is
 * replaced by strerror(errno) by the underlying syslog(3)
 * implementation on systems that support it (POSIX.1-2008).
 *
 * Example:
 *   daemon_log(LOG_ERR, "open(%s) failed: %m", filename);
 *
 * Equivalent to:
 *   openlog("lpd", LOG_PID, LOG_LPR);
 *   syslog(LOG_ERR, "open error for %s: %m", filename);
 * from Figure 13.3 of the text.
 */
void daemon_log(int priority, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vsyslog(priority, format, ap);
    va_end(ap);
}

/*
 * daemon_vlog()
 *
 * va_list variant — wraps vsyslog(3).
 * Available on all four platforms described in APUE, though
 * not included in the Single UNIX Specification.
 */
void daemon_vlog(int priority, const char *format, va_list ap)
{
    vsyslog(priority, format, ap);
}

/*
 * daemon_log_setmask()
 *
 * Wrapper around setlogmask(3).
 *
 * Sets the priority mask for the process.  Only messages
 * whose priority bit is set in maskpri will be logged.
 *
 * Common usage:
 *   daemon_log_setmask(LOG_UPTO(LOG_NOTICE));
 *   — suppresses LOG_INFO and LOG_DEBUG messages.
 *
 * Note: passing 0 has no effect (POSIX requirement).
 *
 * Returns the previous mask.
 */
int daemon_log_setmask(int maskpri)
{
    return setlogmask(maskpri);
}
