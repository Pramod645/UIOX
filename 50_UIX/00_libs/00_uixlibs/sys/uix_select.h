
#ifndef __SYS_UIX_SELECT__H
#define __SYS_UIX_SELECT__H
/*
sys/select.h 
*/
/* This is for only POXIS */

#include "features.h"

#include <sys/types.h>
#include <time.h>

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

/*
  Number of fds representable in fdset.
  This is the traditional default.
 */
#ifndef FDSETSIZE
#define FDSETSIZE 1024
#endif

/*
  Bitset backing type.
 */
typedef long fdmask;

#define NFDBITS   (8  (int)sizeof(fdmask))
#define FDELT(d) ((d) / NFDBITS)
#define FDMASK(d) ((fdmask)1 << ((d) % NFDBITS))

typedef struct {
    fdmask fdsbits[(FDSETSIZE + NFDBITS - 1) / NFDBITS];
} fdset;

/* fdset manipulation macros */
#define FDZERO(set)                                                      \
    do {                                                                  \
        unsigned int i;                                                 \
        for (i = 0;                                                     \
             i < (sizeof(fdset) / sizeof(fdmask));                    \
             ++i)                                                       \
            (set)->fdsbits[_i] = 0;                                     \
    } while (0)

#define FDSET(fd, set)   ((set)->fdsbits[FDELT(fd)] |= FDMASK(fd))
#define FDCLR(fd, set)   ((set)->fdsbits[FDELT(fd)] &= ~FDMASK(fd))
#define FDISSET(fd, set) (((set)->fdsbits[FDELT(fd)] & FDMASK(fd)) != 0)

/*
  POSIX signal mask type placeholder.
  In a real libc this comes from signal headers.
 */
typedef unsigned long sigsett;

/*
  timeval is traditionally used by select()
 */
#ifndef STRUCTTIMEVAL
#define STRUCTTIMEVAL
struct timeval {
    timet tvsec;
    long   tvusec;
};
#endif

/* Function declarations */
int select(int nfds,
           fdset readfds,
           fdset writefds,
           fdset exceptfds,
           struct timeval timeout);

int pselect(int nfds,
            fdset readfds,
            fdset writefds,
            fdset exceptfds,
            const struct timespec timeout,
            const sigsett sigmask);


#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */

#ifndef UIX_SELECT_H
#define UIX_SELECT_H

#include "uix_types.h"
#include "uix_time.h"
#include "uix_signal.h"
#include "uix_string.h"

#define UIX_FD_SETSIZE 1024   // Maximum file descriptors monitored by select()

typedef struct {
    uix_uint64_t fds_bits[UIX_FD_SETSIZE / 64];
} uix_fd_set;     // Bit array of file descriptors — 64-bit words

#define UIX_FD_ZERO(s)     uix_memset((s), 0, sizeof(uix_fd_set))          // Clears all bits in set
#define UIX_FD_SET(fd,s)   ((s)->fds_bits[(fd)/64] |=  (1ULL<<((fd)%64))) // Sets bit for fd
#define UIX_FD_CLR(fd,s)   ((s)->fds_bits[(fd)/64] &= ~(1ULL<<((fd)%64)))  // Clears bit for fd
#define UIX_FD_ISSET(fd,s) (!!((s)->fds_bits[(fd)/64] & (1ULL<<((fd)%64))))  // Tests if fd is set

int uix_select (int nfds, uix_fd_set *rfds, uix_fd_set *wfds,
                uix_fd_set *efds, uix_timeval_t *timeout);          // Monitors multiple fds for readability/writability — POSIX
int uix_pselect(int nfds, uix_fd_set *rfds, uix_fd_set *wfds,
                uix_fd_set *efds, const uix_timespec_t *timeout,
                const uix_sigset_t *sigmask);                      // Like select() but uses nanosecond timeout and signal mask — POSIX.1-2001

#endif /* UIX_SELECT_H */



#endif /* End of __SYS_UIX_SELECT__H */
/* ***This is End of file, there is no more line should be added after this line*** */