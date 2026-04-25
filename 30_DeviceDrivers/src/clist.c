/*
 * src/clist.c
 *
 * cblock / clist implementation — six kernel operations on
 * character lists, as described in Bach §10 (Line Disciplines).
 */

 #include "clist.h"
 #include <stdio.h>
 #include <string.h>
 
 /* =============================================================
  * Static cblock free pool
  * ============================================================= */
 static cblock_t  cpool[CBLOCK_POOL_SIZE];
 static cblock_t *cpool_free;            /* head of free list            */
 static int       cpool_used;            /* blocks currently allocated   */
 
 /* =============================================================
  * clist_init
  * Build the free list from the static pool.
  * Call once at subsystem startup.
  * ============================================================= */
 void clist_init(void)
 {
     int i;
     memset(cpool, 0, sizeof cpool);
     cpool_free = NULL;
     cpool_used = 0;
 
     for (i = 0; i < CBLOCK_POOL_SIZE; i++) {
         cpool[i].cb_next = cpool_free;
         cpool_free       = &cpool[i];
     }
 
     printf("[clist] pool init: %d cblocks × %d bytes each\n",
            CBLOCK_POOL_SIZE, CBLOCK_DATA_SIZE);
 }
 
 /* =============================================================
  * Operation 1 — assign a cblock from the free list to a driver
  * ============================================================= */
 cblock_t *cblock_alloc(void)
 {
     cblock_t *cb = cpool_free;
 
     if (!cb) {
         fprintf(stderr, "[cblock_alloc] pool exhausted\n");
         return NULL;
     }
 
     cpool_free   = cb->cb_next;
     cb->cb_next  = NULL;
     cb->cb_start = 0;
     cb->cb_end   = 0;
     cpool_used++;
 
     return cb;
 }
 
 /* =============================================================
  * Operation 2 — return a cblock to the free list
  * ============================================================= */
 void cblock_free(cblock_t *cb)
 {
     if (!cb) return;
     cb->cb_next  = cpool_free;
     cpool_free   = cb;
     cpool_used--;
 }
 
 /* =============================================================
  * Operation 3 — retrieve the first character from a clist
  *
  * Removes the leading character and advances cb_start.
  * When the cblock is exhausted it is recycled to the free list
  * and the clist head pointer is advanced.
  * If the clist is empty the null character (-1) is returned.
  * ============================================================= */
 int clist_getc(clist_t *cl)
 {
     cblock_t *cb;
     int       c;
 
     if (!cl->cl_head || cl->cl_count == 0)
         return -1;          /* clist empty — return null equivalent  */
 
     cb = cl->cl_head;
     c  = (unsigned char)cb->cb_data[cb->cb_start++];
     cl->cl_count--;
 
     if (cb->cb_start >= cb->cb_end) {
         /* Last character consumed — recycle the empty cblock */
         cl->cl_head = cb->cb_next;
         if (!cl->cl_head)
             cl->cl_tail = NULL;
         cblock_free(cb);
     }
 
     return c;
 }
 
 /* =============================================================
  * Operation 4 — place a character onto the end of a clist
  *
  * Appends to the tail cblock; if the cblock is full a new one
  * is allocated, linked onto the end, and the character is placed
  * into the new cblock.
  * Returns 0 on success, -1 if the free pool is exhausted.
  * ============================================================= */
 int clist_putc(clist_t *cl, char c)
 {
     cblock_t *cb = cl->cl_tail;
 
     if (!cb || cb->cb_end >= CBLOCK_DATA_SIZE) {
         /* Tail is full or list is empty — allocate a new cblock  */
         cb = cblock_alloc();
         if (!cb)
             return -1;
 
         if (cl->cl_tail)
             cl->cl_tail->cb_next = cb;  /* link new block onto tail  */
         else
             cl->cl_head = cb;           /* first block on empty list */
 
         cl->cl_tail = cb;
     }
 
     cb->cb_data[cb->cb_end++] = c;
     cl->cl_count++;
     return 0;
 }
 
 /* =============================================================
  * Operation 5 — remove a group of characters from the front
  *               of a clist, one cblock at a time
  *
  * Equivalent to calling clist_getc() up to maxlen times.
  * Returns the number of characters actually copied.
  * ============================================================= */
 int clist_get_blk(clist_t *cl, char *out, int maxlen)
 {
     int n = 0, c;
 
     while (n < maxlen) {
         c = clist_getc(cl);
         if (c < 0) break;
         out[n++] = (char)c;
     }
     return n;
 }
 
 /* =============================================================
  * Operation 6 — place a cblock of characters onto the end
  *               of a clist
  *
  * Returns the number of characters successfully queued.
  * ============================================================= */
 int clist_put_blk(clist_t *cl, const char *in, int len)
 {
     int i;
     for (i = 0; i < len; i++)
         if (clist_putc(cl, in[i]) < 0)
             break;
     return i;
 }
 
 /* =============================================================
  * clist_flush — discard all characters on a clist
  * ============================================================= */
 void clist_flush(clist_t *cl)
 {
     while (cl->cl_head) {
         cblock_t *cb = cl->cl_head;
         cl->cl_head  = cb->cb_next;
         cblock_free(cb);
     }
     cl->cl_tail  = NULL;
     cl->cl_count = 0;
 }
 
 /* =============================================================
  * clist_print_pool — debug dump
  * ============================================================= */
 void clist_print_pool(void)
 {
     printf("[clist] pool: %d used / %d total\n",
            cpool_used, CBLOCK_POOL_SIZE);
 }
 