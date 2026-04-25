/*Using POSIX Unnamed Semaphore (seminit, semwait, sempost)

This program creates two threads that safely increment a shared counter by 
using a semaphore for synchronization.
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define NUMTHREADS 2
#define INCREMENTS 1000000

long counter = 0;
semt sem;

void worker(void arg) {
    for (long i = 0; i < INCREMENTS; ++i) {
        semwait(&sem);
        counter++;
        sempost(&sem);
    }
    return NULL;
}

int libsemaphore(void) {
    pthreadt threads[NUMTHREADS];

    // Initialize semaphore with value 1 (acts like a mutex) /
    if (seminit(&sem, 0, 1) != 0) {
        perror("seminit");
        exit(EXITFAILURE);
    }

    // Create threads /
    for (int i = 0; i < NUMTHREADS; ++i) {
        if (pthreadcreate(&threads[i], NULL, worker, NULL) != 0) {
            perror("pthreadcreate");
            exit(EXITFAILURE);
        }
    }

    // Wait for threads to finish /
    for (int i = 0; i < NUMTHREADS; ++i) {
        pthreadjoin(threads[i], NULL);
    }

    semdestroy(&sem);

    printf("Final counter value: %ld\n", counter);
    return 0;
}