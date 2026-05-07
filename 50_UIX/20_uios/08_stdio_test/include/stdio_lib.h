#ifndef UIOX_STDIO_LIB_H
#define UIOX_STDIO_LIB_H

/*
 * stdio_lib.h
 *
 * Standard I/O Library module .
 *
 * Covers:
 *   5.2  Streams and FILE objects
 *   5.4  Buffering: setbuf, setvbuf, fflush
 *   5.5  Opening streams: fopen, freopen, fdopen, fclose
 *   5.6  Character-at-a-time I/O: getc, fgetc, getchar, putc, fputc
 *   5.7  Line-at-a-time I/O: fgets, fputs
 *   5.8  Standard I/O copy examples
 *   5.9  Binary I/O: fread, fwrite
 *   5.10 Stream positioning: ftell, fseek, ftello, fseeko, fgetpos, fsetpos
 *   5.11 Formatted I/O wrappers
 *   5.12 Implementation details: fileno, buffering diagnostics
 *   5.13 Temporary files: tmpnam, tmpfile, mkstemp, mkdtemp
 *   5.14 Memory streams: fmemopen, open_memstream
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <wchar.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UIOX_MAXLINE
#define UIOX_MAXLINE 4096
#endif

#ifndef UIOX_SMALLLINE
#define UIOX_SMALLLINE 4
#endif

#ifndef UIOX_TEMP_TEMPLATE_LEN
#define UIOX_TEMP_TEMPLATE_LEN 64
#endif

/* =============================================================
 * Stream orientation
 * ============================================================= */

typedef enum uiox_stream_orientation {
    UIOX_STREAM_UNORIENTED = 0,
    UIOX_STREAM_BYTE       = -1,
    UIOX_STREAM_WIDE       = 1
} uiox_stream_orientation_t;

/*
 * Query or request stream orientation.
 *
 * mode < 0 : try byte orientation
 * mode > 0 : try wide orientation
 * mode = 0 : query only
 */
int uiox_fwide(FILE *fp, int mode);

const char *uiox_orientation_name(int fwide_result);

/* =============================================================
 * Buffering
 * ============================================================= */

typedef enum uiox_buffer_mode {
    UIOX_BUF_FULL = _IOFBF,
    UIOX_BUF_LINE = _IOLBF,
    UIOX_BUF_NONE = _IONBF
} uiox_buffer_mode_t;

/*
 * setbuf implemented using setvbuf — Exercise 5.1.
 *
 * If buf != NULL, use full buffering with BUFSIZ bytes.
 * If buf == NULL, disable buffering.
 */
void uiox_setbuf_via_setvbuf(FILE *fp, char *buf);

/*
 * Direct wrapper around setvbuf.
 */
int uiox_setvbuf(FILE *fp, char *buf, int mode, size_t size);

/*
 * Flush one stream, or all output streams if fp == NULL.
 */
int uiox_fflush(FILE *fp);

/*
 * Flush a stdio stream and then fsync its underlying fd.
 * This answers Exercise 5.5.
 */
int uiox_fflush_fsync(FILE *fp);

/*
 * Print buffering mode and fd for a stream.
 *
 * Portable C cannot reliably inspect a FILE internals, so this function
 * reports what can be determined portably and labels the mode using
 * caller-provided expectations.
 */
void uiox_print_stream_basic(const char *name, FILE *fp);

/*
 * Best-effort nonportable inspection for glibc/BSD-like systems.
 * Returns:
 *   0 on success,
 *  -1 if unsupported.
 */
int uiox_print_stream_buffering(const char *name, FILE *fp);

/* =============================================================
 * Opening and closing streams
 * ============================================================= */

FILE *uiox_fopen(const char *pathname, const char *type);

FILE *uiox_freopen(const char *pathname, const char *type, FILE *fp);

FILE *uiox_fdopen(int fd, const char *type);

int uiox_fclose(FILE *fp);

/*
 * Convert fopen type string into POSIX open flags.
 *
 * Supports:
 *   r, rb, w, wb, a, ab,
 *   r+, r+b, rb+,
 *   w+, w+b, wb+,
 *   a+, a+b, ab+
 *
 * Returns 0 on success and stores flags in *oflags.
 * Returns -1 on invalid type.
 */
int uiox_fopen_type_to_oflags(const char *type, int *oflags);

/* =============================================================
 * Character-at-a-time I/O
 * ============================================================= */

int uiox_getc(FILE *fp);
int uiox_fgetc(FILE *fp);
int uiox_getchar(void);

int uiox_putc(int c, FILE *fp);
int uiox_fputc(int c, FILE *fp);
int uiox_putchar(int c);

int uiox_ungetc(int c, FILE *fp);

int uiox_ferror(FILE *fp);
int uiox_feof(FILE *fp);
void uiox_clearerr(FILE *fp);

/* =============================================================
 * Line-at-a-time I/O
 * ============================================================= */

