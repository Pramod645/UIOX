
#ifndef __STDLIB__H
#define __STDLIB__H
/*
<stdlib.h> is one of the most important headers in the C Standard Library.  
It provides memory management, process control, numeric conversions, sorting/searching, and various utility functions.

*/
/* This is for only POXIS */

#include "features.h"

#include <stddef.h>   // for sizet, NULL /
#include <sys/types.h>

#if  (define __POSIX  || define __GLIBC)

#ifdef _cplusplus
extern "C" {
#endif

// Numeric conversion /
int    atoi(const char nptr);
long   atol(const char nptr);
long long atoll(const char nptr);
double atof(const char nptr);

long   strtol(const char nptr, char endptr, int base);
unsigned long strtoul(const char nptr, char *endptr, int base);
double strtod(const char nptr, char *endptr);

// Memory management /
void  malloc(sizet size);
void  calloc(sizet nmemb, sizet size);
void  realloc(void ptr, sizet size);
void   free(void ptr);

// Process control /
void   exit(int status);
void   abort(void);
int    atexit(void (func)(void));
int    system(const char command);

// Searching and sorting /
void qsort(void base, sizet nmemb, sizet size,
           int (compar)(const void , const void ));
void bsearch(const void key, const void base, sizet nmemb, sizet size,
              int (compar)(const void , const void ));

// Random numbers /
int rand(void);
void srand(unsigned int seed);

// Absolute value & division /
int abs(int x);
long labs(long x);
divt div(int numerator, int denominator);
ldivt ldiv(long numerator, long denominator);

// divt structure for integer division result /
typedef struct {
    int quot;
    int rem;
} divt;

// ldivt structure for long integer division /
typedef struct {
    long quot;
    long rem;
} ldivt;

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS and STDLIB*/

#ifndef UIX_STDLIB_H
#define UIX_STDLIB_H

#include "uix_types.h"

#define UIX_EXIT_SUCCESS  0
#define UIX_EXIT_FAILURE  1
#define UIX_RAND_MAX      32767

void        *uix_malloc (uix_size_t size);  // Allocates size bytes — internal first-fit allocator on static heap
void        *uix_calloc (uix_size_t nmemb, uix_size_t size); // Allocates n*sz bytes zero-initialized — prevents uninitialized memory reads
void        *uix_realloc(void *ptr, uix_size_t size); // Resizes allocation — copies existing data to new block if needed
void         uix_free   (void *ptr); // Returns memory to heap — coalesces adjacent free blocks

void         uix_exit   (int status) __attribute__((noreturn));  // Calls atexit handlers in reverse order then terminates — POSIX
void         uix_abort  (void)       __attribute__((noreturn));  // Raises SIGABRT, terminates immediately — POSIX
int          uix_atexit (void (*fn)(void));  // Registers function to call at program exit — POSIX, up to 32 handlers
int          uix_system (const char *cmd);

char        *uix_getenv  (const char *name);// Searches environment array for name=value pair
int          uix_setenv  (const char *name, const char *value, int overwrite); // Sets environment variable, overwrites if ow!=0 — POSIX.1-2001
int          uix_unsetenv(const char *name); // Removes environment variable — POSIX.1-2001
int          uix_putenv  (char *string);  // Sets variable from name=value string — POSIX

int          uix_atoi (const char *str);// Converts decimal string to int — no error detection
long         uix_atol (const char *str);
double       uix_atof (const char *str);
long         uix_strtol (const char *str, char **endptr, int base); // Full-featured int conversion with end pointer and base — POSIX
unsigned long uix_strtoul(const char *str, char **endptr, int base);
double       uix_strtod (const char *str, char **endptr); // String to double with end pointer

int          uix_abs (int x); // Absolute value of int
long         uix_labs(long x); // Absolute value of long

int          uix_rand (void); // Linear congruential PRNG — returns 0..RAND_MAX
void         uix_srand(unsigned int seed); // Seeds the PRNG

void         uix_qsort  (void *base, uix_size_t nmemb, uix_size_t size,
                          int (*compar)(const void *, const void *)); // In-place sort — implemented here as insertion sort for simplicity
void        *uix_bsearch(const void *key, const void *base,
                          uix_size_t nmemb, uix_size_t size,
                          int (*compar)(const void *, const void *)); // Binary search in sorted array — returns pointer or NULL

#endif /* UIX_STDLIB_H */


#endif /* End of __STDLIB__H */
/* ***This is End of file, there is no more line should be added after this line*** */