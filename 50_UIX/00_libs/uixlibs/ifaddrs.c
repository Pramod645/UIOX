#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int ifaddrs(void) {
    struct ifaddrs ifaddr, ifa;
    char addrbuf[INET6ADDRSTRLEN];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return 1;
    }

    printf("Network interfaces and addresses:\n");
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifanext) {
        if (ifa->ifaaddr == NULL)
            continue;

        int family = ifa->ifaaddr->safamily;
        if (family == AFINET || family == AFINET6) {
            void addrptr;
            const char version;

            if (family == AFINET) {
                addrptr = &((struct sockaddrin )ifa->ifaaddr)->sinaddr;
                version = "IPv4";
            } else {
                addrptr = &((struct sockaddrin6 )ifa->ifaaddr)->sin6addr;
                version = "IPv6";
            }

            inetntop(family, addrptr, addrbuf, sizeof(addrbuf));
            printf("%-8s %5s  %s\n", ifa->ifaname, version, addrbuf);
        }
    }

    freeifaddrs(ifaddr);
    return 0;
}
/*
Network interfaces and addresses:
lo        IPv4  127.0.0.1
lo        IPv6  ::1
eth0      IPv4  192.168.1.10
eth0      IPv6  fe80::a00:27ff:fe33:7a11
`

Notes
• ifaddrs vs. net/if.h:  
  - net/if.h + ioctl(SIOCGIFCONF)  → low-level, manual buffer management.  
  - ifaddrs.h + getifaddrs() → automatically allocates a linked list and is much simpler.

• IPv4 and IPv6 support: Built-in and automatic.

• Cleaning up: Always call freeifaddrs() to release memory allocated by getifaddrs().

*/