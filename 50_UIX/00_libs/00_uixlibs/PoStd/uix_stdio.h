
#ifndef __UIX_STDIO__H
#define __UIX_STDIO__H
/*
<stdio.h> is one of the core components of the C Standard Library. It defines the I/O interface for file and 
stream operations — things like printf(), scanf(), fopen(), fclose(), fread(), and so on.  

This header is part of ISO C (and POSIX by inclusion). 
*/
/* This is for only POXIS and standerd */

//#include "features.h"


#include "sys/uix_types.h"
#include "uix_stdarg.h"

#define UIX_EOF      (-1)  // End-of-file indicator
#define UIX_BUFSIZ   1024 // Default I/O buffer size
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
} uix_FILE;  // Stream descriptor: fd, flags, buffer, buffer position/length

extern uix_FILE *uix_stdin; // Pre-initialized standard streams on fds 0,1,2
extern uix_FILE *uix_stdout;
extern uix_FILE *uix_stderr;

uix_FILE  *uix_fopen   (const char *path, const char *mode); // Opens file, allocates FILE struct and buffer. Modes: r,w,a,r+,w+,a+
int        uix_fclose  (uix_FILE *stream); // Flushes buffer, closes fd, frees FILE struct
int        uix_fflush  (uix_FILE *stream); // Writes buffered data to kernel — POSIX

int        uix_fgetc   (uix_FILE *stream); // Reads one byte, sets eof flag on EOF
int        uix_fputc   (int c, uix_FILE *stream); // Writes byte to buffer, flushes on newline
int        uix_getchar (void);
int        uix_putchar (int c);
int        uix_ungetc  (int c, uix_FILE *stream);

char      *uix_fgets   (char *s, int n, uix_FILE *stream); // Reads up to n-1 bytes or newline — POSIX recommended over gets()
int        uix_fputs   (const char *s, uix_FILE *stream); //Writes string without adding newline
int        uix_puts    (const char *s); // Writes string plus newline to stdout

int        uix_printf  (const char *fmt, ...);// Formatted output to stdout
int        uix_fprintf (uix_FILE *stream, const char *fmt, ...); // Formatted output to stream
int        uix_sprintf (char *str, const char *fmt, ...); // Formatted output to string
int        uix_snprintf(char *str, uix_size_t size, const char *fmt, ...); // Bounded formatted output — POSIX.1-2001
int        uix_vprintf (const char *fmt, uix_va_list ap);
int        uix_vfprintf(uix_FILE *stream, const char *fmt, uix_va_list ap);
int        uix_vsprintf(char *str, const char *fmt, uix_va_list ap);
int        uix_vsnprintf(char *str, uix_size_t size, const char *fmt, uix_va_list ap); // Core formatter — handles %d,%u,%x,%s,%c,%p,% with width/pad
int        uix_scanf   (const char *fmt, ...); // Formatted input from stdin
int        uix_sscanf  (const char *str, const char *fmt, ...); // Formatted parsing from string

uix_size_t uix_fread   (void *ptr, uix_size_t size, uix_size_t count, uix_FILE *stream); // Reads n items of sz bytes each
uix_size_t uix_fwrite  (const void *ptr, uix_size_t size, uix_size_t count, uix_FILE *stream); // Writes n items of sz bytes each

int        uix_fseek   (uix_FILE *stream, uix_off_t offset, int whence); // Repositions stream — SEEK_SET/CUR/END
uix_off_t  uix_ftell   (uix_FILE *stream); // Returns current stream position
void       uix_rewind  (uix_FILE *stream); // Seeks to beginning and clears error

void       uix_clearerr(uix_FILE *stream); // Clears error and EOF flags
int        uix_feof    (uix_FILE *stream);  // Tests end-of-file flag
int        uix_ferror  (uix_FILE *stream);  // Tests error flag
void       uix_perror  (const char *s); // Prints s: strerror(errno) to stderr



#endif /* End of __UIX_STDIO__H */
/* ***This is End of file, there is no more line should be added after this line*** */
