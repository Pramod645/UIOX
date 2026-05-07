#include <stdio.h>
#include <stdlib.h>
#include <glob.h>

int glob(int argc, char argv[]) {
    globt results;
    int ret;
    const char pattern;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pattern>\nExample: %s '.c'\n", argv[0], argv[0]);
        exit(EXITFAILURE);
    }

    pattern = argv[1];

    // Perform globbing /
    ret = glob(pattern, 0, NULL, &results);
    if (ret == GLOBNOMATCH) {
        printf("No matches found for pattern: %s\n", pattern);
        globfree(&results);
        return 0;
    } else if (ret != 0) {
        fprintf(stderr, "glob() failed (error code %d)\n", ret);
        globfree(&results);
        return 1;
    }

    printf("Matches for pattern '%s':\n", pattern);
    for (sizet i = 0; i < results.glpathc; ++i) {
        printf("  %s\n", results.glpathv[i]);
    }

    // Free allocated memory /
    globfree(&results);
    return 0;
}
/////////////////////////////////////
/* src/uix_glob.c */
#include "uix_glob.h"
#include "uix_dirent.h"
#include "uix_fnmatch.h"
#include "uix_string.h"
#include "uix_stdlib.h"
#include "uix_stat.h"

int uix_glob(const char *pattern, int flags,
             int (*errfunc)(const char*, int), uix_glob_t *pglob)
{
    if (!pattern || !pglob) return UIX_GLOB_NOMATCH;
    if (!(flags & UIX_GLOB_APPEND)) {
        pglob->gl_pathc = 0;
        pglob->gl_pathv = NULL;
        pglob->gl_offs  = 0;
    }

    /* Split pattern into dir and file pattern parts */
    char dirpart[UIX_PATH_MAX], filepart[UIX_PATH_MAX];
    const char *last_slash = uix_strrchr(pattern, '/');
    if (last_slash) {
        uix_size_t dl = (uix_size_t)(last_slash - pattern);
        uix_strncpy(dirpart, pattern, dl); dirpart[dl] = '\0';
        uix_strcpy(filepart, last_slash + 1);
    } else {
        uix_strcpy(dirpart, ".");
        uix_strcpy(filepart, pattern);
    }

    uix_DIR *d = uix_opendir(dirpart);
    if (!d) {
        if (errfunc) errfunc(dirpart, 0);
        if (flags & UIX_GLOB_NOCHECK) {
            pglob->gl_pathv = (char**)uix_malloc(2*sizeof(char*));
            pglob->gl_pathv[0] = uix_strdup(pattern);
            pglob->gl_pathv[1] = NULL;
            pglob->gl_pathc = 1;
            return 0;
        }
        return UIX_GLOB_NOMATCH;
    }

    uix_dirent_t *de;
    uix_size_t cap = 16, count = pglob->gl_pathc;
    char **paths = (char**)uix_malloc(cap * sizeof(char*));
    if (pglob->gl_pathv)
        for (uix_size_t i=0;i<count;i++) paths[i]=pglob->gl_pathv[i];

    while ((de = uix_readdir(d)) != NULL) {
        if (de->d_name[0]=='.' && filepart[0]!='.' &&
            !(flags & UIX_GLOB_MARK)) continue;
        if (uix_fnmatch(filepart, de->d_name, 0) != 0) continue;

        char fullpath[UIX_PATH_MAX];
        if (uix_strcmp(dirpart,".") == 0)
            uix_strcpy(fullpath, de->d_name);
        else
            uix_snprintf(fullpath, sizeof(fullpath), "%s/%s",
                         dirpart, de->d_name);

        if (count >= cap) {
            cap *= 2;
            paths = (char**)uix_realloc(paths, cap*sizeof(char*));
        }
        paths[count++] = uix_strdup(fullpath);
    }
    uix_closedir(d);

    if (count == 0 && !(flags & UIX_GLOB_NOCHECK)) {
        uix_free(paths); return UIX_GLOB_NOMATCH;
    }
    paths = (char**)uix_realloc(paths, (count+1)*sizeof(char*));
    paths[count] = NULL;
    uix_free(pglob->gl_pathv);
    pglob->gl_pathv = paths;
    pglob->gl_pathc = count;
    return 0;
}

void uix_globfree(uix_glob_t *pglob)
{
    if (!pglob||!pglob->gl_pathv) return;
    for (uix_size_t i=0; i<pglob->gl_pathc; i++)
        uix_free(pglob->gl_pathv[i]);
    uix_free(pglob->gl_pathv);
    pglob->gl_pathv = NULL;
    pglob->gl_pathc = 0;
}


