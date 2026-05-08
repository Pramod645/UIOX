#ifndef __SYS_UIX_WAIT__H
#define __SYS_UIX_WAIT__H
/*
sys/wait.h in simplified form, plus a matching example source file.
*/
/* This is for only POXIS */

#include "features.h"

#include <sys/types.h>

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

/*
  Options for waitpid()
 */
#define WNOHANG    1
#define WUNTRACED  2
#define WCONTINUED 8

/*
  Macros for interpreting process status
 */
#define WEXITSTATUS(status) (((status) & 0xff00) >> 8)
#define WTERMSIG(status)    ((status) & 0x7f)
#define WSTOPSIG(status)    WEXITSTATUS(status)

#define WIFEXITED(status)   (WTERMSIG(status) == 0)
#define WIFSIGNALED(status) ((((signed char)(((status) & 0x7f) + 1) >> 1) > 0))
#define WIFSTOPPED(status)  (((status) & 0xff) == 0x7f)
#define WIFCONTINUED(status) ((status) == 0xffff)

/*
  Special pid values for waitpid()
  pid > 0  : wait for specific child
  pid == -1: wait for any child
  pid == 0 : wait for any child in same process group
  pid < -1 : wait for any child in process group -pid
 */

/* Function declarations */
pidt wait(int status);
pidt waitpid(pidt pid, int status, int options);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */


#ifndef UIX_WAIT_H
#define UIX_WAIT_H

#include "uix_types.h"

#define UIX_WNOHANG    1     // Return immediately if no child exited
#define UIX_WUNTRACED  2      // Also return if child stopped
#define UIX_WCONTINUED 8

#define UIX_WIFEXITED(s)    (((s)&0x7f)==0)             // True if child exited normally
#define UIX_WEXITSTATUS(s)  (((s)>>8)&0xff)                  // Extract exit code
#define UIX_WIFSIGNALED(s)  (((s)&0x7f)!=0 && ((s)&0x7f)!=0x7f) // True if child killed by signal
#define UIX_WTERMSIG(s)     ((s)&0x7f)                      // Extract signal number that killed child
#define UIX_WIFSTOPPED(s)   (((s)&0xff)==0x7f)
#define UIX_WSTOPSIG(s)     (((s)>>8)&0xff)
#define UIX_WIFCONTINUED(s) ((s)==0xffff)

uix_pid_t uix_wait   (int *wstatus);      // Waits for any child to change state
uix_pid_t uix_waitpid(uix_pid_t pid, int *wstatus, int options);  // Waits for specific child — POSIX
uix_pid_t uix_wait3  (int *wstatus, int options, void *rusage);
uix_pid_t uix_wait4  (uix_pid_t pid, int *wstatus, int options,
                       void *rusage);                            // Like waitpid but also fills rusage — Linux/BSD

#endif /* UIX_WAIT_H */



#endif /* End of __SYS_UIX_WAIT__H */
/* ***This is End of file, there is no more line should be added after this line*** */