//demo program that creates, writes to, and reads from a shared memory segment.


#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
/*
Shared memory ID: 65538
Written to shared memory: Hello from process using shared memory!
Reading from shared memory: Hello from process using shared memory!
*/
#define SHMKEY 4321
#define SHMSIZE 1024

int shm(void) {
    int shmid;
    void shmaddr;

    // Create a shared memory segment /
    shmid = shmget(SHMKEY, SHMSIZE, IPCCREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXITFAILURE);
    }

    printf("Shared memory ID: %d\n", shmid);

    // Attach the segment to our address space /
    shmaddr = shmat(shmid, NULL, 0);
    if (shmaddr == (void ) -1) {
        perror("shmat");
        exit(EXITFAILURE);
    }

    // Write a message into the shared memory /
    const char message = "Hello from process using shared memory!";
    strncpy((char )shmaddr, message, SHMSIZE);
    printf("Written to shared memory: %s\n", (char )shmaddr);

    // Simulate another process reading /
    printf("Reading from shared memory: %s\n", (char )shmaddr);

    // Detach and remove the shared memory segment /
    if (shmdt(shmaddr) == -1)
        perror("shmdt");

    if (shmctl(shmid, IPC_RMID, NULL) == -1)
        perror("shmctl (remove)");

    return 0;
}
///////////////////////////
/* src/uix_shm.c */
#include "uix_shm.h"
#include "uix_errno.h"

int uix_shmget(uix_key_t key, uix_size_t size, int shmflg)
{
    extern int sys_shmget(uix_key_t,uix_size_t,int) __attribute__((weak));
    if (sys_shmget) return sys_shmget(key, size, shmflg);
    uix_errno = UIX_ENOSYS; return -1;
}

void *uix_shmat(int shmid, const void *shmaddr, int shmflg)
{
    extern void *sys_shmat(int,const void*,int) __attribute__((weak));
    if (sys_shmat) return sys_shmat(shmid, shmaddr, shmflg);
    uix_errno = UIX_ENOSYS; return (void*)-1;
}

int uix_shmdt(const void *shmaddr)
{
    extern int sys_shmdt(const void*) __attribute__((weak));
    if (sys_shmdt) return sys_shmdt(shmaddr);
    uix_errno = UIX_ENOSYS; return -1;
}

int uix_shmctl(int shmid, int cmd, uix_shmid_ds_t *buf)
{
    extern int sys_shmctl(int,int,void*) __attribute__((weak));
    if (sys_shmctl) return sys_shmctl(shmid, cmd, buf);
    uix_errno = UIX_ENOSYS; return -1;
}


