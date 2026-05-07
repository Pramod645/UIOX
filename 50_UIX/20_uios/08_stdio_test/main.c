/*
 * main.c
 *
 * Demonstration for stdio_lib.h / stdio_lib.c.
 */

 #include "stdio_lib.h"

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 
 static void banner(const char *s)
 {
     printf("\n══════════════════════════════════════════\n");
     printf("  %s\n", s);
     printf("══════════════════════════════════════════\n");
 }
 
 int main(void)
 {
     banner("5.2 Stream orientation");
 
     printf("stdout orientation before byte I/O: %s\n",
            uiox_orientation_name(uiox_fwide(stdout, 0)));
 
     fputs("byte I/O sets byte orientation\n", stdout);
 
     printf("stdout orientation after byte I/O: %s\n",
            uiox_orientation_name(uiox_fwide(stdout, 0)));
 
     banner("5.4 Buffering");
 
     char buf[BUFSIZ];
     FILE *fp = uiox_fopen("/tmp/uiox_stdio_buffer.txt", "w+");
     if (!fp)
         uiox_err_sys("fopen");
 
     uiox_setbuf_via_setvbuf(fp, buf);
     uiox_fprintf(fp, "buffered output line\n");
     uiox_fflush(fp);
     uiox_fclose(fp);
 
     printf("setbuf implemented via setvbuf and flushed successfully\n");
 
     banner("5.5 Opening streams");
 
     int flags;
     const char *modes[] = {"r", "w", "a", "r+", "w+", "a+", NULL};
 
     for (int i = 0; modes[i]; i++) {
         if (uiox_fopen_type_to_oflags(modes[i], &flags) == 0)
             printf("mode %-2s -> open flags 0x%x\n", modes[i], flags);
     }
 
     banner("5.6 Character-at-a-time I/O");
 
     fp = uiox_fopen("/tmp/uiox_stdio_chars.txt", "w+");
     if (!fp)
         uiox_err_sys("fopen chars");
 
     uiox_fputc('A', fp);
     uiox_putc('B', fp);
     uiox_fputc('\n', fp);
 
     uiox_rewind(fp);
 
     int c1 = uiox_fgetc(fp);
     int c2 = uiox_getc(fp);
     printf("read chars: %c %c\n", c1, c2);
 
     uiox_ungetc(c2, fp);
     int c3 = uiox_getc(fp);
     printf("after ungetc: %c\n", c3);
 
     uiox_fclose(fp);
 
     banner("5.7 Line-at-a-time I/O");
 
     fp = uiox_fopen("/tmp/uiox_stdio_lines.txt", "w+");
     if (!fp)
         uiox_err_sys("fopen lines");
 
     uiox_fputs("line one\n", fp);
     uiox_fputs("line two longer than four chars\n", fp);
     uiox_rewind(fp);
 
     char line[UIOX_SMALLLINE];
 
     printf("Reading with MAXLINE=4 equivalent:\n");
     while (uiox_fgets(line, sizeof(line), fp))
         printf("[%s]", line);
     printf("\n");
 
     uiox_fclose(fp);
 
     banner("5.8 Copy using stdio");
 
     FILE *in = uiox_fopen("/tmp/uiox_stdio_lines.txt", "r");
     FILE *out = uiox_fopen("/tmp/uiox_stdio_copy.txt", "w");
 
     if (in && out) {
         uiox_copy_fgets_fputs(in, out, UIOX_MAXLINE);
         uiox_fclose(in);
         uiox_fclose(out);
         printf("copied /tmp/uiox_stdio_lines.txt -> /tmp/uiox_stdio_copy.txt\n");
     }
 
     banner("5.9 Binary I/O");
 
     fp = uiox_fopen("/tmp/uiox_stdio_record.bin", "wb+");
     if (!fp)
         uiox_err_sys("fopen record");
 
     uiox_binary_record_t rec = {
         .id = 7,
         .total = 123456789,
         .name = "record-name"
     };
 
     if (uiox_write_record(fp, &rec) < 0)
         uiox_err_sys("write record");
 
     uiox_rewind(fp);
 
     uiox_binary_record_t rec2;
     memset(&rec2, 0, sizeof(rec2));
 
     if (uiox_read_record(fp, &rec2) < 0)
         uiox_err_sys("read record");
 
     printf("record: id=%d total=%lld name=%s\n",
            rec2.id, (long long)rec2.total, rec2.name);
 
     uiox_fclose(fp);
 
     banner("5.10 Stream positioning");
 
     fp = uiox_fopen("/tmp/uiox_stdio_pos.txt", "w+");
     if (!fp)
         uiox_err_sys("fopen pos");
 
     uiox_fputs("abcdefghijklmnopqrstuvwxyz\n", fp);
 
     printf("ftell after write: %ld\n", uiox_ftell(fp));
 
     uiox_fseek(fp, 5, SEEK_SET);
     printf("ftell after fseek(5): %ld\n", uiox_ftell(fp));
 
     fpos_t pos;
     uiox_fgetpos(fp, &pos);
 
     int ch = uiox_fgetc(fp);
     printf("char at offset 5: %c\n", ch);
 
     uiox_fsetpos(fp, &pos);
     ch = uiox_fgetc(fp);
     printf("same char after fsetpos: %c\n", ch);
 
     uiox_fclose(fp);
 
     banner("5.11 Formatted I/O");
 
     char sbuf[64];
     int needed = uiox_snprintf(sbuf, sizeof(sbuf),
                                "value=%d hex=%#x", 255, 255);
 
     printf("snprintf: '%s' needed=%d\n", sbuf, needed);
 
     int parsed = 0;
     char word[32];
 
     if (uiox_sscanf("123 hello", "%d %31s", &parsed, word) == 2)
         printf("sscanf parsed: %d '%s'\n", parsed, word);
 
     uiox_dprintf(STDOUT_FILENO, "dprintf writes to fd %d\n", STDOUT_FILENO);
 
     banner("5.12 fileno and buffering details");
 
     printf("fileno(stdout) = %d\n", uiox_fileno(stdout));
     uiox_print_stream_basic("stdout", stdout);
 
     if (uiox_print_stream_buffering("stdout", stdout) < 0)
         printf("nonportable FILE internals not available on this libc\n");
 
     banner("5.13 Temporary files");
 
     if (uiox_demo_tmpnam_tmpfile() < 0)
         uiox_err_ret("tmpnam/tmpfile demo failed");
 
     if (uiox_demo_mkstemp_good() < 0)
         uiox_err_ret("mkstemp demo failed");
 
     uiox_mkstemp_template_warning();
 
     banner("5.14 Memory streams");
 
     if (uiox_demo_fmemopen() < 0)
         uiox_err_ret("fmemopen demo unavailable or failed");
 
     if (uiox_demo_open_memstream() < 0)
         uiox_err_ret("open_memstream demo unavailable or failed");
 
     banner("Exercise 5.5: fsync with stdio stream");
 
     fp = uiox_fopen("/tmp/uiox_stdio_fsync.txt", "w");
     if (!fp)
         uiox_err_sys("fopen fsync");
 
     uiox_fputs("must reach kernel before fsync\n", fp);
 
     if (uiox_fflush_fsync(fp) == 0)
         printf("fflush + fsync succeeded\n");
     else
         uiox_err_ret("fflush + fsync failed");
 
     uiox_fclose(fp);
 
     banner("Done");
 
     return 0;
 }
 