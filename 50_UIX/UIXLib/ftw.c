#include <stdio.h>
#include <stdlib.h>
#include <ftw.h>
#include <string.h>
#include <sys/stat.h>

// Callback function for nftw() /
static int displayinfo(const char fpath, const struct stat sb, int typeflag, struct FTW ftwbuf) {
    (void)sb;       /// Unused in this example /
    (void)ftwbuf;   / Unused here but useful for depth/base info /

    switch (typeflag) {
        case FTWF:
            printf("FILE : %s\n", fpath);
            break;
        case FTWD:
            printf("DIR  : %s\n", fpath);
            break;
        case FTWDNR:
            printf("UNREADABLE DIR: %s\n", fpath);
            break;
        case FTWNS:
            printf("CANNOT STAT : %s\n", fpath);
            break;
        default:
            printf("UNKNOWN : %s\n", fpath);
    }
    return 0;  // 0 tells nftw() to continue /
}

int ftw(int argc, char argv[]) {
    const char path = (argc > 1) ? argv[1] : ".";

    printf("Walking directory tree: %s\n\n", path);

    // nftw allows more control; FTWPHYS avoids following symlinks /
    if (nftw(path, displayinfo, 10, FTWPHYS) == -1) {
        perror("nftw");
        exit(EXITFAILURE);
    }

    return 0;
}
