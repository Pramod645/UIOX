#include "../include/exit_wait.h"
#include "../include/signal.h"

int kernel_exit(int status)
{
    printf("[exit] exiting status=%d\n", status);
    printf("[exit] ignoring all signals\n");
    printf("[exit] closing open files\n");
    printf("[exit] releasing current directory\n");
    printf("[exit] writing accounting record\n");
    printf("[exit] reparenting children to init\n");
    printf("[exit] sending SIGCHLD to parent\n");
    printf("[exit] becoming zombie status=0x%x\n",
           (unsigned int)EXIT_NORMAL(status));
    return 0;
}

int kernel_wait(int *status_ptr)
{
    int has_children = 0;
    printf("[wait] kernel_wait called\n");
    printf("[wait] scanning for zombie children\n");
    printf("[wait] no zombie found yet\n");

    if (!has_children) {
        printf("[wait] ERROR: no child processes\n");
        return -1;
    }

    printf("[wait] sleeping, waiting for child to exit\n");

    if (!has_children) {
        printf("[wait] ERROR: no children remain\n");
        return -1;
    }

    if (status_ptr) *status_ptr = 0;
    return 0;
}
