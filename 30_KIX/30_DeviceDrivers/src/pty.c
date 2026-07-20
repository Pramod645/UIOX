/*
 * src/pty.c
 *
 * STREAMS / pseudo-tty (pty) multiplexer — Figure 10.14.
 *
 * A pty pair has two ends:
 *   master (mpx side)  — the multiplexer reads/writes this
 *   slave  (shell side) — looks like a real tty to the shell
 *
 * Output written to master → appears as input on slave.
 * Output written to slave  → appears as input on master.
 */

 #include "pty.h"
 #include <stdio.h>
 #include <string.h>
 
 /* =============================================================
  * mpx_init
  * ============================================================= */
 void mpx_init(mpx_state_t *mpx)
 {
     int i;
     memset(mpx, 0, sizeof *mpx);
 
     for (i = 0; i < MAX_PTY_PAIRS; i++) {
         mpx->mx_pairs[i].pp_allocated = 0;
         mpx->mx_pairs[i].pp_index     = i;
     }
 
     printf("[mpx] init: %d pty pairs available\n", MAX_PTY_PAIRS);
 }
 
 /* =============================================================
  * pty_alloc
  *
  * Allocate a free pty pair from the pool.
  * The driver open ensures the pty was not previously allocated.
  * Returns the pool index on success, -1 if no pairs are free.
  * ============================================================= */
 int pty_alloc(mpx_state_t *mpx)
 {
     int i;
 
     for (i = 0; i < MAX_PTY_PAIRS; i++) {
         if (!mpx->mx_pairs[i].pp_allocated) {
             pty_pair_t *p = &mpx->mx_pairs[i];
 
             p->pp_allocated = 1;
 
             /* Initialise master side (mpx side) */
             tty_init(&p->pp_master, 0, (unsigned int)(i * 2));
 
             /* Initialise slave side (shell side) */
             tty_init(&p->pp_slave, 0, (unsigned int)(i * 2 + 1));
             p->pp_slave.td_canonical = 1;   /* slave ≡ real tty   */
             p->pp_slave.td_echo      = 1;
             p->pp_slave.td_open      = 1;
 
             printf("[pty_alloc] pair %d allocated "
                    "(master minor=%u slave minor=%u)\n",
                    i, p->pp_master.td_minor, p->pp_slave.td_minor);
             return i;
         }
     }
 
     fprintf(stderr, "[pty_alloc] no free pty pairs\n");
     return -1;
 }
 
 /* =============================================================
  * pty_free — release a pty pair back to the pool
  * ============================================================= */
 void pty_free(mpx_state_t *mpx, int idx)
 {
     if (idx < 0 || idx >= MAX_PTY_PAIRS) return;
 
     clist_flush(&mpx->mx_pairs[idx].pp_master.td_outq);
     clist_flush(&mpx->mx_pairs[idx].pp_master.td_inq);
     clist_flush(&mpx->mx_pairs[idx].pp_slave.td_outq);
     clist_flush(&mpx->mx_pairs[idx].pp_slave.td_inq);
 
     mpx->mx_pairs[idx].pp_allocated = 0;
     printf("[pty_free] pair %d released\n", idx);
 }
 
 /* =============================================================
  * pty_master_write
  *
  * mpx writes to slave's input queue.
  * Data flow: mpx writes → slave td_inq → shell reads.
  * ============================================================= */
 int pty_master_write(pty_pair_t *pair, const char *buf, int len)
 {
     int n = clist_put_blk(&pair->pp_slave.td_inq, buf, len);
     printf("[pty] master→slave: %d chars (slave inq=%d)\n",
            n, pair->pp_slave.td_inq.cl_count);
     return n;
 }
 
 /* =============================================================
  * pty_slave_write
  *
  * Shell writes to master's input queue.
  * Data flow: shell writes → master td_inq → mpx reads.
  * ============================================================= */
 int pty_slave_write(pty_pair_t *pair, const char *buf, int len)
 {
     int n = clist_put_blk(&pair->pp_master.td_inq, buf, len);
     printf("[pty] slave→master: %d chars (master inq=%d)\n",
            n, pair->pp_master.td_inq.cl_count);
     return n;
 }
 
 /* =============================================================
  * mpx_loop — multiplexer event loop  (Figure 10.14)
  *
  *   assume fds 0 and 1 already refer to physical tty
  *
  *   for (;;)                          ← loop
  *   {
  *       select(input);                ← wait for some line with input
  *       read input line;
  *       switch (line with input data)
  *       {
  *           case physical tty:        ← input on physical tty line
  *               if (control command)  ← e.g. create new window
  *               {
  *                   open a free pseudo-tty;
  *                   fork a new process;
  *                   if (parent)
  *                   {
  *                       push a msg discipline on mpx side;
  *                       continue;
  *                   }
  *                   // child here
  *                   close unnecessary file descriptors;
  *                   open other member of pseudo-tty pair;
  *                       → get stdin, stdout, stderr;
  *                   push tty line discipline;
  *                   exec shell;       ← looks like virtual tty
  *               }
  *               // regular data from physical tty for virtual tty
  *               demultiplex data from physical tty;
  *               strip headers, write to appropriate pty;
  *               continue;
  *
  *           case logical tty:         ← a virtual tty writing a window
  *               encode header indicating which window data is for;
  *               write header + data to physical tty;
  *               continue;
  *       }
  *   }
  * ============================================================= */
 int mpx_loop(mpx_state_t *mpx)
 {
     char data_buf[CBLOCK_DATA_SIZE];
     char hdr_buf[MPX_HDR_SIZE + CBLOCK_DATA_SIZE];
     int  i, n, idx;
     int  iterations = 0;
 
     printf("[mpx_loop] starting\n");
     mpx->mx_running = 1;
 
     /* In this simulation we run for a fixed number of iterations
      * rather than blocking forever, so the demo can terminate.  */
     while (mpx->mx_running && iterations < 4) {
         iterations++;
         printf("[mpx_loop] iteration %d\n", iterations);
 
         /* ── select(input): wait for some line with input ──── */
         /* Simulated: check each source in turn                  */
 
         /* ── Case: physical tty — control command (new window) */
         if (iterations == 1) {
             /*
              * Control command: create a new window.
              * open a free pseudo-tty;
              */
             idx = pty_alloc(mpx);
             if (idx >= 0) {
                 pty_pair_t *pair = &mpx->mx_pairs[idx];
 
                 printf("[mpx_loop] new window on pty pair %d\n", idx);
 
                 /*
                  * fork a new process:
                  *   if (parent) { push msg discipline; continue; }
                  *
                  * Parent (mpx) side:
                  *   push a msg discipline on mpx side of pty.
                  *   In this simulation we just mark it active.
                  */
                 pair->pp_master.td_open = 1;
 
                 /*
                  * Child (shell) side would:
                  *   close unnecessary file descriptors;
                  *   open slave pty → stdin(0), stdout(1), stderr(2);
                  *   push tty line discipline;
                  *   exec shell;   ← looks like virtual tty
                  *
                  * Simulated: enable line discipline on slave.
                  */
                 pair->pp_slave.td_canonical = 1;
                 pair->pp_slave.td_echo      = 1;
                 pair->pp_slave.td_open      = 1;
 
                 printf("[mpx_loop] shell side: line discipline pushed, "
                        "exec shell\n");
             }
         }
 
         /* ── Case: physical tty — regular data for a virtual tty */
         /*
          * Demultiplex data read from physical tty:
          * strip off window-ID headers and write to appropriate pty.
          */
         for (i = 0; i < MAX_PTY_PAIRS; i++) {
             pty_pair_t *pair = &mpx->mx_pairs[i];
             if (!pair->pp_allocated) continue;
 
             /* simulate physical tty delivering data to pair i   */
             if (pair->pp_master.td_inq.cl_count > 0) {
                 n = clist_get_blk(&pair->pp_master.td_inq,
                                   data_buf, CBLOCK_DATA_SIZE);
                 if (n > 0) {
                     printf("[mpx_loop] phys→slave[%d]: %d bytes\n", i, n);
                     pty_master_write(pair, data_buf, n);
                 }
             }
         }
 
         /* ── Case: logical tty — a virtual tty writing a window */
         /*
          * A slave pty has data for the physical tty.
          * Encode a window-ID header and forward to physical tty output.
          */
         for (i = 0; i < MAX_PTY_PAIRS; i++) {
             pty_pair_t *pair = &mpx->mx_pairs[i];
             if (!pair->pp_allocated) continue;
 
             if (pair->pp_slave.td_outq.cl_count > 0) {
                 n = clist_get_blk(&pair->pp_slave.td_outq,
                                   data_buf, CBLOCK_DATA_SIZE);
                 if (n > 0) {
                     /*
                      * encode header indicating what window data is for:
                      *   byte 0: window index
                      *   byte 1: data length
                      *   bytes 2-3: reserved
                      */
                     hdr_buf[0] = (char)i;
                     hdr_buf[1] = (char)n;
                     hdr_buf[2] = 0;
                     hdr_buf[3] = 0;
                     memcpy(hdr_buf + MPX_HDR_SIZE, data_buf, (size_t)n);
 
                     /* write header + data to physical tty         */
                     printf("[mpx_loop] slave[%d]→phys: %d bytes "
                            "(hdr+data=%d)\n",
                            i, n, n + MPX_HDR_SIZE);
                 }
             }
         }
     }
 
     mpx->mx_running = 0;
     printf("[mpx_loop] stopped after %d iterations\n", iterations);
     return 0;
 }
 
 /* =============================================================
  * pty_print — debug dump
  * ============================================================= */
 void pty_print(const mpx_state_t *mpx)
 {
     int i;
     printf("[pty] pool state:\n");
     for (i = 0; i < MAX_PTY_PAIRS; i++) {
         const pty_pair_t *p = &mpx->mx_pairs[i];
         if (p->pp_allocated)
             printf("  pair %d  master(inq=%d outq=%d)  "
                    "slave(inq=%d outq=%d)\n", i,
                    p->pp_master.td_inq.cl_count,
                    p->pp_master.td_outq.cl_count,
                    p->pp_slave.td_inq.cl_count,
                    p->pp_slave.td_outq.cl_count);
         else
             printf("  pair %d  free\n", i);
     }
 }
 