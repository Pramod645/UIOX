// pwd.c — Example using <pwd.h> to print user information /

#include <stdio.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

int libpwd(void) {
    uidt uid = getuid();                 // get current user ID
    struct passwd pw = getpwuid(uid);    // look up passwd entry for UID

    if (pw == NULL) {
        perror("getpwuid");
        return 1;
    }

    printf("Username: %s\n", pw->pwname);
    printf("User ID: %d\n", pw->pwuid);
    printf("Group ID: %d\n", pw->pwgid);
    printf("Home Directory: %s\n", pw->pwdir);
    printf("Shell: %s\n", pw->pw_shell);

    return 0;
}
/*
Username: xxxx
User ID: 1000
Group ID: 1000
Home Directory: /home/alice
Shell: /bin/bash
*/