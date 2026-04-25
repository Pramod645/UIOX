//This program displays the available priority range for each policy, runs a simple CPU loop, and voluntarily yields the processor.

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int libsched(void) {
    printf("Scheduling policies and priority ranges:\n");
    printf("SCHEDOTHER : %d - %d\n",
           schedgetprioritymin(SCHEDOTHER),
           schedgetprioritymax(SCHEDOTHER));
    printf("SCHEDFIFO  : %d - %d\n",
           schedgetprioritymin(SCHEDFIFO),
           schedgetprioritymax(SCHEDFIFO));
    printf("SCHEDRR    : %d - %d\n",
           schedgetprioritymin(SCHEDRR),
           schedgetprioritymax(SCHEDRR));

    // Get current policy /
    int policy = schedgetscheduler(0);
    printf("\nCurrent policy: ");
    switch (policy) {
        case SCHEDOTHER: printf("SCHEDOTHER\n"); break;
        case SCHEDFIFO:  printf("SCHEDFIFO\n"); break;
        case SCHEDRR:    printf("SCHEDRR\n");   break;
        default: printf("Unknown (%d)\n", policy);
    }

    // Try to change scheduling (root privileges usually required) /
    struct schedparam param;
    param.schedpriority = schedgetprioritymax(SCHEDRR);

    if (schedsetscheduler(0, SCHEDRR, &param) == -1) {
        perror("schedsetscheduler (requires root privileges)");
    } else {
        printf("Changed scheduling policy to SCHEDRR, priority=%d\n", param.schedpriority);
    }

    // Demonstrate yielding /
    printf("\nRunning a small workload with periodic schedyield()...\n");
    for (int i = 0; i < 5; ++i) {
        printf("Iteration %d (PID=%d)\n", i + 1, getpid());
        schedyield();  / voluntarily yield CPU /
        sleep(1);
    }

    return 0;
}
