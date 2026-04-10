
#ifndef __SEMAPHORE__H
#define __SEMAPHORE__H
/*
semaphore.h defines the POSIX semaphore API, which is used for synchronization between threads or processes.  
Unlike System V semaphores (sys/sem.h), POSIX semaphores are simpler, modern, and often used with threads (via pthread), 
or across processes via named semaphores.

*/
/* This is for only POXIS */

#include "features.h"

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

// Semaphore type /
typedef struct {
    unsigned int value;
} semt;

// Functions for named semaphores (usable between processes) /
#include <fcntl.h>   // for O flags /

semt semopen(const char name, int oflag, ...);
int semclose(semt sem);
int semunlink(const char name);

// Functions for unnamed semaphores (usable within threads or shared memory) /
int seminit(semt sem, int pshared, unsigned int value);
int semdestroy(semt sem);

int semwait(semt sem);
int semtrywait(semt sem);
int sempost(semt sem);
int semgetvalue(semt sem, int sval);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */

#endif /* End of __SEMAPHORE__H */
/* ***This is End of file, there is no more line should be added after this line*** */