/*********************************  uix_in.c ************************************/
#include "uix_in.h"

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


/* Protocol constants — no runtime code required;
   socket options are handled via uix_setsockopt/uix_getsockopt */

/* ***This is End of file, there is no more line should be added after this line*** */
