/*
 * src/tty.c
 *
 * Algorithm 6  tty_write  — terminal write with line discipline
 * Algorithm 7  tty_read   — terminal read (canonical / raw)
 * Algorithm 8  do_login   — getty → login → shell sequence
 */

 #include "tty.h"
 #include <stdio.h>
 #include <string.h>
 #include <errno.h>
 
 /* =============================================================
  * tty_init
  * ============================================================= */
 void tty_init(tty_dev_t *tty, unsigned int major, unsigned int minor)
 {
     memset(tty, 0, sizeof *tty);
     tty->td_major     = major;
     tty->td_minor     = minor;
     tty->td_canonical = 1;   /* cooked mode by default             */
     tty->td_echo      = 1;
     printf("[tty] init major=%u minor=%u\n", major, minor);
 }
 
 /* =============================================================
  * Algorithm 6 — tty_write  (terminal_write)
  *
  *   while (more data to copy from user space)
  *   {
  *       if (tty flooded with output data)
  *       {
  *           start write to hardware with data on output clist;
  *           sleep (event: tty can accept more data);
  *           continue;
  *       }
  *       copy cblock-sized chunk from user space to output clist;
  *           line discipline: convert tabs, CR/LF, etc.
  *   }
  *   start write to hardware with data on output clist;
  * ============================================================= */
 int tty_write(tty_dev_t *tty, const char *buf, int count)
 {
     int remaining = count;
     int written   = 0;
 
     while (remaining > 0) {
 
         /* ── flood check ──────────────────────────────────── */
         while (tty->td_outq.cl_count >= TTY_FLOOD_HIGH) {
             tty->td_flooded = 1;
 
             /*
              * start write operation to hardware with data on output clist
              * — in a real driver this kicks the hardware start routine
              */
             if (tty->td_start)
                 tty->td_start(tty);
 
             printf("[tty_write] minor=%u flooded (%d chars) — draining\n",
                    tty->td_minor, tty->td_outq.cl_count);
 
             /*
              * sleep (event: tty can accept more data)
              * — simulated: drain clist directly
              */
             clist_flush(&tty->td_outq);
             tty->td_flooded = 0;
         }
 
         /* ── copy one cblock-sized chunk from user space ─── */
         {
             int   chunk = remaining < CBLOCK_DATA_SIZE
                           ? remaining : CBLOCK_DATA_SIZE;
             int   i;
             char  c;
 
             /* ── line discipline processing ──────────────── */
             for (i = 0; i < chunk; i++) {
                 c = buf[written + i];
 
                 /*
                  * Tab expansion: replace '\t' with spaces to
                  * the next 8-column boundary.
                  */
                 if (c == '\t') {
                     int col    = tty->td_outq.cl_count % 8;
                     int spaces = 8 - col;
                     while (spaces--) {
                         if (clist_putc(&tty->td_outq, ' ') < 0)
                             goto done;
                     }
                     continue;
                 }
 
                 /*
                  * Newline translation (canonical mode):
                  * convert '\n' to CR + LF.
                  */
                 if (c == '\n' && tty->td_canonical) {
                     if (clist_putc(&tty->td_outq, '\r') < 0)
                         goto done;
                 }
 
                 if (clist_putc(&tty->td_outq, c) < 0)
                     goto done;
             }
 
             written   += chunk;
             remaining -= chunk;
         }
     }
 
 done:
     /* start write operation to hardware with data on output clist */
     if (tty->td_start)
         tty->td_start(tty);
 
     printf("[tty_write] minor=%u wrote=%d outq=%d\n",
            tty->td_minor, written, tty->td_outq.cl_count);
     return written;
 }
 
 /* =============================================================
  * Algorithm 7 — tty_read  (terminal_read)
  *
  *   Canonical mode: collect characters up to and including '\n'.
  *   Raw mode:       return whatever is immediately available.
  *   Sleeps (simulated) if the input clist is empty.
  * ============================================================= */
 int tty_read(tty_dev_t *tty, char *buf, int count)
 {
     int n = 0;
     int c;
 
     /* sleep until at least one character arrives
      * (simulated: return 0 immediately if empty)               */
     if (tty->td_inq.cl_count == 0) {
         printf("[tty_read] minor=%u inq empty — would sleep\n",
                tty->td_minor);
         return 0;
     }
 
     if (tty->td_canonical) {
         /*
          * Canonical mode: return one line at a time.
          * Read up to and including '\n', or up to count bytes.
          */
         while (n < count) {
             c = clist_getc(&tty->td_inq);
             if (c < 0) break;
             buf[n++] = (char)c;
             if ((char)c == '\n') break;
         }
     } else {
         /*
          * Raw mode: return all available characters up to count.
          */
         int want = count < CBLOCK_DATA_SIZE ? count : CBLOCK_DATA_SIZE;
         n = clist_get_blk(&tty->td_inq, buf, want);
     }
 
     printf("[tty_read] minor=%u read=%d canonical=%d\n",
            tty->td_minor, n, tty->td_canonical);
     return n;
 }
 
 /* =============================================================
  * Algorithm 8 — do_login  (getty → login → shell)
  *
  *   getty:
  *     set process group (setpgrp);
  *     open tty line;   ← sleeps until carrier is raised
  *   if (open successful)
  *   {
  *       exec login program;
  *       prompt for user name;
  *       turn off echo, prompt for password;
  *       if (successful)     // matches /etc/passwd
  *       {
  *           put tty in canonical mode (ioctl);
  *           exec shell;
  *       }
  *       else
  *           count login attempts, try again up to a point;
  *   }
  * ============================================================= */
 int do_login(tty_dev_t *tty)
 {
     int attempts = 0;
 
     printf("[login] getty starting on tty %u:%u\n",
            tty->td_major, tty->td_minor);
 
     /*
      * set process group — real: setpgrp();
      * open tty line — sleeps until modem raises carrier.
      * Both handled by the driver open above this routine.
      */
 
     if (!tty->td_open) {
         printf("[login] tty line not open — cannot log in\n");
         return -ENODEV;
     }
 
     printf("[login] tty line open\n");
 
     for (attempts = 0; attempts < LOGIN_MAX_ATTEMPTS; attempts++) {
 
         int authenticated;
 
         /* exec login program: prompt for user name */
         printf("[login] Username: ");
 
         /* turn off echo; prompt for password */
         tty->td_echo = 0;
         printf("[login] Password: (echo off)\n");
 
         /*
          * Authenticate against /etc/passwd.
          * In production this is a PAM call; here it is a stub.
          */
         authenticated = 1;      /* stub: always succeeds             */
 
         /* restore echo regardless of outcome */
         tty->td_echo = 1;
 
         if (authenticated) {
             /*
              * put tty in canonical mode:
              *   ioctl(fd, TIOCSETCANON, NULL);
              */
             tty->td_canonical = 1;
             tty->td_echo      = 1;
 
             printf("[login] success — canonical mode set, exec shell\n");
 
             /*
              * exec shell:
              *   real: execve("/bin/sh", argv, envp);
              */
             return 0;
         }
 
         printf("[login] attempt %d/%d failed\n",
                attempts + 1, LOGIN_MAX_ATTEMPTS);
     }
 
     printf("[login] too many failures on tty %u:%u\n",
            tty->td_major, tty->td_minor);
     return -EACCES;
 }
 
 /* =============================================================
  * tty_receive_chars
  * Simulate a hardware input interrupt: push received characters
  * onto the input clist and wake any sleeping reader.
  * ============================================================= */
 void tty_receive_chars(tty_dev_t *tty, const char *buf, int len)
 {
     int n = clist_put_blk(&tty->td_inq, buf, len);
     printf("[tty] minor=%u received %d chars (inq=%d)\n",
            tty->td_minor, n, tty->td_inq.cl_count);
 
     /* echo back to output if enabled */
     if (tty->td_echo)
         clist_put_blk(&tty->td_outq, buf, n);
 }
 
 /* =============================================================
  * tty_print — debug dump
  * ============================================================= */
 void tty_print(const tty_dev_t *tty)
 {
     printf("[tty] %u:%u  inq=%d  outq=%d  canon=%d  echo=%d  flood=%d\n",
            tty->td_major, tty->td_minor,
            tty->td_inq.cl_count, tty->td_outq.cl_count,
            tty->td_canonical, tty->td_echo, tty->td_flooded);
 }
 