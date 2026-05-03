#include "uix_stdio.h"
#include "uix_stdlib.h"
#include "uix_string.h"
#include "uix_errno.h"
#include "uix_unistd.h"
#include "uix_fcntl.h"
#include <stdarg.h>

/* ── Standard streams ───────────────────────────────────────── */
static uix_FILE _stdin  = { UIX_STDIN_FILENO,  0, 0, 0,
                             NULL, 0, 0, 0 };
static uix_FILE _stdout = { UIX_STDOUT_FILENO, 0, 0, 0,
                             NULL, 0, 0, 0 };
static uix_FILE _stderr = { UIX_STDERR_FILENO, 0, 0, 0,
                             NULL, 0, 0, 0 };

uix_FILE *uix_stdin  = &_stdin;
uix_FILE *uix_stdout = &_stdout;
uix_FILE *uix_stderr = &_stderr;

/* ── File open / close ──────────────────────────────────────── */
uix_FILE *uix_fopen(const char *path, const char *mode)
{
    int flags = 0;
    if (uix_strcmp(mode, "r")  == 0) flags = UIX_O_RDONLY;
    else if (uix_strcmp(mode, "w")  == 0)
        flags = UIX_O_WRONLY | UIX_O_CREAT | UIX_O_TRUNC;
    else if (uix_strcmp(mode, "a")  == 0)
        flags = UIX_O_WRONLY | UIX_O_CREAT | UIX_O_APPEND;
    else if (uix_strcmp(mode, "r+") == 0) flags = UIX_O_RDWR;
    else if (uix_strcmp(mode, "w+") == 0)
        flags = UIX_O_RDWR  | UIX_O_CREAT | UIX_O_TRUNC;
    else if (uix_strcmp(mode, "a+") == 0)
        flags = UIX_O_RDWR  | UIX_O_CREAT | UIX_O_APPEND;
    else { uix_errno = UIX_EINVAL; return NULL; }

    int fd = uix_open(path, flags, 0644);
    if (fd < 0) return NULL;

    uix_FILE *fp = (uix_FILE *)uix_malloc(sizeof(uix_FILE));
    if (!fp) { uix_close(fd); return NULL; }

    fp->fd       = fd;
    fp->flags    = flags;
    fp->error    = 0;
    fp->eof      = 0;
    fp->buffer   = (char *)uix_malloc(UIX_BUFSIZ);
    fp->buf_size = UIX_BUFSIZ;
    fp->buf_pos  = 0;
    fp->buf_len  = 0;
    return fp;
}

int uix_fclose(uix_FILE *stream)
{
    if (!stream) return UIX_EOF;
    uix_fflush(stream);
    int r = uix_close(stream->fd);
    uix_free(stream->buffer);
    uix_free(stream);
    return r;
}

int uix_fflush(uix_FILE *stream)
{
    if (!stream) return 0;
    if (stream->buf_len > 0) {
        uix_write(stream->fd, stream->buffer, stream->buf_len);
        stream->buf_pos = stream->buf_len = 0;
    }
    return 0;
}

/* ── Character I/O ──────────────────────────────────────────── */
int uix_fputc(int c, uix_FILE *stream)
{
    if (!stream) return UIX_EOF;
    if (!stream->buffer) {
        char ch = (char)c;
        return (uix_write(stream->fd, &ch, 1) == 1) ? c : UIX_EOF;
    }
    if (stream->buf_len >= stream->buf_size) uix_fflush(stream);
    stream->buffer[stream->buf_len++] = (char)c;
    if (c == '\n') uix_fflush(stream);
    return c;
}

int uix_putchar(int c)   { return uix_fputc(c, uix_stdout); }

int uix_fgetc(uix_FILE *stream)
{
    if (!stream) return UIX_EOF;
    char c;
    uix_ssize_t n = uix_read(stream->fd, &c, 1);
    if (n == 1) return (unsigned char)c;
    stream->eof = 1;
    return UIX_EOF;
}

int uix_getchar(void) { return uix_fgetc(uix_stdin); }

/* ── String I/O ─────────────────────────────────────────────── */
char *uix_fgets(char *s, int n, uix_FILE *stream)
{
    if (!s || n <= 0 || !stream) return NULL;
    int i = 0;
    while (i < n - 1) {
        int c = uix_fgetc(stream);
        if (c == UIX_EOF) { if (i == 0) return NULL; break; }
        s[i++] = (char)c;
        if (c == '\n') break;
    }
    s[i] = '\0';
    return s;
}

