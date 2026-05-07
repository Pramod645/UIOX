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
///////////////////////
/* src/uix_ftw.c */
#include "uix_ftw.h"
#include "uix_dirent.h"
#include "uix_string.h"
#include "uix_stdlib.h"

static int _ftw_walk(const char *path, uix_ftw_fn fn, int maxfd,
                     int depth)
{
    uix_stat_t sb;
    (void)maxfd;
    if (uix_lstat(path, &sb) < 0) return fn(path, &sb, UIX_FTW_NS);

    if (UIX_S_ISDIR(sb.st_mode)) {
        int r = fn(path, &sb, UIX_FTW_D);
        if (r) return r;
        uix_DIR *d = uix_opendir(path);
        if (!d) return fn(path, &sb, UIX_FTW_DNR);
        uix_dirent_t *de;
        while ((de = uix_readdir(d)) != NULL) {
            if (uix_strcmp(de->d_name,".")==0 ||
                uix_strcmp(de->d_name,"..")==0) continue;
            char buf[UIX_PATH_MAX];
            uix_snprintf(buf, sizeof(buf), "%s/%s", path, de->d_name);
            r = _ftw_walk(buf, fn, maxfd, depth+1);
            if (r) { uix_closedir(d); return r; }
        }
        uix_closedir(d);
        return 0;
    }
    int flag = UIX_S_ISLNK(sb.st_mode) ? UIX_FTW_SL : UIX_FTW_F;
    return fn(path, &sb, flag);
}

int uix_ftw(const char *path, uix_ftw_fn fn, int nopenfd)
{
    return _ftw_walk(path, fn, nopenfd, 0);
}

int uix_nftw(const char *path, uix_nftw_fn fn, int nopenfd, int flags)
{
    (void)nopenfd; (void)flags;
    uix_stat_t sb;
    if (uix_lstat(path, &sb) < 0) {
        uix_FTW_t fw = {0,0};
        return fn(path, &sb, UIX_FTW_NS, &fw);
    }
    uix_FTW_t fw = {(int)uix_strlen(path)+1, 0};
    return fn(path, &sb, UIX_S_ISDIR(sb.st_mode)?UIX_FTW_D:UIX_FTW_F, &fw);
}


