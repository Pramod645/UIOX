#include "sys/resource.h"

#include "sys/types.h"
#include <unistd.h>
#include <sys/syscall.h>

int getrlimit(int resource, struct rlimit rlim)
{
    return syscall(SYSgetrlimit, resource, rlim);
}

int setrlimit(int resource, const struct rlimit rlim)
{
    return syscall(SYSsetrlimit, resource, rlim);
}

int getrusage(int who, struct rusage usage)
{
    return syscall(SYSgetrusage, who, usage);
}

int getpriority(int which, idt who)
{
    return syscall(SYSgetpriority, which, who);
}

int setpriority(int which, idt who, int prio)
{
    return syscall(SYSsetpriority, which, who, prio);
}


/*
Caveats

This is a minimal educational implementation. Real Linux libc differs in a few ways:

• modern libc may use:
  - prlimit64
  - architecture-specific syscall selection
• RLIMINFINITY and rlimt can vary by ABI
• struct rusage layout must exactly match the target system ABI
• getpriority() has slightly awkward kernel/libc error-handling semantics because -1 can be both a valid return value and an error indication

In real libc, getpriority() often clears errno before the syscall and checks it afterward.

A slightly safer version would be:


#include "sys/resource.h"

#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

int getpriority(int which, idt who)
{
    errno = 0;
    return syscall(SYSgetpriority, which, who);
}
*/
/*
include <sys/resource.h>
#include <stdio.h>

int resource(void)
{
    struct rlimit rl;

    if (getrlimit(RLIMITNOFILE, &rl) == 0) {
        printf("soft limit: %lu\n", (unsigned long)rl.rlimcur);
        printf("hard limit: %lu\n", (unsigned long)rl.rlimmax);
    }

    return 0;
}
*/