int uix_fputs(const char *s, uix_FILE *stream)
{
    if (!s || !stream) return UIX_EOF;
    uix_size_t len = uix_strlen(s);
    return (uix_write(stream->fd, s, len) == (uix_ssize_t)len) ? 0 : UIX_EOF;
}

int uix_puts(const char *s)
{
    uix_fputs(s, uix_stdout);
    uix_fputc('\n', uix_stdout);
    return 0;
}

/* ── Block I/O ──────────────────────────────────────────────── */
uix_size_t uix_fread(void *ptr, uix_size_t size,
                     uix_size_t count, uix_FILE *stream)
{
    if (!ptr || !stream || size == 0) return 0;
    uix_ssize_t n = uix_read(stream->fd, ptr, size * count);
    if (n <= 0) { stream->eof = 1; return 0; }
    return (uix_size_t)n / size;
}

uix_size_t uix_fwrite(const void *ptr, uix_size_t size,
                      uix_size_t count, uix_FILE *stream)
{
    if (!ptr || !stream || size == 0) return 0;
    uix_ssize_t n = uix_write(stream->fd, ptr, size * count);
    if (n <= 0) return 0;
    return (uix_size_t)n / size;
}

/* ── File positioning ───────────────────────────────────────── */
int uix_fseek(uix_FILE *stream, uix_off_t offset, int whence)
{
    if (!stream) return -1;
    uix_fflush(stream);
    return (uix_lseek(stream->fd, offset, whence) < 0) ? -1 : 0;
}

uix_off_t uix_ftell(uix_FILE *stream)
{
    if (!stream) return -1;
    return uix_lseek(stream->fd, 0, UIX_SEEK_CUR);
}

void uix_rewind(uix_FILE *stream)
{
    if (stream) { uix_fseek(stream, 0, UIX_SEEK_SET);
                  stream->error = 0; }
}

/* ── Error handling ─────────────────────────────────────────── */
void uix_clearerr(uix_FILE *stream) { if (stream) { stream->error = stream->eof = 0; } }
int  uix_feof   (uix_FILE *stream)  { return stream ? stream->eof   : 0; }
int  uix_ferror (uix_FILE *stream)  { return stream ? stream->error : 0; }

void uix_perror(const char *s)
{
    if (s && *s) {
        uix_fputs(s, uix_stderr);
        uix_fputs(": ", uix_stderr);
    }
    uix_fputs(uix_strerror(uix_errno), uix_stderr);
    uix_fputc('\n', uix_stderr);
}

