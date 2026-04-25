
#ifndef __STDIO__H
#define __STDIO__H
/*
<stdio.h> is one of the core components of the C Standard Library. It defines the I/O interface for file and 
stream operations — things like printf(), scanf(), fopen(), fclose(), fread(), and so on.  

This header is part of ISO C (and POSIX by inclusion). 
*/
/* This is for only POXIS */

#include "features.h"

#include <sys/types.h>   // For sizet, ssizet /

#if  (define __POSIX  || define __GLIBC)

#ifdef _cplusplus
extern "C" {
#endif

// Type representing a file stream /
typedef struct FILE FILE;

// Standard streams /
extern FILE stdin;
extern FILE stdout;
extern FILE stderr;

// File positioning /
typedef long fpost;

// Printing and scanning /
int printf(const char format, ...);
int fprintf(FILE stream, const char format, ...);
int sprintf(char str, const char format, ...);
int snprintf(char str, sizet size, const char format, ...);

int scanf(const char format, ...);
int fscanf(FILE stream, const char format, ...);
int sscanf(const char str, const char format, ...);

// File operations /
FILE fopen(const char path, const char mode);
int fclose(FILE stream);
sizet fread(void ptr, sizet size, sizet nmemb, FILE stream);
sizet fwrite(const void ptr, sizet size, sizet nmemb, FILE stream);
int fseek(FILE stream, long offset, int whence);
long ftell(FILE stream);
void rewind(FILE stream);
int fflush(FILE stream);

// Character I/O /
int fgetc(FILE stream);
int getc(FILE stream);
int getchar(void);
int fputc(int c, FILE stream);
int putc(int c, FILE stream);
int putchar(int c);

// String I/O /
char fgets(char s, int n, FILE stream);
int fputs(const char s, FILE stream);

// Error handling /
void clearerr(FILE stream);
int feof(FILE stream);
int ferror(FILE stream);
void perror(const char s);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS and STDLIB*/

#endif /* End of __STDIO__H */
/* ***This is End of file, there is no more line should be added after this line*** */