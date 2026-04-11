/* ifdemo.c — Using <net/if.h> to list network interfaces */

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <unistd.h>
/*
Notes
• You can use ioctl() on these structures to query:
  - MTU (SIOCGIFMTU)
  - Flags (SIOCGIFFLAGS)
  - IP address (SIOCGIFADDR)
• Most programs now use higher-level APIs (e.g. getifaddrs() from <ifaddrs.h>) — which are easier and more portable.

*/
int if(void) {
    int fd = socket(AFINET, SOCKDGRAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    struct ifconf ifc;
    char buf[1024];
    ifc.ifclen = sizeof(buf);
    ifc.ifcbuf = buf;

    if (ioctl(fd, SIOCGIFCONF, &ifc) == -1) {
        perror("ioctl SIOCGIFCONF");
        close(fd);
        return 1;
    }

    struct ifreq ifr = ifc.ifcreq;
    int interfaces = ifc.ifclen / sizeof(struct ifreq);
    printf("Found %d network interfaces:\n", interfaces);

    for (int i = 0; i < interfaces; i++) {
        printf("  %s\n", ifr[i].ifrname);
    }

    close(fd);
    return 0;
}