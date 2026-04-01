//can you provide the header file and source code for this wait.h file?
Here’s a Linux-style sys/wait.h in simplified form, plus a matching example source file.

sys/wait.h

``c
#ifndef SYSWAITH
#define SYSWAITH 1

#ifdef _cplusplus
extern "C" {
#endif

#include <sys/types.h>

/
  Options for waitpid()
 /
#define WNOHANG    1
#define WUNTRACED  2
#define WCONTINUED 8

/
  Macros for interpreting process status
  These are Linux-style simplified forms.
 /
#define WEXITSTATUS(status) (((status) & 0xff00) >> 8)
#define WTERMSIG(status)    ((status) & 0x7f)
#define WSTOPSIG(status)    WEXITSTATUS(status)

#define WIFEXITED(status)   (WTERMSIG(status) == 0)
#define WIFSIGNALED(status) ((((signed char)(((status) & 0x7f) + 1) >> 1) > 0))
#define WIFSTOPPED(status)  (((status) & 0xff) == 0x7f)
#define WIFCONTINUED(status) ((status) == 0xffff)

/
  Special pid values for waitpid()
  pid > 0  : wait for specific child
  pid == -1: wait for any child
  pid == 0 : wait for any child in same process group
  pid < -1 : wait for any child in process group -pid
 /

/ Function declarations /
pidt wait(int status);
pidt waitpid(pidt pid, int status, int options);

#ifdef _cplusplus
}
#endif

#endif / SYSWAITH /
`

wait.c

`c
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

Example usage

`c
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>

int main(void)
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
`

If you want, I can continue and give you the next matching header/source pair, like:

• unistd.h
• signal.h
• errno.h
• dirent.h`

so they all stay consistent with the mini Linux-style libc set.