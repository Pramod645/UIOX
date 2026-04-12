#include "sys/wait.h"

#include <unistd.h>
#include <sys/syscall.h>

pidt wait(int status)
{
    return waitpid(-1, status, 0);
}

pidt waitpid(pidt pid, int status, int options)
{
    return (pidt)syscall(SYSwait4, pid, status, options, 0);
}
`
/*
Notes

In real Linux systems:

• wait() is usually implemented in terms of wait4() or waitid()
• sys/wait.h may also declare:
  - waitid()
  - idtypet
  - siginfot
  - WNOWAIT
  - WCLONE, WALL, WNOTHREAD on Linux
• exact macro definitions can vary slightly by libc

*/

#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>

int libwait(void)
{
    pidt pid = fork();

    if (pid == 0) {
        _exit(42);
    }

    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        printf("child exited with code %d\n", WEXITSTATUS(status));
    }

    return 0;
}
