#ifndef UIOX_PTY_H
#define UIOX_PTY_H

#include "dev_types.h"
#include "tty.h"

/* =============================================================
 * pty_pair — one pseudo-terminal pair (STREAMS / mpx)
 *
 * A pty operates in pairs:
 *   master (mpx side)  — the multiplexer reads/writes this
 *   slave  (shell side) — looks exactly like a real tty
 *
 * Output written to the master appears as input on the slave.
 * Output written to the slave  appears as input on the master.
 * ============================================================= */
typedef struct pty_pair {
    int         pp_allocated;   /* non-zero if this pair is in use   */
    int         pp_index;       /* index in the pool                 */
    tty_dev_t   pp_master;      /* mpx side                          */
    tty_dev_t   pp_slave;       /* shell side                        */
} pty_pair_t;

/* =============================================================
 * mpx_state — multiplexer state for STREAMS window management
 * ============================================================= */
typedef struct {
    int          mx_phys_open;              /* physical tty is open      */
    pty_pair_t   mx_pairs[MAX_PTY_PAIRS];   /* pty pool                  */
    int          mx_running;                /* mpx loop active           */
} mpx_state_t;

/* =============================================================
 * PTY / mpx API
 * ============================================================= */

/* Initialise the mpx state and pty pool */
void mpx_init(mpx_state_t *mpx);

/*
 * pty_alloc — allocate a free pty pair from the mpx pool.
 * Returns the pool index on success, -1 if no pairs are free.
 * The driver open ensures the pty was not previously allocated.
 */
int  pty_alloc(mpx_state_t *mpx);

/*
 * pty_free — release a pty pair back to the pool.
 */
void pty_free(mpx_state_t *mpx, int idx);

/*
 * pty_master_write — mpx writes to slave input queue.
 * Data flow: mpx writes → slave td_inq → shell reads.
 */
int  pty_master_write(pty_pair_t *pair, const char *buf, int len);

/*
 * pty_slave_write — shell writes to master input queue.
 * Data flow: shell writes → master td_inq → mpx reads.
 */
int  pty_slave_write(pty_pair_t *pair, const char *buf, int len);

/*
 * mpx_loop — the multiplexer event loop (Figure 10.14)
 *
 *   Assumes fds 0 and 1 already refer to physical tty.
 *
 *   for (;;)
 *   {
 *       select(input);        // wait for any line with input
 *       read input line;
 *       switch (line with input data)
 *       {
 *           case physical tty:
 *               if (control command)   // e.g. "create new window"
 *               {
 *                   open a free pseudo-tty;
 *                   fork new process;
 *                   if (parent) { push msg discipline; continue; }
 *                   // child:
 *                   close unnecessary fds;
 *                   open slave pty → stdin/stdout/stderr;
 *                   push tty line discipline;
 *                   exec shell;
 *               }
 *               demultiplex data: strip header, write to slave pty;
 *               break;
 *
 *           case logical tty (virtual tty writing a window):
 *               encode window-ID header;
 *               write header + data to physical tty;
 *               break;
 *       }
 *   }
 */
int  mpx_loop(mpx_state_t *mpx);

/* Debug: print pty pool state */
void pty_print(const mpx_state_t *mpx);

#endif /* UIOX_PTY_H */
