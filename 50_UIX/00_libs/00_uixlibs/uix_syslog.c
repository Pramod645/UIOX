//Here’s syslogdemo.c, a simple demonstration of logging messages:

#include <stdio.h>
#include <syslog.h>
#include <unistd.h>

int libsyslog(void) {
    // Open connection to system logger /
    openlog("syslogdemo", LOGPID | LOGCONS, LOGUSER);

    syslog(LOGINFO, "Program started (PID=%d)", getpid());
    syslog(LOGWARNING, "This is a warning message.");
    syslog(LOGERR, "This is an error message!");
    syslog(LOGDEBUG, "Debugging details: var=%d", 42);

    closelog();
    return 0;
}
/////////////////
/* src/uix_syslog.c */
#include "uix_syslog.h"
#include "uix_stdio.h"
#include "uix_time.h"
#include "uix_unistd.h"
#include "uix_string.h"

static const char *_ident    = "uiox";
static int         _option   = 0;
static int         _facility = UIX_LOG_USER;
static int         _logmask  = 0xFF;

static const char *_level_str(int pri)
{
    switch(pri & 0x07){
    case UIX_LOG_EMERG:   return "EMERG";
    case UIX_LOG_ALERT:   return "ALERT";
    case UIX_LOG_CRIT:    return "CRIT";
    case UIX_LOG_ERR:     return "ERR";
    case UIX_LOG_WARNING: return "WARNING";
    case UIX_LOG_NOTICE:  return "NOTICE";
    case UIX_LOG_INFO:    return "INFO";
    case UIX_LOG_DEBUG:   return "DEBUG";
    default:              return "UNKNOWN";
    }
}

void uix_openlog(const char *ident, int option, int facility)
{
    if (ident)  _ident    = ident;
    _option   = option;
    _facility = facility;
}

void uix_closelog(void) { _ident = "uiox"; }

int uix_setlogmask(int mask) { int old=_logmask; _logmask=mask; return old; }

void uix_vsyslog(int priority, const char *format, uix_va_list ap)
{
    if (!(_logmask & UIX_LOG_MASK(priority & 0x07))) return;
    char msg[1024];
    uix_vsnprintf(msg, sizeof(msg), format, ap);

    if (_option & UIX_LOG_PID)
        uix_fprintf(uix_stderr, "<%s>[%d]: %s: %s\n",
                    _level_str(priority), (int)uix_getpid(), _ident, msg);
    else
        uix_fprintf(uix_stderr, "<%s>%s: %s\n",
                    _level_str(priority), _ident, msg);
}

void uix_syslog(int priority, const char *format, ...)
{
    uix_va_list ap;
    uix_va_start(ap, format);
    uix_vsyslog(priority, format, ap);
    uix_va_end(ap);
}
    

