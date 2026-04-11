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