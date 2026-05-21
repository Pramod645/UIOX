#include "../../40_SystemCallInterface/uix_sys.h"

typedef struct { long mtype; char mtext[64]; } tmsg_t;

int main(void)
{
    /* msg queue */
    int msqid = sys_msgget((uix_key_t)0x1234, 0x200|0666);
    tmsg_t m;
    m.mtype=1; m.mtext[0]='H'; m.mtext[1]='i'; m.mtext[2]='\0';
    sys_msgsnd(msqid, &m, 3, 0);
    sys_msgrcv(msqid, &m, 64, 1, 0);
    sys_msgctl(msqid, 0, (void*)0);

    /* shared memory */
    int shmid = sys_shmget((uix_key_t)0x5678, 256, 0x200|0666);
    char *shm = (char*)sys_shmat(shmid, (void*)0, 0);
    if (shm && shm != (char*)-1) {
        shm[0]='U'; shm[1]='I'; shm[2]='X'; shm[3]='\0';
        sys_shmdt(shm);
    }
    sys_shmctl(shmid, 0, (void*)0);

    /* semaphore */
    int semid = sys_semget((uix_key_t)0x9ABC, 1, 0x200|0666);
    uix_sembuf_t sb;
    sb.sem_num=0; sb.sem_op=1; sb.sem_flg=0;
    sys_semop(semid, &sb, 1);
    sb.sem_op=-1;
    sys_semop(semid, &sb, 1);
    sys_semctl(semid, 0, 0);

    return 0;
}
