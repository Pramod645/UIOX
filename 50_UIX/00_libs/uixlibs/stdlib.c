// stdlibdemo.c — Demonstrate <stdlib.h> functions /

#include <stdio.h>
#include <stdlib.h>

// Comparison function for qsort /
int compareints(const void a, const void b) {
    int x = (const int )a;
    int y = (const int )b;
    return (x > y) - (x < y);
}

int libstdlib(void) {
    // 1. Dynamic memory allocation /
    int n = 5;
    int arr = malloc(n  sizeof(int));
    if (arr == NULL) {
        perror("malloc");
        return 1;
    }

    arr[0] = 42; arr[1] = 7; arr[2] = 13; arr[3] = 100; arr[4] = -5;
    printf("Original array: ");
    for (int i = 0; i < n; i++) printf("%d ", arr[i]);
    printf("\n");

    // 2. Sort with qsort /
    qsort(arr, n, sizeof(int), compareints);
    printf("Sorted array:   ");
    for (int i = 0; i < n; i++) printf("%d ", arr[i]);
    printf("\n");

    // 3. Random numbers /
    srand(1234);  // fixed seed
    printf("Random numbers: %d %d %d\n", rand(), rand(), rand());

    // 4. String to integer conversion /
    const char numstr = "12345";
    long num = strtol(numstr, NULL, 10);
    printf("Converted '%s' to %ld\n", numstr, num);

    // 5. Division results /
    divt d = div(17, 5);
    printf("div(17, 5): quotient = %d, remainder = %d\n", d.quot, d.rem);

    // Free memory /
    free(arr);
    return 0;
}