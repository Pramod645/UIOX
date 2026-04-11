#include <stdio.h>
#include <grp.h>
#include <sys/types.h>
#include <unistd.h>

int grp(void) {
    gidt gid = getgid();              // current group ID
    struct group gr = getgrgid(gid);  // retrieve group info

    if (gr == NULL) {
        perror("getgrgid");
        return 1;
    }

    printf("Group Name: %s\n", gr->grname);
    printf("Group ID: %d\n", gr->grgid);
    printf("Members: ");

    char *members = gr->gr_mem;
    if (!members || !members) {
        printf("(none)");
    } else {
        for (; members; members++) {
            printf("%s ", members);
        }
    }
    printf("\n");

    return 0;
}