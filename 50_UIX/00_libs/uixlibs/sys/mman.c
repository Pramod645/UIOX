/* demo Source Code Using sys/mman.h */

//Here’s a simple program that creates a shared memory region using mmap():

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int mman() {
    const char shmname = "/demoshm";
    const sizet SIZE = 4096;

    /* Create shared memory object */
    int fd = shmopen(shmname, OCREAT | ORDWR, 0666);
    if (fd == -1) {
        perror("shmopen");
        return 1;
    }

    /* Set size of shared memory */
    if (ftruncate(fd, SIZE) == -1) {
        perror("ftruncate");
        return 1;
    }

    /* Map shared memory */
    void ptr = mmap(0, SIZE, PROTREAD | PROTWRITE, MAPSHARED, fd, 0);
    if (ptr == MAPFAILED) {
        perror("mmap");
        return 1;
    }

    /* Write to shared memory */
    sprintf(ptr, "Hello from shared memory!");
    printf("Written: %s\n", (char )ptr);

    /* Unmap and clean up */
    munmap(ptr, SIZE);
    close(fd);
    shmunlink(shmname);

    return 0;
}