#include "../../40_SystemCallInterface/uix_sys.h"

int main(void)
{
    int sfd=sys_socket(2,1,0);
    if(sfd<0) sys_exit(1);
    uix_sockaddr_t addr;
    addr.sa_family=2;
    addr.sa_data[0]=(char)(8080>>8);
    addr.sa_data[1]=(char)(8080&0xFF);
    int j; for(j=2;j<14;j++) addr.sa_data[j]=0;
    if(sys_bind(sfd,&addr,sizeof(addr))<0) sys_exit(1);
    if(sys_listen(sfd,5)<0) sys_exit(1);
    uix_socklen_t cl=sizeof(addr);
    int cfd=sys_accept(sfd,&addr,&cl);
    if(cfd>=0) {
        const char *r="HTTP/1.0 200 OK\r\nContent-Length: 3\r\n\r\nUIX";
        sys_sendto(cfd,r,41,0,(void*)0,0);
        sys_shutdown(cfd,2);
    }
    sys_shutdown(sfd,2);
    return 0;
}
