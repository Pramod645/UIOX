#ifndef __SYS_UIX_WAIT__H
#define __SYS_UIX_WAIT__H
/*
sys/wait.h in simplified form.
*/
/* This is for only POXIS */

#include "uix_features.h"//??

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

#endif /* End of __SYS_UIX_WAIT__H */
/* ***This is End of file, there is no more line should be added after this line*** */
