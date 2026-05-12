
#ifndef UIX_SYSLOG_H
#define UIX_SYSLOG_H
/*
syslog.h header defines the standard UNIX system logging interface.  
It provides the functions and macros used to send messages to the system logger (syslogd or journald on modern systems). 
These messages usually go to /var/log/syslog, /var/log/messages, or the system journal.

*/
/* This is for only POXIS */

//#include "features.h"

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
#define UIX_LOG_EMERG   0              // System is unusable
#define UIX_LOG_ALERT   1              // Action must be taken immediately
#define UIX_LOG_CRIT    2
#define UIX_LOG_ERR     3              // Error conditions
#define UIX_LOG_WARNING 4              // Warning conditions
#define UIX_LOG_NOTICE  5
#define UIX_LOG_INFO    6              // Informational messages
#define UIX_LOG_DEBUG   7              // Debug-level messages

/* Options */
#define UIX_LOG_CONS    0x02        // Write to console if syslog fails
#define UIX_LOG_NDELAY  0x08
#define UIX_LOG_NOWAIT  0x10
#define UIX_LOG_ODELAY  0x04
#define UIX_LOG_PERROR  0x20        // W
#define UIX_LOG_PID     0x01        // Include PID in each message

void uix_openlog (const char *ident, int option, int facility);    // Opens connection to syslog, sets identity string
void uix_closelog(void);                                            // Closes syslog connection
void uix_syslog  (int priority, const char *format, ...);           // Logs formatted message at priority level
void uix_vsyslog (int priority, const char *format, uix_va_list ap); // Like syslog() but with va_list
int  uix_setlogmask(int mask);                                      // Sets which priorities are logged

#define UIX_LOG_MASK(pri)  (1 << (pri))              // Mask for single priority level
#define UIX_LOG_UPTO(pri)  ((1 << ((pri)+1)) - 1)   // Mask for all levels up to and including pri




#endif /* End of UIX_SYSLOG_H */
/* ***This is End of file, there is no more line should be added after this line*** */
