/* indemo.c — Demonstrating <netinet/in.h> structures */

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>   // for inetpton / inetntop
#include <netinet/in.h>
#include <sys/socket.h>
/*
Typically, when building a socket program:

• <sys/socket.h> → defines socket(), bind(), connect(), etc.  
• <netinet/in.h> → defines struct sockaddrin, AFINET, IPPROTOTCP, etc.  
• <arpa/inet.h> → provides htons(), inetpton(), etc.

They work together like this diagram:

`
socket() ─────────────┐
bind()  ───► sockaddr ┼── uses struct sockaddrin (netinet/in.h)
connect()              │
send()/recv()          │
                       └── IP addresses & ports converted by (arpa/inet.h)
`

*/
int main(void) {
    struct sockaddrin addr;

    /* Clear structure */
    memset(&addr, 0, sizeof(addr));

    addr.sinfamily = AFINET;
    addr.sinport = htons(8080);  // convert to network byte order
    inetpton(AFINET, "192.168.1.100", &addr.sinaddr);

    char ipstr[INETADDRSTRLEN];
    inetntop(AFINET, &addr.sinaddr, ipstr, sizeof(ipstr));

    printf("Address family : %d\n", addr.sinfamily);
    printf("Port (host order): %d\n", ntohs(addr.sinport));
    printf("IP address     : %s\n", ipstr);

    return 0;
}

///////////////////////////////////
/* src/uix_in.c */
#include "uix_in.h"
/* Protocol constants — no runtime code required;
   socket options are handled via uix_setsockopt/uix_getsockopt */


