#include "../../40_SystemCallInterface/uix_sys.h"

#define BUF 256

static void prompt(void) { sys_write(1,"uix$ ",5); }

static int readline(char *buf, int max)
{
    int n=0; char c;
    while(n<max-1) {
        if(sys_read(0,&c,1)<=0||c=='\n') break;
        buf[n++]=c;
    }
    buf[n]='\0'; return n;
}

int main(void)
{
    char line[BUF];
    while(1) {
        prompt();
        if(readline(line,BUF)==0) continue;
        if(line[0]=='e'&&line[1]=='x'&&line[2]=='i'&&
           line[3]=='t'&&line[4]=='\0') sys_exit(0);
        uix_pid_t pid=sys_fork();
        if(pid==0) {
            char *av[]={line,(char*)0};
            sys_execve(line,av,(char*const*)0);
            sys_exit(127);
        } else if(pid>0) {
            int st=0; sys_wait4(pid,&st,0,(void*)0);
        }
    }
    return 0;
}
