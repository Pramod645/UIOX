// time.c — Demonstration of using <time.h> /

#include <stdio.h>
#include <time.h>

int libtime(void) {
    timet now;
    struct tm localtime;
    char buffer[100];

    // Get current time (seconds since Unix epoch) /
    time(&now);

    // Convert to local time structure /
    localtime = localtime(&now);

    if (localtime == NULL) {
        perror("localtime");
        return 1;
    }

    // Print readable string /
    printf("Current local time: %s", ctime(&now));

    // Custom formatted output /
    if (strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S %Z", localtime)) {
        printf("Formatted time: %s\n", buffer);
    } else {
        printf("strftime() buffer was too small.\n");
    }

    // Demonstrate clock() usage /
    clockt cputime = clock();
    printf("CPU clock ticks since program start: %ld\n", cputime);

    return 0;
}
/*
Current local time: Thu Apr 25 15:21:34 2024
Formatted time: 2024-04-25 15:21:34 PDT
CPU clock ticks since program start: 1234
`

In summary
• <time.h> defines C and POSIX mechanisms for working with time and date values.
• timet stores absolute time (seconds since epoch).
• struct tm represents broken-down calendar time.
• ctime(), strftime(), and localtime() help convert between formats.
*/