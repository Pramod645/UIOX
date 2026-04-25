E/* demo Source Code Using sys/ipc.h and ftok()*/

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
/*
Uses ftok() to generate a unique key.
Creates a shared memory segment with shmget().
Prints its ID.
Removes it cleanly with shmctl().
*/
int ipc(void) {
    keyt key;
    int shmid;

    /* Generate a unique key based on a file path and project ID */
    key = ftok("/tmp", 'A');
    if (key == -1) {
        perror("ftok");
        exit(EXITFAILURE);
    }

    /* Create shared memory segment */
    shmid = shmget(key, 1024, IPCCREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXITFAILURE);
    }

    printf("Shared memory segment created:\n");
    printf("  Key: 0x%08x\n", key);
    printf("  ID : %d\n", shmid);

    /* Remove it for cleanup */
    if (shmctl(shmid, IPCRMID, NULL) == -1) {
        perror("shmctl");
        exit(EXITFAILURE);
    }

    return 0;
}