char *uiox_fgets(char *buf, int n, FILE *fp);

/*
 * Intentionally no gets wrapper. gets is unsafe and removed from C11.
 */
int uiox_fputs(const char *str, FILE *fp);
int uiox_puts(const char *str);

/* Copy stdin to stdout using getc/putc — Figure 5.4. */
int uiox_copy_getc_putc(FILE *in, FILE *out);

/* Copy using fgetc/fputc. */
int uiox_copy_fgetc_fputc(FILE *in, FILE *out);

/* Copy using fgets/fputs — Figure 5.5. */
int uiox_copy_fgets_fputs(FILE *in, FILE *out, size_t maxline);

/* =============================================================
 * Binary I/O
 * ============================================================= */

size_t uiox_fread(void *ptr, size_t size, size_t nobj, FILE *fp);

size_t uiox_fwrite(const void *ptr, size_t size, size_t nobj, FILE *fp);

/*
 * Example binary record used in demonstrations.
 * Note: binary layout is not portable across architectures/compilers.
 */
typedef struct uiox_binary_record {
    int32_t id;
    int64_t total;
    char    name[32];
} uiox_binary_record_t;

int uiox_write_record(FILE *fp, const uiox_binary_record_t *rec);

int uiox_read_record(FILE *fp, uiox_binary_record_t *rec);

/* =============================================================
 * Stream positioning
 * ============================================================= */

long uiox_ftell(FILE *fp);
int  uiox_fseek(FILE *fp, long offset, int whence);
void uiox_rewind(FILE *fp);

off_t uiox_ftello(FILE *fp);
int   uiox_fseeko(FILE *fp, off_t offset, int whence);

int uiox_fgetpos(FILE *fp, fpos_t *pos);
int uiox_fsetpos(FILE *fp, const fpos_t *pos);

/* =============================================================
 * Formatted I/O wrappers
 * ============================================================= */

int uiox_printf(const char *fmt, ...);
int uiox_fprintf(FILE *fp, const char *fmt, ...);
int uiox_dprintf(int fd, const char *fmt, ...);
int uiox_snprintf(char *buf, size_t n, const char *fmt, ...);

int uiox_vprintf(const char *fmt, va_list ap);
int uiox_vfprintf(FILE *fp, const char *fmt, va_list ap);
int uiox_vdprintf(int fd, const char *fmt, va_list ap);
int uiox_vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);

int uiox_scanf(const char *fmt, ...);
int uiox_fscanf(FILE *fp, const char *fmt, ...);
int uiox_sscanf(const char *buf, const char *fmt, ...);

int uiox_vscanf(const char *fmt, va_list ap);
int uiox_vfscanf(FILE *fp, const char *fmt, va_list ap);
int uiox_vsscanf(const char *buf, const char *fmt, va_list ap);

/* =============================================================
 * Implementation details
 * ============================================================= */

int uiox_fileno(FILE *fp);

/*
 * Show stdin/stdout/stderr and one regular file buffering state.
 */
void uiox_demo_stdio_buffering(void);

/* =============================================================
 * Temporary files
 * ============================================================= */

/*
 * Demonstrates tmpnam and tmpfile.
 * tmpnam is included because the chapter discusses it, but new code
 * should prefer tmpfile, mkstemp, or mkdtemp.
 */
int uiox_demo_tmpnam_tmpfile(void);

/*
 * Create a temporary file using mkstemp.
 *
 * template_buf must be mutable and end with "XXXXXX".
 * On success:
 *   - returns fd
 *   - template_buf is modified to the actual pathname
 */
int uiox_mkstemp(char *template_buf);

/*
 * Create a temporary directory using mkdtemp.
 *
 * template_buf must be mutable and end with "XXXXXX".
 * On success:
 *   - returns template_buf
 *   - template_buf is modified to the actual directory pathname
 */
char *uiox_mkdtemp(char *template_buf);

/*
 * Demonstrate correct mkstemp usage with mutable template.
 */
int uiox_demo_mkstemp_good(void);

/*
 * DO NOT call with a string literal. This function documents why:
 * mkstemp modifies its template in place.
 */
void uiox_mkstemp_template_warning(void);

/* =============================================================
 * Memory streams
 * ============================================================= */

/*
 * Demonstrate fmemopen behavior like Figure 5.15.
 *
 * Requires POSIX.1-2008 / glibc-like support.
 */
int uiox_demo_fmemopen(void);

/*
 * Create a string using open_memstream.
 *
 * Caller owns returned *out_buf and must free it.
 */
int uiox_build_string_memstream(char **out_buf, size_t *out_size,
                                const char *prefix, int number);

/*
 * Demonstrate open_memstream behavior.
 */
int uiox_demo_open_memstream(void);

/* =============================================================
 * Error helpers
 * ============================================================= */

void uiox_err_sys(const char *fmt, ...);

void uiox_err_ret(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* UIOX_STDIO_LIB_H */
