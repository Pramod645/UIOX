#include "../../40_SystemCallInterface/uix_sys.h"

int main(void)
{
    char buf[512];
    int fd=sys_open("/etc/uix_version",0,0);
    if(fd<0) {
        fd=sys_creat("/etc/uix_version",0644);
        if(fd<0) sys_exit(1);
        sys_write(fd,"UIX 1.0.0\n",10);
        sys_close(fd);
        sys_exit(0);
    }
    uix_ssize_t n;
    while((n=sys_read(fd,buf,512))>0)
        sys_write(1,buf,(uix_size_t)n);
    sys_close(fd);
    return 0;
}
