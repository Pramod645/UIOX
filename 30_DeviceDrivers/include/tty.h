#ifndef UIOX_TTY_H
#define UIOX_TTY_H

#include "dev_types.h"
#include "clist.h"

/* =============================================================
 * tty_dev — per-TTY state for line-discipline algorithms
 * ============================================================= */
typedef struct tty_dev {
    unsigned int    td_major;       /* character major number            */
    unsigned int    td_minor;       /* minor number / unit index         */

    clist_t         td_outq;        /* output queue  (→ hardware)        */
    clist_t         td_inq;         /* input  queue  (← hardware)        */

    int             td_flooded;     /* non-zero when outq > high-water   */
    int             td_canonical;   /* non-zero → canonical (cooked) mode*/
    int             td_echo;        /* non-zero → echo input to output   */
    int             td_open_count;  /* number of open file descriptors   */
    int             td_open;        /* non-zero if tty line is open      */

    /* Hardware start routine: drain td_outq to the device.
     * Driver populates this during its open routine.              */
    void (*td_start)(struct tty_dev *tty);

} tty_dev_t;

/* =============================================================
 * TTY API
 * ============================================================= */

/* Initialise a tty_dev structure */
void tty_init(tty_dev_t *tty, unsigned int major, unsigned int minor);

/* =============================================================
 * Algorithm 6  —  tty_write  (terminal_write)
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
int tty_write(tty_dev_t *tty, const char *buf, int count);

/* =============================================================
 * Algorithm 7  —  tty_read  (terminal_read)
 *
 *   Canonical mode: collect characters up to and including '\n'.
 *   Raw mode:       return whatever is immediately available.
 *   Sleeps if the input clist is empty.
 * ============================================================= */
int tty_read(tty_dev_t *tty, char *buf, int count);

/* =============================================================
 * Algorithm 8  —  do_login  (getty → login → shell)
 *
 *   getty:
 *     set process group (setpgrp)
 *     open tty line  ← sleeps until carrier raised
 *   if open successful:
 *     exec login
 *       prompt username
 *       echo off, prompt password
 *       if password matches /etc/passwd:
 *         ioctl → canonical mode
 *         exec shell
 *       else:
 *         count attempts, retry up to limit
 * ============================================================= */
int do_login(tty_dev_t *tty);

/* Simulate hardware interrupt: push received characters onto td_inq */
void tty_receive_chars(tty_dev_t *tty, const char *buf, int len);

/* Debug: print tty queue state */
void tty_print(const tty_dev_t *tty);

#endif /* UIOX_TTY_H */
