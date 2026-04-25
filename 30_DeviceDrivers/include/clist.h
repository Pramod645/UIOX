#ifndef UIOX_CLIST_H
#define UIOX_CLIST_H

#include "dev_types.h"

/* =============================================================
 * cblock — one node in a character list
 *
 *  cb_data[]
 *  ┌───┬───┬───┬───┬───┬───┬───┬───┐
 *  │   │ v │ v │ v │ v │   │   │   │
 *  └───┴───┴───┴───┴───┴───┴───┴───┘
 *        ↑               ↑
 *     cb_start         cb_end  (first invalid index)
 *
 * cb_start: index of first valid byte
 * cb_end:   index one past the last valid byte
 * ============================================================= */
typedef struct cblock {
    struct cblock  *cb_next;                /* next cblock on this clist  */
    char            cb_data[CBLOCK_DATA_SIZE];
    int             cb_start;              /* first valid byte index      */
    int             cb_end;               /* one past last valid byte     */
} cblock_t;

/* =============================================================
 * clist — variable-length linked list of cblocks
 * ============================================================= */
typedef struct {
    cblock_t   *cl_head;       /* oldest data (dequeue from here)     */
    cblock_t   *cl_tail;       /* newest data (enqueue here)          */
    int         cl_count;      /* total valid characters on list      */
} clist_t;

/* =============================================================
 * cblock / clist API  (six kernel operations)
 * ============================================================= */

/* Initialise the static cblock free pool (call once at startup) */
void      clist_init(void);

/* op 1 — assign a cblock from the free list to a driver        */
cblock_t *cblock_alloc(void);

/* op 2 — return a cblock to the free list                      */
void      cblock_free(cblock_t *cb);

/* op 3 — retrieve (and remove) the first character from a clist.
 *         Returns the character (0-255) or -1 if the list is empty.
 *         Recycles the cblock when its last character is consumed. */
int       clist_getc(clist_t *cl);

/* op 4 — place a character onto the end of a clist.
 *         Allocates a new cblock if the tail is full.
 *         Returns 0 on success, -1 if the pool is exhausted.   */
int       clist_putc(clist_t *cl, char c);

/* op 5 — remove up to maxlen characters from the front of a
 *         clist (one cblock at a time).
 *         Returns the number of characters actually copied.     */
int       clist_get_blk(clist_t *cl, char *out, int maxlen);

/* op 6 — place a block of characters onto the end of a clist.
 *         Returns the number of characters successfully queued. */
int       clist_put_blk(clist_t *cl, const char *in, int len);

/* Flush / discard all characters on a clist */
void      clist_flush(clist_t *cl);

/* Debug: print pool and clist state */
void      clist_print_pool(void);

#endif /* UIOX_CLIST_H */
