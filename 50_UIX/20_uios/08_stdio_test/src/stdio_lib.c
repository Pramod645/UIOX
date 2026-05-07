/*
 * stdio_lib.c
 *
 * Standard I/O Library module.
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
 
 #include "stdio_lib.h"
 
 #include <errno.h>
 #include <string.h>
 #include <stdarg.h>
 #include <sys/stat.h>
 
 /* =============================================================
  * Error helpers
  * ============================================================= */
 
 static void uiox_vwarn_errno(const char *fmt, va_list ap)
 {
     int err = errno;
 
     vfprintf(stderr, fmt, ap);
 
     if (err != 0)
         fprintf(stderr, ": %s", strerror(err));
 
     fputc('\n', stderr);
 }
 
 void uiox_err_sys(const char *fmt, ...)
 {
     va_list ap;
 
     va_start(ap, fmt);
     uiox_vwarn_errno(fmt, ap);
     va_end(ap);
 
     exit(EXIT_FAILURE);
 }
 
 void uiox_err_ret(const char *fmt, ...)
 {
     va_list ap;
 
     va_start(ap, fmt);
     uiox_vwarn_errno(fmt, ap);
     va_end(ap);
 }
 
 /* =============================================================
  * Stream orientation
  * ============================================================= */
 
 int uiox_fwide(FILE *fp, int mode)
 {
     return fwide(fp, mode);
 }
 
 const char *uiox_orientation_name(int fwide_result)
 {
     if (fwide_result > 0)
         return "wide oriented";
 
     if (fwide_result < 0)
         return "byte oriented";
 
     return "unoriented";
 }
 
 /* =============================================================
  * Buffering
  * ============================================================= */
 
 void uiox_setbuf_via_setvbuf(FILE *fp, char *buf)
 {
     if (!fp)
         return;
 
     if (buf)
         (void)setvbuf(fp, buf, _IOFBF, BUFSIZ);
     else
         (void)setvbuf(fp, NULL, _IONBF, 0);
 }
 
 int uiox_setvbuf(FILE *fp, char *buf, int mode, size_t size)
 {
     return setvbuf(fp, buf, mode, size);
 }
 
 int uiox_fflush(FILE *fp)
 {
     return fflush(fp);
 }
 
 int uiox_fflush_fsync(FILE *fp)
 {
     if (!fp) {
         errno = EINVAL;
         return -1;
     }
 
     if (fflush(fp) == EOF)
         return -1;
 
     int fd = fileno(fp);
     if (fd < 0)
         return -1;
 
     return fsync(fd);
 }
 
 void uiox_print_stream_basic(const char *name, FILE *fp)
 {
     if (!fp) {
         printf("stream = %s, invalid stream\n", name ? name : "(null)");
         return;
     }
 
     int fd = fileno(fp);
 
     printf("stream = %s, fd = %d, orientation = %s\n",
            name ? name : "(unnamed)",
            fd,
            uiox_orientation_name(fwide(fp, 0)));
 }
 
 /*
  * This is intentionally best-effort. FILE internals are not portable.
  * It supports common glibc and BSD layouts when visible.
  */
 int uiox_print_stream_buffering(const char *name, FILE *fp)
 {
     if (!fp) {
         errno = EINVAL;
         return -1;
     }
 
     const char *bufmode = "unknown";
     size_t bsz = 0;
 
 #if defined(__GLIBC__) || defined(_IO_EOF_SEEN)
     /*
      * glibc exposes _flags and _IO_buf_base/end in libio FILE.
      * This is nonportable and may break on other libc versions.
      */
 # if defined(_IO_UNBUFFERED)
     if (fp->_flags & _IO_UNBUFFERED)
         bufmode = "unbuffered";
 # endif
 # if defined(_IO_LINE_BUF)
     if (fp->_flags & _IO_LINE_BUF)
         bufmode = "line buffered";
 # endif
     if (strcmp(bufmode, "unknown") == 0)
         bufmode = "fully buffered";
 
     if (fp->_IO_buf_base && fp->_IO_buf_end)
         bsz = (size_t)(fp->_IO_buf_end - fp->_IO_buf_base);
 
 #elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
     /*
      * BSD-like FILE often exposes _flags and _bf._size.
      */
 # if defined(__SNBF)
     if (fp->_flags & __SNBF)
         bufmode = "unbuffered";
 # endif
 # if defined(__SLBF)
     if (fp->_flags & __SLBF)
         bufmode = "line buffered";
 # endif
     if (strcmp(bufmode, "unknown") == 0)
         bufmode = "fully buffered";
 
     bsz = (size_t)fp->_bf._size;
 #else
     errno = ENOSYS;
     return -1;
 #endif
 
     printf("stream = %s, %s, buffer size = %zu, fd = %d\n",
            name ? name : "(unnamed)", bufmode, bsz, fileno(fp));
 
     return 0;
 }
 
 /* =============================================================
  * Opening and closing streams
  * ============================================================= */
 
 FILE *uiox_fopen(const char *pathname, const char *type)
 {
     return fopen(pathname, type);
 }
 
 FILE *uiox_freopen(const char *pathname, const char *type, FILE *fp)
 {
     return freopen(pathname, type, fp);
 }
 
 FILE *uiox_fdopen(int fd, const char *type)
 {
     return fdopen(fd, type);
 }
 
 int uiox_fclose(FILE *fp)
 {
     return fclose(fp);
 }
 
 static bool mode_has_plus(const char *type)
 {
     return strchr(type, '+') != NULL;
 }
 
 int uiox_fopen_type_to_oflags(const char *type, int *oflags)
 {
     if (!type || !oflags || type[0] == '\0') {
         errno = EINVAL;
         return -1;
     }
 
     bool plus = mode_has_plus(type);
 
     switch (type[0]) {
     case 'r':
         *oflags = plus ? O_RDWR : O_RDONLY;
         return 0;
 
     case 'w':
         *oflags = (plus ? O_RDWR : O_WRONLY) | O_CREAT | O_TRUNC;
         return 0;
 
     case 'a':
         *oflags = (plus ? O_RDWR : O_WRONLY) | O_CREAT | O_APPEND;
         return 0;
 
     default:
         errno = EINVAL;
         return -1;
     }
 }
 
 /* =============================================================
  * Character-at-a-time I/O
  * ============================================================= */
 
 int uiox_getc(FILE *fp)
 {
     return getc(fp);
 }
 
 int uiox_fgetc(FILE *fp)
 {
     return fgetc(fp);
 }
 
 int uiox_getchar(void)
 {
     return getchar();
 }
 
 int uiox_putc(int c, FILE *fp)
 {
     return putc(c, fp);
 }
 
 int uiox_fputc(int c, FILE *fp)
 {
     return fputc(c, fp);
 }
 
 int uiox_putchar(int c)
 {
     return putchar(c);
 }
 
 int uiox_ungetc(int c, FILE *fp)
 {
     return ungetc(c, fp);
 }
 
 int uiox_ferror(FILE *fp)
 {
     return ferror(fp);
 }
 
 int uiox_feof(FILE *fp)
 {
     return feof(fp);
 }
 
 void uiox_clearerr(FILE *fp)
 {
     clearerr(fp);
 }
 
 /* =============================================================
  * Line-at-a-time I/O
  * ============================================================= */
 
 char *uiox_fgets(char *buf, int n, FILE *fp)
 {
     return fgets(buf, n, fp);
 }
 
 int uiox_fputs(const char *str, FILE *fp)
 {
     return fputs(str, fp);
 }
 
 int uiox_puts(const char *str)
 {
     return puts(str);
 }
 
 int uiox_copy_getc_putc(FILE *in, FILE *out)
 {
     int c;
 
     if (!in || !out) {
         errno = EINVAL;
         return -1;
     }
 
     while ((c = getc(in)) != EOF) {
         if (putc(c, out) == EOF)
             return -1;
     }
 
     if (ferror(in))
         return -1;
 
     return 0;
 }
 
 int uiox_copy_fgetc_fputc(FILE *in, FILE *out)
 {
     int c;
 
     if (!in || !out) {
         errno = EINVAL;
         return -1;
     }
 
     while ((c = fgetc(in)) != EOF) {
         if (fputc(c, out) == EOF)
             return -1;
     }
 
     if (ferror(in))
         return -1;
 
     return 0;
 }
 
 int uiox_copy_fgets_fputs(FILE *in, FILE *out, size_t maxline)
 {
     if (!in || !out || maxline == 0) {
         errno = EINVAL;
         return -1;
     }
 
     char *buf = malloc(maxline);
     if (!buf)
         return -1;
 
     while (fgets(buf, (int)maxline, in) != NULL) {
         if (fputs(buf, out) == EOF) {
             free(buf);
             return -1;
         }
     }
 
     if (ferror(in)) {
         free(buf);
         return -1;
     }
 
     free(buf);
     return 0;
 }
 
 /* =============================================================
  * Binary I/O
  * ============================================================= */
 
 size_t uiox_fread(void *ptr, size_t size, size_t nobj, FILE *fp)
 {
     return fread(ptr, size, nobj, fp);
 }
 
 size_t uiox_fwrite(const void *ptr, size_t size, size_t nobj, FILE *fp)
 {
     return fwrite(ptr, size, nobj, fp);
 }
 
 int uiox_write_record(FILE *fp, const uiox_binary_record_t *rec)
 {
     if (!fp || !rec) {
         errno = EINVAL;
         return -1;
     }
 
     if (fwrite(rec, sizeof(*rec), 1, fp) != 1)
         return -1;
 
     return 0;
 }
 
 int uiox_read_record(FILE *fp, uiox_binary_record_t *rec)
 {
     if (!fp || !rec) {
         errno = EINVAL;
         return -1;
     }
 
     if (fread(rec, sizeof(*rec), 1, fp) != 1) {
         if (feof(fp))
             return 0;
         return -1;
     }
 
     return 1;
 }
 
 /* =============================================================
  * Stream positioning
  * ============================================================= */
 
 long uiox_ftell(FILE *fp)
 {
     return ftell(fp);
 }
 
 int uiox_fseek(FILE *fp, long offset, int whence)
 {
     return fseek(fp, offset, whence);
 }
 
 void uiox_rewind(FILE *fp)
 {
     rewind(fp);
 }
 
 off_t uiox_ftello(FILE *fp)
 {
     return ftello(fp);
 }
 
 int uiox_fseeko(FILE *fp, off_t offset, int whence)
 {
     return fseeko(fp, offset, whence);
 }
 
 int uiox_fgetpos(FILE *fp, fpos_t *pos)
 {
     return fgetpos(fp, pos);
 }
 
 int uiox_fsetpos(FILE *fp, const fpos_t *pos)
 {
     return fsetpos(fp, pos);
 }
 
 /* =============================================================
  * Formatted I/O wrappers
  * ============================================================= */
 
 int uiox_printf(const char *fmt, ...)
 {
     va_list ap;
     int ret;
 
     va_start(ap, fmt);
     ret = vprintf(fmt, ap);
     va_end(ap);
 
     return ret;
 }
 
 int uiox_fprintf(FILE *fp, const char *fmt, ...)
 {
     va_list ap;
     int ret;
 
     va_start(ap, fmt);
     ret = vfprintf(fp, fmt, ap);
     va_end(ap);
 
     return ret;
 }
 
 int uiox_dprintf(int fd, const char *fmt, ...)
 {
     va_list ap;
     int ret;
 
     va_start(ap, fmt);
     ret = vdprintf(fd, fmt, ap);
     va_end(ap);
 
     return ret;
 }
 
 int uiox_snprintf(char *buf, size_t n, const char *fmt, ...)
 {
     va_list ap;
     int ret;
 
     va_start(ap, fmt);
     ret = vsnprintf(buf, n, fmt, ap);
     va_end(ap);
 
     return ret;
 }
 
 int uiox_vprintf(const char *fmt, va_list ap)
 {
     return vprintf(fmt, ap);
 }
 
 int uiox_vfprintf(FILE *fp, const char *fmt, va_list ap)
 {
     return vfprintf(fp, fmt, ap);
 }
 
 int uiox_vdprintf(int fd, const char *fmt, va_list ap)
 {
     return vdprintf(fd, fmt, ap);
 }
 
 int uiox_vsnprintf(char *buf, size_t n, const char *fmt, va_list ap)
 {
     return vsnprintf(buf, n, fmt, ap);
 }
 
 int uiox_scanf(const char *fmt, ...)
 {
     va_list ap;
     int ret;
 
     va_start(ap, fmt);
     ret = vscanf(fmt, ap);
     va_end(ap);
 
     return ret;
 }
 
 int uiox_fscanf(FILE *fp, const char *fmt, ...)
 {
     va_list ap;
     int ret;
 
     va_start(ap, fmt);
     ret = vfscanf(fp, fmt, ap);
     va_end(ap);
 
     return ret;
 }
 
 int uiox_sscanf(const char *buf, const char *fmt, ...)
 {
     va_list ap;
     int ret;
 
     va_start(ap, fmt);
     ret = vsscanf(buf, fmt, ap);
     va_end(ap);
 
     return ret;
 }
 
 int uiox_vscanf(const char *fmt, va_list ap)
 {
     return vscanf(fmt, ap);
 }
 
 int uiox_vfscanf(FILE *fp, const char *fmt, va_list ap)
 {
     return vfscanf(fp, fmt, ap);
 }
 
 int uiox_vsscanf(const char *buf, const char *fmt, va_list ap)
 {
     return vsscanf(buf, fmt, ap);
 }
 
 /* =============================================================
  * Implementation details
  * ============================================================= */
 
 int uiox_fileno(FILE *fp)
 {
     return fileno(fp);
 }
 
 void uiox_demo_stdio_buffering(void)
 {
     FILE *fp = NULL;
 
     fputs("standard I/O buffering demo\n", stdout);
     fputc('\n', stderr);
 
     (void)uiox_print_stream_buffering("stdin", stdin);
     (void)uiox_print_stream_buffering("stdout", stdout);
     (void)uiox_print_stream_buffering("stderr", stderr);
 
     fp = fopen("/etc/passwd", "r");
     if (fp) {
         (void)getc(fp); /* force buffer allocation */
         if (uiox_print_stream_buffering("/etc/passwd", fp) < 0)
             uiox_print_stream_basic("/etc/passwd", fp);
         fclose(fp);
     } else {
         perror("fopen /etc/passwd");
     }
 }
 
 /* =============================================================
  * Temporary files
  * ============================================================= */
 
 int uiox_demo_tmpnam_tmpfile(void)
 {
     char name[L_tmpnam];
     char line[UIOX_MAXLINE];
     FILE *fp;
 
     /*
      * tmpnam is obsolete-ish and race-prone, but shown because
      * the chapter discusses it.
      */
     printf("tmpnam(NULL): %s\n", tmpnam(NULL));
 
     if (tmpnam(name))
         printf("tmpnam(name): %s\n", name);
 
     fp = tmpfile();
     if (!fp)
         return -1;
 
     fputs("one line of output\n", fp);
     rewind(fp);
 
     if (!fgets(line, sizeof(line), fp)) {
         fclose(fp);
         return -1;
     }
 
     fputs(line, stdout);
     fclose(fp);
     return 0;
 }
 
 int uiox_mkstemp(char *template_buf)
 {
     if (!template_buf) {
         errno = EINVAL;
         return -1;
     }
 
     return mkstemp(template_buf);
 }
 
 char *uiox_mkdtemp(char *template_buf)
 {
     if (!template_buf) {
         errno = EINVAL;
         return NULL;
     }
 
     return 0;//mkdtemp(template_buf);
 }
 
 int uiox_demo_mkstemp_good(void)
 {
     char template_buf[] = "/tmp/uiox_stdio_XXXXXX";
     struct stat sb;
     int fd;
 
     fd = mkstemp(template_buf);
     if (fd < 0)
         return -1;
 
     printf("mkstemp created: %s\n", template_buf);
 
     close(fd);
 
     if (stat(template_buf, &sb) == 0) {
         printf("file exists, mode=%o\n", (unsigned)(sb.st_mode & 0777));
         unlink(template_buf);
     } else if (errno == ENOENT) {
         printf("file does not exist\n");
     } else {
         return -1;
     }
 
     return 0;
 }
 
 void uiox_mkstemp_template_warning(void)
 {
     printf("mkstemp modifies its template in-place.\n");
     printf("Use: char tmpl[] = \"/tmp/nameXXXXXX\";\n");
     printf("Do NOT use: char *tmpl = \"/tmp/nameXXXXXX\";\n");
 }
 
 /* =============================================================
  * Memory streams
  * ============================================================= */
 
 int uiox_demo_fmemopen(void)
 {
 #if defined(__linux__) || defined(_GNU_SOURCE)
     enum { BSZ = 48 };
     FILE *fp;
     char buf[BSZ];
 
     memset(buf, 'a', BSZ - 2);
     buf[BSZ - 2] = '\0';
     buf[BSZ - 1] = 'X';
 
     fp = fmemopen(buf, BSZ, "w+");
     if (!fp)
         return -1;
 
     printf("initial buffer contents: %s\n", buf);
 
     fprintf(fp, "hello, world");
     printf("before flush: %s\n", buf);
 
     fflush(fp);
     printf("after fflush: %s\n", buf);
     printf("len of string in buf = %ld\n", (long)strlen(buf));
 
     memset(buf, 'b', BSZ - 2);
     buf[BSZ - 2] = '\0';
     buf[BSZ - 1] = 'X';
 
     fprintf(fp, "hello, world");
     fseek(fp, 0, SEEK_SET);
     printf("after fseek: %s\n", buf);
     printf("len of string in buf = %ld\n", (long)strlen(buf));
 
     memset(buf, 'c', BSZ - 2);
     buf[BSZ - 2] = '\0';
     buf[BSZ - 1] = 'X';
 
     fprintf(fp, "hello, world");
     fclose(fp);
 
     printf("after fclose: %s\n", buf);
     printf("len of string in buf = %ld\n", (long)strlen(buf));
 
     return 0;
 #else
     errno = ENOSYS;
     return -1;
 #endif
 }
 
 int uiox_build_string_memstream(char **out_buf, size_t *out_size,
                                 const char *prefix, int number)
 {
     if (!out_buf || !out_size) {
         errno = EINVAL;
         return -1;
     }
 
     *out_buf = NULL;
     *out_size = 0;
 
 #if defined(__linux__) || defined(_GNU_SOURCE)
     FILE *fp = open_memstream(out_buf, out_size);
     if (!fp)
         return -1;
 
     fprintf(fp, "%s%d", prefix ? prefix : "", number);
 
     /*
      * Buffer address and size become valid after fflush/fclose.
      */
     if (fflush(fp) == EOF) {
         fclose(fp);
         free(*out_buf);
         *out_buf = NULL;
         *out_size = 0;
         return -1;
     }
 
     fclose(fp);
     return 0;
 #else
     /*
      * Portable fallback using snprintf.
      */
     char tmp[128];
     int n = snprintf(tmp, sizeof(tmp), "%s%d", prefix ? prefix : "", number);
     if (n < 0)
         return -1;
 
     *out_buf = malloc((size_t)n + 1);
     if (!*out_buf)
         return -1;
 
     memcpy(*out_buf, tmp, (size_t)n + 1);
     *out_size = (size_t)n;
     return 0;
 #endif
 }
 
 int uiox_demo_open_memstream(void)
 {
     char *buf = NULL;
     size_t size = 0;
 
     if (uiox_build_string_memstream(&buf, &size, "answer=", 42) < 0)
         return -1;
 
     printf("open_memstream built string: '%s' size=%zu\n", buf, size);
     free(buf);
     return 0;
 }
 