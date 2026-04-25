/* inetdemo.c — Demonstrate <arpa/inet.h> functions */

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
/*
• <arpa/inet.h> is specific to internet networking (AFINET / AFINET6).  
• For sockets themselves (creating, binding, etc.), you include in addition:
  - <sys/socket.h>  
  - <netinet/in.h>
*/

int inet(void) {
    const char ipstr = "192.168.1.100";
    struct inaddr addr;

    /* Convert text to binary */
    if (inetaton(ipstr, &addr) == 0) {
        fprintf(stderr, "Invalid IP address: %s\n", ipstr);
        return 1;
    }

    printf("IP address (text): %s\n", ipstr);
    printf("IP address (binary, hex): 0x%x\n", ntohl(addr.saddr));

    /* Convert back to dotted-decimal text */
    char convertedback = inetntoa(addr);
    printf("Converted back: %s\n", convertedback);

    /* Modern version using inetpton and inetntop */
    struct inaddr addr2;
    if (inetpton(AFINET, ipstr, &addr2) == 1) {
        char buf[INETADDRSTRLEN];
        inetntop(AFINET, &addr2, buf, sizeof(buf));
        printf("inetpton/inetntop round-trip: %s\n", buf);
    }

    /* Demonstrate byte order functions */
    uint16t port = 8080;
    uint16t netport = htons(port);
    printf("Host port: %u, Network byte order: %u\n", port, netport);

    return 0;
}