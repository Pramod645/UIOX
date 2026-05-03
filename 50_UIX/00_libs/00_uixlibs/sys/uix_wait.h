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

#endif /* End of __SYS_UIX_WAIT__H */
/* ***This is End of file, there is no more line should be added after this line*** */