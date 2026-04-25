/*
 * main.c  —  uiox_dev demonstration
 *
 * Exercises all device driver algorithms in the same style as
 * the uiox_fs main.c:
 *
 *   clist ops 1-6       cblock_alloc/free, clist_getc/putc,
 *                        clist_get_blk/put_blk
 *   Algorithm 1          dev_open
 *   Algorithm 2          dev_close
 *   Algorithm 3          dev_read
 *   Algorithm 4          dev_write
 *   Algorithm 5          dev_ioctl
 *   strategy             dev_strategy
 *   interrupt            dev_interrupt_handler
 *   Algorithm 6          tty_write
 *   Algorithm 7          tty_read
 *   Algorithm 8          do_login
 *   STREAMS / pty        mpx_loop
 */

 #include <stdio.h>
 #include <string.h>
 #include "dev_types.h"
 #include "clist.h"
 #include "devsw.h"
 #include "tty.h"
 #include "pty.h"
 
 static void banner(const char *s)
 {
     printf("\n══════════════════════════════════════════\n");
     printf("  %s\n", s);
     printf("══════════════════════════════════════════\n");
 }
 
 int main(void)
 {
     /* ── Initialise subsystems ──────────────────────────────── */
     banner("Device Driver Subsystem Init");
     clist_init();
     devsw_init();
     devsw_print();
 
     /* ── clist operations 1-6 ───────────────────────────────── */
     banner("clist Operations 1-6");
 
     clist_t cl;
     memset(&cl, 0, sizeof cl);
 
     /* op 4: put individual characters */
     clist_putc(&cl, 'H');
     clist_putc(&cl, 'e');
     clist_putc(&cl, 'l');
     clist_putc(&cl, 'l');
     clist_putc(&cl, 'o');
     printf("[main] after putc x5: cl_count=%d\n", cl.cl_count);
 
     /* op 3: get individual characters */
     int c = clist_getc(&cl);
     printf("[main] getc → '%c'  cl_count=%d\n", (char)c, cl.cl_count);
 
     /* op 6: put a block */
     clist_put_blk(&cl, " world", 6);
     printf("[main] after put_blk: cl_count=%d\n", cl.cl_count);
 
     /* op 5: get a block */
     char rbuf[32] = {0};
     int  n = clist_get_blk(&cl, rbuf, 20);
     rbuf[n] = '\0';
     printf("[main] get_blk → '%s'  n=%d\n", rbuf, n);
 
     /* op 1 & 2: alloc / free cblocks directly */
     cblock_t *cb = cblock_alloc();
     printf("[main] cblock_alloc → %s\n", cb ? "ok" : "NULL");
     cblock_free(cb);
     clist_print_pool();
 
     /* ── Algorithm 1 — dev_open ─────────────────────────────── */
     banner("Algorithm 1 — dev_open");
 
     uint32_t disk_dev = DEV_MAKE(0, 0);   /* gd disk, major=0 minor=0  */
     uint32_t con_dev  = DEV_MAKE(0, 0);   /* console, major=0 minor=0  */
     uint32_t tape_dev = DEV_MAKE(1, 0);   /* gt tape, major=1 minor=0  */
 
     dev_open(disk_dev, DEV_O_RDWR,   DEV_TYPE_BLOCK);
     dev_open(con_dev,  DEV_O_RDWR,   DEV_TYPE_CHAR);
     dev_open(tape_dev, DEV_O_RDONLY, DEV_TYPE_BLOCK);
 
     /* ── Algorithm 2 — dev_close ────────────────────────────── */
     banner("Algorithm 2 — dev_close");
 
     dev_close(disk_dev, DEV_TYPE_BLOCK, 0 /* not mounted */);
     dev_close(con_dev,  DEV_TYPE_CHAR,  0);
     dev_close(tape_dev, DEV_TYPE_BLOCK, 1 /* mounted — skip */);
 
     /* ── Algorithms 3 & 4 — dev_read / dev_write ────────────── */
     banner("Algorithms 3 & 4 — dev_read / dev_write");
 
     char      rdbuf[64] = {0};
     uint32_t  pos = 0;
 
     /* character device read/write */
     dev_write(con_dev, DEV_TYPE_CHAR, "Hello console\n", 14, &pos);
     dev_read (con_dev, DEV_TYPE_CHAR, rdbuf, 14, &pos);
 
     /* block device read/write (uses strategy) */
     dev_write(disk_dev, DEV_TYPE_BLOCK, "block data", 10, &pos);
     dev_read (disk_dev, DEV_TYPE_BLOCK, rdbuf, 10, &pos);
 
     /* ── Algorithm 5 — dev_ioctl ────────────────────────────── */
     banner("Algorithm 5 — dev_ioctl");
 
     dev_ioctl(con_dev, TIOCSETCANON, 0);
     dev_ioctl(con_dev, TIOCCLRECHO,  0);
     dev_ioctl(con_dev, TIOCSETECHO,  0);
 
     /* ── Strategy interface ──────────────────────────────────── */
     banner("Strategy Interface — dev_strategy");
 
     BufHdr bp;
     memset(&bp, 0, sizeof bp);
     bp.b_dev    = disk_dev;
     bp.b_flags  = B_WRITE;
     bp.b_blkno  = 5;
     bp.b_bcount = 512;
     dev_strategy(&bp);
     printf("[main] strategy B_DONE=%d\n", !!(bp.b_flags & B_DONE));
 
     /* ── Interrupt handler ───────────────────────────────────── */
     banner("Interrupt Handler");
     dev_interrupt_handler(14, (void *)(size_t)DEV_MAKE(0, 0));
 
     /* ── Algorithm 6 — tty_write ────────────────────────────── */
     banner("Algorithm 6 — tty_write (terminal_write)");
 
     tty_dev_t con_tty;
     tty_init(&con_tty, 0, 0);
     con_tty.td_open = 1;
 
     tty_write(&con_tty, "Hello\tworld\n", 12);
     tty_print(&con_tty);
 
     /* ── Algorithm 7 — tty_read ─────────────────────────────── */
     banner("Algorithm 7 — tty_read (terminal_read)");
 
     /* Simulate hardware pushing characters into input queue */
     tty_receive_chars(&con_tty, "echo test\n", 10);
 
     char linebuf[64] = {0};
     n = tty_read(&con_tty, linebuf, 63);
     linebuf[n] = '\0';
     printf("[main] tty_read → '%s'  n=%d\n", linebuf, n);
 
     tty_print(&con_tty);
 
     /* ── Algorithm 8 — do_login ─────────────────────────────── */
     banner("Algorithm 8 — do_login (getty → login → shell)");
 
     tty_dev_t login_tty;
     tty_init(&login_tty, 0, 1);
     login_tty.td_open = 1;
     do_login(&login_tty);
 
     /* ── STREAMS / pseudo-tty / mpx ─────────────────────────── */
     banner("STREAMS — mpx_loop (Love you Engineering)");
 
     mpx_state_t mpx;
     mpx_init(&mpx);
     mpx.mx_phys_open = 1;
 
     /* Push some data through a slave pty before mpx_loop runs */
     {
         int idx = pty_alloc(&mpx);
         if (idx >= 0) {
             pty_pair_t *pair = &mpx.mx_pairs[idx];
 
             /* shell writes output that mpx should forward to physical tty */
             pty_slave_write(pair, "shell output\n", 13);
 
             /* mpx writes data down to the shell (demuxed from phys tty)  */
             pty_master_write(pair, "prompt: ", 8);
 
             pty_print(&mpx);
             pty_free(&mpx, idx);
         }
     }
 
     mpx_loop(&mpx);
     pty_print(&mpx);
 
     /* ── Final pool state ────────────────────────────────────── */
     banner("Final State");
     clist_print_pool();
     devsw_print();
 
     return 0;
 }
 