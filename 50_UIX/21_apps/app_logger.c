#include "../../40_SystemCallInterface/uix_sys.h"

typedef struct { long mtype; char mtext[256]; } log_msg_t;

int main(void)
{
    int msqid=sys_msgget((uix_key_t)0x4C4F4700, 0x200|0666);
    if(msqid<0) sys_exit(1);
    log_msg_t msg;
    while(1) {
        uix_ssize_t n=sys_msgrcv(msqid,&msg,sizeof(msg.mtext),0,0);
        if(n>0) { sys_write(1,"[LOG] ",6); sys_write(1,msg.mtext,(uix_size_t)n); sys_write(1,"\n",1); }
    }
    return 0;
}
