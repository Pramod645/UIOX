/*
 * main.c — Exercises all functions from APUE Chapter 18.
 *
 * Demonstrates:
 *   §18.9  tty_ctermid(), tty_isatty(), tty_ttyname()
 *   §18.10 tty_getpass()
 *   §18.11 tty_cbreak(), tty_raw(), tty_reset()
 *   §18.12 tty_watch_winsize() (press Ctrl-C to exit)
 */

 #include "../include/tty.h"
 #include "../include/tty_mode.h"
 #include "../include/tty_id.h"
 #include "../include/tty_pass.h"
 #include "../include/tty_winsize.h"
 
 static void sig_catch(int signo)
 {
     printf("\nsignal %d caught\n", signo);
     tty_reset(STDIN_FILENO);
     exit(0);
 }
 
 int main(int argc, char *argv[])
 {
     (void)argc; (void)argv;
     char  c;
     int   i;
     char *pw;
     char  ctbuf[L_ctermid];
 
     /* ── §18.9 Terminal Identification ─────────────────── */
     printf("=== Terminal Identification ===\n");
     printf("ctermid: %s\n", tty_ctermid(ctbuf));
     printf("fd 0: %s\n", tty_isatty(0) ? "tty" : "not a tty");
     printf("fd 1: %s\n", tty_isatty(1) ? "tty" : "not a tty");
     printf("fd 2: %s\n", tty_isatty(2) ? "tty" : "not a tty");
 
     if (tty_isatty(0)) {
         char *name = tty_ttyname(0);
         printf("fd 0 name: %s\n", name ? name : "(null)");
     }
 
     /* ── §18.11 Character size (mask demo) ──────────────── */
     printf("\n=== Character size ===\n");
     tty_charsize(STDIN_FILENO);
 
     /* ── §18.10 getpass ─────────────────────────────────── */
     printf("\n=== getpass test ===\n");
     pw = tty_getpass("Enter test password: ");
     if (pw) {
         printf("You entered: %s\n", pw);
         /* Zero out cleartext when done */
         while (*pw) *pw++ = '\0';
     }
 
     /* ── §18.11 Raw mode test ────────────────────────────── */
     printf("\n=== Raw mode (type chars, DELETE to exit) ===\n");
     signal(SIGINT,  sig_catch);
     signal(SIGQUIT, sig_catch);
     signal(SIGTERM, sig_catch);
 
     if (tty_raw(STDIN_FILENO) < 0) {
         perror("tty_raw");
         exit(1);
     }
     if (atexit(tty_atexit) < 0) {
         perror("atexit");
         exit(1);
     }
 
     while ((i = read(STDIN_FILENO, &c, 1)) == 1) {
         c = (char)(c & 0xFF);
         if (c == 0177) break;   /* ASCII DELETE */
         printf("%o\r\n", (unsigned char)c);
     }
 
     if (tty_reset(STDIN_FILENO) < 0) {
         perror("tty_reset");
         exit(1);
     }
 
     /* ── §18.11 cbreak mode test ─────────────────────────── */
     printf("\n=== cbreak mode (signals active, ^C to exit) ===\n");
     if (tty_cbreak(STDIN_FILENO) < 0) {
         perror("tty_cbreak");
         exit(1);
     }
 
     while ((i = read(STDIN_FILENO, &c, 1)) == 1) {
         c = (char)(c & 0xFF);
         printf("%o\r\n", (unsigned char)c);
     }
 
     tty_reset(STDIN_FILENO);
 
     /* ── §18.12 Window size watch ────────────────────────── */
     printf("\n=== Window size (resize window, ^C to exit) ===\n");
     tty_watch_winsize();   /* never returns */
 
     return 0;
 }
 