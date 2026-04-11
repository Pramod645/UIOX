//a basic multithreading example with two threads incrementing a shared counter safely using a mutex.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUMTHREADS 2
#define INCREMENTS 1000000

long counter = 0;
pthreadmutext lock;

void increment(void arg) {
    for (long i = 0; i < INCREMENTS; ++i) {
        pthreadmutexlock(&lock);
        counter++;
        pthreadmutexunlock(&lock);
    }
    return NULL;
}

int libpthread(void) {
    pthreadt threads[NUMTHREADS];

    // Initialize the mutex /
    if (pthreadmutexinit(&lock, NULL) != 0) {
        perror("pthreadmutexinit");
        exit(EXITFAILURE);
    }

    // Create threads /
    for (int i = 0; i < NUMTHREADS; ++i) {
        if (pthreadcreate(&threads[i], NULL, increment, NULL) != 0) {
            perror("pthreadcreate");
            exit(EXITFAILURE);
        }
    }

    // Wait for all threads to complete /
    for (int i = 0; i < NUMTHREADS; ++i) {
        pthreadjoin(threads[i], NULL);
    }

    // Destroy the mutex */
    pthreadmutexdestroy(&lock);

    printf("Final counter value: %ld\n", counter);
    return 0;
}
