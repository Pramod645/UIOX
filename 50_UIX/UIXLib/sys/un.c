#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define SOCKETPATH "/tmp/demosocket"

int un() {
    int sockfd;
    struct sockaddrun addr;

    // Create a Unix domain socket
    sockfd = socket(AFUNIX, SOCKSTREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXITFAILURE);
    }

    // Remove old socket if it exists
    unlink(SOCKETPATH);

    memset(&addr, 0, sizeof(struct sockaddrun));
    addr.sunfamily = AFUNIX;
    strncpy(addr.sunpath, SOCKETPATH, sizeof(addr.sunpath) - 1);

    // Bind the socket to the file path
    if (bind(sockfd, (struct sockaddr )&addr, sizeof(struct sockaddrun)) < 0) {
        perror("bind");
        close(sockfd);
        exit(EXITFAILURE);
    }

    printf("Socket bound to %s\n", SOCKETPATH);
    close(sockfd);
    return 0;
}
