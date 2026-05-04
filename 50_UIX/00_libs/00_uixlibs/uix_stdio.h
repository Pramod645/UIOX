
#ifndef __UIX_STDIO__H
#define __UIX_STDIO__H
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

#ifndef UIX_STDIO_H
#define UIX_STDIO_H

#include "uix_types.h"
#include "uix_stdarg.h"

#define UIX_EOF      (-1)
#define UIX_BUFSIZ   1024
#define UIX_SEEK_SET 0
#define UIX_SEEK_CUR 1
#define UIX_SEEK_END 2

#define UIX_STDIN_FILENO  0
#define UIX_STDOUT_FILENO 1
#define UIX_STDERR_FILENO 2

typedef struct uix_FILE {
    int        fd;
    int        flags;
    int        error;
    int        eof;
    char      *buffer;
    uix_size_t buf_size;
    uix_size_t buf_pos;
    uix_size_t buf_len;
} uix_FILE;

extern uix_FILE *uix_stdin;
extern uix_FILE *uix_stdout;
extern uix_FILE *uix_stderr;

uix_FILE  *uix_fopen   (const char *path, const char *mode);
int        uix_fclose  (uix_FILE *stream);
int        uix_fflush  (uix_FILE *stream);

int        uix_fgetc   (uix_FILE *stream);
int        uix_fputc   (int c, uix_FILE *stream);
int        uix_getchar (void);
int        uix_putchar (int c);
int        uix_ungetc  (int c, uix_FILE *stream);

char      *uix_fgets   (char *s, int n, uix_FILE *stream);
int        uix_fputs   (const char *s, uix_FILE *stream);
int        uix_puts    (const char *s);

int        uix_printf  (const char *fmt, ...);
int        uix_fprintf (uix_FILE *stream, const char *fmt, ...);
int        uix_sprintf (char *str, const char *fmt, ...);
int        uix_snprintf(char *str, uix_size_t size, const char *fmt, ...);
int        uix_vprintf (const char *fmt, uix_va_list ap);
int        uix_vfprintf(uix_FILE *stream, const char *fmt, uix_va_list ap);
int        uix_vsprintf(char *str, const char *fmt, uix_va_list ap);
int        uix_vsnprintf(char *str, uix_size_t size, const char *fmt, uix_va_list ap);
int        uix_scanf   (const char *fmt, ...);
int        uix_sscanf  (const char *str, const char *fmt, ...);

uix_size_t uix_fread   (void *ptr, uix_size_t size, uix_size_t count, uix_FILE *stream);
uix_size_t uix_fwrite  (const void *ptr, uix_size_t size, uix_size_t count, uix_FILE *stream);

int        uix_fseek   (uix_FILE *stream, uix_off_t offset, int whence);
uix_off_t  uix_ftell   (uix_FILE *stream);
void       uix_rewind  (uix_FILE *stream);

void       uix_clearerr(uix_FILE *stream);
int        uix_feof    (uix_FILE *stream);
int        uix_ferror  (uix_FILE *stream);
void       uix_perror  (const char *s);

#endif /* UIX_STDIO_H */


#endif /* End of __UIX_STDIO__H */
/* ***This is End of file, there is no more line should be added after this line*** */