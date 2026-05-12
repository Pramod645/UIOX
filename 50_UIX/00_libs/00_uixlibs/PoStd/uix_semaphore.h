
#ifndef __UIX_SEMAPHORE__H
#define __UIX_SEMAPHORE__H
/*
semaphore.h defines the POSIX semaphore API, which is used for synchronization between threads or processes.  
Unlike System V semaphores (sys/sem.h), POSIX semaphores are simpler, modern, and often used with threads (via pthread), 
or across processes via named semaphores.

*/
/* This is for only POXIS */

//#include "uix_features.h"//??

#include "sys/uix_types.h"
#include "sys/uix_time.h"

typedef struct uix_sem {
    volatile int value;
    int          pshared;
} uix_sem_t;

#define UIX_SEM_FAILED ((uix_sem_t *)-1)

int        uix_sem_init     (uix_sem_t *sem, int pshared, unsigned int value); // Initializes unnamed semaphore with value val
int        uix_sem_destroy  (uix_sem_t *sem);  // Destroys unnamed semaphore
int        uix_sem_wait     (uix_sem_t *sem);  // Decrements semaphore, blocks if zero
int        uix_sem_trywait  (uix_sem_t *sem);  // Non-blocking decrement — returns EAGAIN
int        uix_sem_timedwait(uix_sem_t *sem, const uix_timespec_t *abs);  // Timed decrement with absolute timeout
int        uix_sem_post     (uix_sem_t *sem);   // Increments semaphore, wakes one waiter
int        uix_sem_getvalue (uix_sem_t *sem, int *sval);  // Reads current semaphore value
uix_sem_t *uix_sem_open     (const char *name, int oflag, ...);  // Opens or creates named semaphore
int        uix_sem_close    (uix_sem_t *sem);  // Closes named semaphore handle
int        uix_sem_unlink   (const char *name);  // Removes named semaphore from filesystem



#endif /* End of __UIX_SEMAPHORE__H */
/* ***This is End of file, there is no more line should be added after this line*** */
