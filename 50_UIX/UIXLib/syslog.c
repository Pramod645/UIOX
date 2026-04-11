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
