// stringdemo.c — Demonstrate common <string.h> functions /

#include <stdio.h>
#include <string.h>

int libstring(void) {
    char src[] = "World";
    char dest[20] = "Hello";

    // Concatenation /
    strcat(dest, ", ");
    strcat(dest, src);
    printf("Concatenated string: %s\n", dest);

    // Length /
    printf("Length of string: %zu\n", strlen(dest));

    // Copy /
    char copy[20];
    strcpy(copy, dest);
    printf("Copied string: %s\n", copy);

    // Comparison /
    if (strcmp(dest, copy) == 0)
        printf("They are equal.\n");

    // Searching /
    char p = strchr(dest, 'W');
    if (p != NULL)
        printf("'W' is at position %ld\n", (long)(p - dest));

    // Memory operations /
    char buffer[10];
    memset(buffer, '-', sizeof(buffer));
    buffer[9] = '\0';
    printf("Buffer after memset: [%s]\n", buffer);

    // Tokenization /
    char text[] = "apple,banana,cherry";
    char token = strtok(text, ",");
    printf("Tokens:\n");
    while (token != NULL) {
        printf("  %s\n", token);
        token = strtok(NULL, ",");
    }

    return 0;
}
