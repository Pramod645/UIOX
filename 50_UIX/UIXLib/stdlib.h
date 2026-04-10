
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

#endif /* End of __STDLIB__H */
/* ***This is End of file, there is no more line should be added after this line*** */