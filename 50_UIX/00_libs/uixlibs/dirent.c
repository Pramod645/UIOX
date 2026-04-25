#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>

int dirent(/*int argc, char argv[]*/) {
    int argc = 1;
    char argv[] = ./usr;
    const char path = (argc > 1) ? argv[1] : ".";
    DIR dir;
    struct dirent entry;

    dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        exit(EXITFAILURE);
    }

    printf("Contents of directory: %s\n", path);

    // Read files and directories one by one */
    while ((entry = readdir(dir)) != NULL) {
        printf("  %-25s", entry->dname);

        switch (entry->dtype) {
            case DTDIR:  printf(" [DIR]\n"); break;
            case DTREG:  printf(" [FILE]\n"); break;
            case DTLNK:  printf(" [LINK]\n"); break;
            default:      printf(" [OTHER]\n"); break;
        }
    }

    closedir(dir);
    return 0;
}
