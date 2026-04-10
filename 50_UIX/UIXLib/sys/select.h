
#ifndef __SYS_SELECT__H
#define __SYS_SELECT__H
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

#endif /* End of __SYS_SELECT__H */
/* ***This is End of file, there is no more line should be added after this line*** */