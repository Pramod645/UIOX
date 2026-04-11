/* demo Source Code Using signal.h

This program demonstrates how to register a custom signal handler for 
SIGINT (Ctrl‑C) and SIGUSR1, and how to send signals programmatically.
*/
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

// Simple handler function /
void handlesignal(int sig) {
    if (sig == SIGINT)
        printf("\nCaught SIGINT (Ctrl-C). Ignoring gracefully...\n");
    else if (sig == SIGUSR1)
        printf("Caught SIGUSR1 (user-defined signal)\n");
    else
        printf("Caught signal %d\n", sig);
}

int libsignal(void) {
    printf("Process PID: %d\n", getpid());
    printf("Press Ctrl-C to trigger SIGINT, or send SIGUSR1 using:\n");
    printf("    kill -USR1 %d\n\n", getpid());

    // Register signal handlers /
    signal(SIGINT, handlesignal);
    signal(SIGUSR1, handlesignal);

    // Loop forever /
    while (1) {
        printf("Running... (PID=%d)\n", getpid());
        sleep(3);
    }

    return 0;
}
