//demo shows how to set a file’s access/modification times manually.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <utime.h>
#include <unistd.h>

int libutime(int argc, char argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXITFAILURE);
    }

    const char filename = argv[1];
    struct utimbuf newtimes;

    // Optional: create the file if it doesn't exist /
    FILE fp = fopen(filename, "a");
    if (!fp) {
        perror("fopen");
        exit(EXITFAILURE);
    }
    fclose(fp);

    // Display original modification time /
    printf("Setting custom timestamps for: %s\n", filename);

    // Set to Jan 1, 2000 00:00:00 for example /
    newtimes.actime  = 946684800;  / access time   (Unix epoch seconds) /
    newtimes.modtime = 946684800;  / modification time */

    if (utime(filename, &newtimes) == -1) {
        perror("utime");
        exit(EXITFAILURE);
    }

    printf("File timestamp successfully changed.\n");

    return 0;
}
////////////////////
/* src/uix_utime.c */
#include "uix_utime.h"
#include "uix_errno.h"

int uix_utime(const char *path, const uix_utimbuf_t *times)
{
    extern int sys_utime(const char*, const uix_utimbuf_t*)
        __attribute__((weak));
    if (sys_utime) return sys_utime(path, times);
    uix_errno = UIX_ENOSYS; return -1;
}


