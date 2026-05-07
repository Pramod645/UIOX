
#ifndef __SYSLOG__H
#define __SYSLOG__H
/*
syslog.h header defines the standard UNIX system logging interface.  
It provides the functions and macros used to send messages to the system logger (syslogd or journald on modern systems). 
These messages usually go to /var/log/syslog, /var/log/messages, or the system journal.

*/
/* This is for only POXIS */

#include "features.h"

#include <stdarg.h>

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

// Priorities (ordered from high to low) /
#define LOGEMERG   0   // system is unusable /
#define LOGALERT   1   // action must be taken immediately /
#define LOGCRIT    2   // critical conditions /
#define LOGERR     3   // error conditions /
#define LOGWARNING 4   // warning conditions /
#define LOGNOTICE  5   // normal but significant condition /
#define LOGINFO    6   // informational /
#define LOGDEBUG   7   // debug-level messages /

// Facilities /
#define LOGKERN     (0<<3)
#define LOGUSER     (1<<3)
#define LOGMAIL     (2<<3)
#define LOGDAEMON   (3<<3)
#define LOGAUTH     (4<<3)
#define LOGSYSLOG   (5<<3)
#define LOGLPR      (6<<3)
#define LOGNEWS     (7<<3)
#define LOGUUCP     (8<<3)
#define LOGCRON     (9<<3)
#define LOGAUTHPRIV (10<<3)
#define LOGLOCAL0   (16<<3)
#define LOGLOCAL1   (17<<3)
#define LOGLOCAL2   (18<<3)
#define LOGLOCAL3   (19<<3)
#define LOGLOCAL4   (20<<3)
#define LOGLOCAL5   (21<<3)
#define LOGLOCAL6   (22<<3)
#define LOGLOCAL7   (23<<3)

// Options for openlog() /
#define LOGPID    0x01  // log the process ID /
#define LOGCONS   0x02  // log on the console if errors /
#define LOGNDELAY 0x08  // open the log immediately /
#define LOGNOWAIT 0x10  // don't wait for console forks /

// Macros to extract parts of the priority /
#define LOGPRI(p)      ((p) & 0x07)
#define LOGFAC(p)      ((p) & ~0x07)
#define LOGMAKEPRI(f, p) (((f) & 0x03f8) | ((p) & 0x07))

// Function prototypes /
void openlog(const char ident, int option, int facility);
void syslog(int priority, const char format, ...);
void closelog(void);
int setlogmask(int maskpri);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */



/* include/uix_syslog.h */
#ifndef UIX_SYSLOG_H
#define UIX_SYSLOG_H

#include "uix_stdarg.h"

/* Facilities */
#define UIX_LOG_KERN    (0<<3)
#define UIX_LOG_USER    (1<<3)
#define UIX_LOG_MAIL    (2<<3)
#define UIX_LOG_DAEMON  (3<<3)
#define UIX_LOG_AUTH    (4<<3)
#define UIX_LOG_SYSLOG  (5<<3)
#define UIX_LOG_LPR     (6<<3)
#define UIX_LOG_NEWS    (7<<3)
#define UIX_LOG_UUCP    (8<<3)
#define UIX_LOG_CRON    (9<<3)
#define UIX_LOG_LOCAL0  (16<<3)
#define UIX_LOG_LOCAL7  (23<<3)

/* Severity levels */
#define UIX_LOG_EMERG   0
#define UIX_LOG_ALERT   1
#define UIX_LOG_CRIT    2
#define UIX_LOG_ERR     3
#define UIX_LOG_WARNING 4
#define UIX_LOG_NOTICE  5
#define UIX_LOG_INFO    6
#define UIX_LOG_DEBUG   7

/* Options */
#define UIX_LOG_CONS    0x02
#define UIX_LOG_NDELAY  0x08
#define UIX_LOG_NOWAIT  0x10
#define UIX_LOG_ODELAY  0x04
#define UIX_LOG_PERROR  0x20
#define UIX_LOG_PID     0x01

void uix_openlog (const char *ident, int option, int facility);
void uix_closelog(void);
void uix_syslog  (int priority, const char *format, ...);
void uix_vsyslog (int priority, const char *format, uix_va_list ap);
int  uix_setlogmask(int mask);

#define UIX_LOG_MASK(pri)  (1 << (pri))
#define UIX_LOG_UPTO(pri)  ((1 << ((pri)+1)) - 1)

#endif /* UIX_SYSLOG_H */





#endif /* End of __SYSLOG__H */
/* ***This is End of file, there is no more line should be added after this line*** */