/* ── Formatted output (vsnprintf) ───────────────────────────── */
int uix_vsnprintf(char *str, uix_size_t size,
                  const char *fmt, va_list ap)
{
    uix_size_t pos = 0;
#define OUT(c) do { if (pos < size - 1) str[pos] = (c); pos++; } while(0)

    while (*fmt) {
        if (*fmt != '%') { OUT(*fmt++); continue; }
        fmt++;

        /* Flags */
        int left = 0, zero = 0;
        while (*fmt == '-' || *fmt == '0') {
            if (*fmt == '-') left = 1;
            if (*fmt == '0') zero = 1;
            fmt++;
        }
        /* Width */
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9')
            width = width * 10 + (*fmt++ - '0');

        /* Length modifier */
        int is_long = 0, is_ll = 0;
        if (*fmt == 'l') { fmt++; is_long = 1;
            if (*fmt == 'l') { fmt++; is_ll = 1; } }

        char pad   = zero && !left ? '0' : ' ';
        char buf[64];
        const char *p = NULL;
        uix_size_t  plen = 0;

        switch (*fmt++) {
        case 'd': case 'i': {
            long long val = is_ll ? va_arg(ap, long long) :
                            is_long ? va_arg(ap, long) :
                            va_arg(ap, int);
            int neg = val < 0;
            if (neg) val = -val;
            int i = 63; buf[i] = '\0';
            do { buf[--i] = '0' + (int)(val % 10); val /= 10; } while (val);
            if (neg) buf[--i] = '-';
            p = &buf[i]; plen = uix_strlen(p);
            break;
        }
        case 'u': {
            unsigned long long val = is_ll ? va_arg(ap, unsigned long long) :
                                     is_long ? va_arg(ap, unsigned long) :
                                     va_arg(ap, unsigned int);
            int i = 63; buf[i] = '\0';
            do { buf[--i] = '0' + (int)(val % 10); val /= 10; } while (val);
            p = &buf[i]; plen = uix_strlen(p);
            break;
        }
        case 'x': case 'X': {
            unsigned long long val = is_ll ? va_arg(ap, unsigned long long) :
                                     is_long ? va_arg(ap, unsigned long) :
                                     va_arg(ap, unsigned int);
            const char *hex = (*(fmt-1) == 'x') ? "0123456789abcdef"
                                                 : "0123456789ABCDEF";
            int i = 63; buf[i] = '\0';
            if (!val) buf[--i] = '0';
            else while (val) { buf[--i] = hex[val & 0xF]; val >>= 4; }
            p = &buf[i]; plen = uix_strlen(p);
            break;
        }
        case 'o': {
            unsigned long long val = va_arg(ap, unsigned int);
            int i = 63; buf[i] = '\0';
            do { buf[--i] = '0' + (int)(val & 7); val >>= 3; } while (val);
            p = &buf[i]; plen = uix_strlen(p);
            break;
        }
        case 'c': {
            buf[0] = (char)va_arg(ap, int);
            buf[1] = '\0';
            p = buf; plen = 1;
            break;
        }
        case 's': {
            p = va_arg(ap, const char *);
            if (!p) p = "(null)";
            plen = uix_strlen(p);
            break;
        }
        case 'p': {
            uix_uintptr_t val = (uix_uintptr_t)va_arg(ap, void *);
            const char *hex = "0123456789abcdef";
            int i = 63; buf[i] = '\0';
            if (!val) buf[--i] = '0';
            else while (val) { buf[--i] = hex[val & 0xF]; val >>= 4; }
            buf[--i] = 'x'; buf[--i] = '0';
            p = &buf[i]; plen = uix_strlen(p);
            break;
        }
        case '%': OUT('%'); continue;
        default:  OUT(*(fmt-1)); continue;
        }

        /* Padding */
        if (!left)
            for (uix_size_t i = plen; (int)i < width; i++) OUT(pad);
        for (uix_size_t i = 0; i < plen; i++) OUT(p[i]);
        if (left)
            for (uix_size_t i = plen; (int)i < width; i++) OUT(' ');
    }
    if (size > 0) str[pos < size ? pos : size - 1] = '\0';
    return (int)pos;
#undef OUT
}

int uix_vfprintf(uix_FILE *stream, const char *fmt, va_list ap)
{
    char buf[4096];
    int n = uix_vsnprintf(buf, sizeof(buf), fmt, ap);
    if (n > 0) uix_write(stream->fd, buf, (uix_size_t)n);
    return n;
}

int uix_vprintf(const char *fmt, va_list ap)
{
    return uix_vfprintf(uix_stdout, fmt, ap);
}

int uix_printf(const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    int n = uix_vprintf(fmt, ap);
    va_end(ap);
    return n;
}

int uix_fprintf(uix_FILE *stream, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    int n = uix_vfprintf(stream, fmt, ap);
    va_end(ap);
    return n;
}

int uix_sprintf(char *str, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    int n = uix_vsnprintf(str, (uix_size_t)-1, fmt, ap);
    va_end(ap);
    return n;
}

int uix_snprintf(char *str, uix_size_t size, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    int n = uix_vsnprintf(str, size, fmt, ap);
    va_end(ap);
    return n;
}

/* ── Formatted input (sscanf) ───────────────────────────────── */
int uix_sscanf(const char *str, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int count = 0;
    const char *s = str;

    while (*fmt) {
        if (*fmt != '%') {
            if (*fmt != *s) break;
            fmt++; s++;
            continue;
        }
        fmt++;
        switch (*fmt++) {
        case 'd': {
            int *dest = va_arg(ap, int *);
            while (*s == ' ') s++;
            *dest = uix_atoi(s);
            while (*s == '-' || (*s >= '0' && *s <= '9')) s++;
            count++;
            break;
        }
        case 's': {
            char *dest = va_arg(ap, char *);
            while (*s == ' ') s++;
            while (*s && *s != ' ' && *s != '\t' && *s != '\n')
                *dest++ = *s++;
            *dest = '\0';
            count++;
            break;
        }
        default: break;
        }
    }
    va_end(ap);
    return count;
}

int uix_scanf(const char *fmt, ...)
{
    char buf[1024];
    uix_fgets(buf, sizeof(buf), uix_stdin);
    va_list ap; va_start(ap, fmt);
    int n = uix_sscanf(buf, fmt, ap);
    va_end(ap);
    return n;
}
