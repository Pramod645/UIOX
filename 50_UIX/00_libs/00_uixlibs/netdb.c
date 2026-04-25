//netdbdemo.c — Demonstrate <netdb.h> functions /

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int libnetdb(int argc, char argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hostname>\n", argv[0]);
        return 1;
    }

    const char hostname = argv[1];
    struct addrinfo hints, res, p;
    char ipstr[INET6ADDRSTRLEN];

    memset(&hints, 0, sizeof(hints));
    hints.aifamily = AFUNSPEC;    / AFINET or AFINET6 /
    hints.aisocktype = SOCKSTREAM; / Doesn’t matter much here /

    int status = getaddrinfo(hostname, NULL, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gaistrerror(status));
        return 2;
    }

    printf("IP addresses for %s:\n", hostname);

    for (p = res; p != NULL; p = p->ainext) {
        void addr;
        char ipver;

        if (p->aifamily == AFINET) { // IPv4
            struct sockaddrin ipv4 = (struct sockaddrin )p->aiaddr;
            addr = &(ipv4->sinaddr);
            ipver = "IPv4";
        } else if (p->aifamily == AFINET6) { // IPv6
            struct sockaddrin6 ipv6 = (struct sockaddrin6 *)p->aiaddr;
            addr = &(ipv6->sin6addr);
            ipver = "IPv6";
        } else {
            continue;
        }

        inetntop(p->aifamily, addr, ipstr, sizeof(ipstr));
        printf("  %s: %s\n", ipver, ipstr);
    }

    freeaddrinfo(res);
    return 0;
}
/*

Possible output:

`
IP addresses for example.com:
  IPv4: 93.184.216.34
  IPv6: 2606:2800:220:1:248:1893:25c8:1946
`

Notes
• Modern, thread-safe functions:  
  - getaddrinfo() replaces gethostbyname()  
  - getnameinfo() replaces gethostbyaddr()
• Legacy ones (gethostbyname, etc.) are still available but not reentrant.
• Works with both IPv4 and IPv6, depending on aifamily.
*/