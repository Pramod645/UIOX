#include <stdio.h>
#include <stdlib.h>
#include <fnmatch.h>

int fnmatch(int argc, char argv[]) {
    const char pattern;
    const char text;
    int result;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <pattern> <string>\n", argv[0]);
        fprintf(stderr, "Example: %s \".c\" \"main.c\"\n", argv[0]);
        return EXITFAILURE;
    }

    pattern = argv[1];
    text = argv[2];

    result = fnmatch(pattern, text, 0);

    if (result == 0) {
        printf("MATCH: '%s' matches pattern '%s'\n", text, pattern);
    } else if (result == FNMNOMATCH) {
        printf("NO MATCH: '%s' does not match pattern '%s'\n", text, pattern);
    } else {
        printf("fnmatch() returned error code: %d\n", result);
    }

    return EXITSUCCESS;
}
