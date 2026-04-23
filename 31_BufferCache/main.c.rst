#include <stdio.h>
#include <string.h>
#include "bcache.h"
#include <stdlib.h>
#include "bcache_types.h"

static void banner(const char *s)
{
    printf("\n══════════════════════════════════════════════\n");
    printf("  %s\n", s);
    printf("══════════════════════════════════════════════\n");
}

int main(void)
{
    bcache_init();
    bcache_print();

    /* ── Scenario 2: block not in cache → clean free buffer ─── */
    banner("Scenario 2 — bread miss: allocate free buffer");
    BufHdr *b1 = bread(0, 10);
    printf("  Got buffer for dev=0 blk=10  valid=%s\n",
           (b1->status & BUF_VALID) ? "YES" : "NO");

    /* Write some data and release */
    memset(b1->data, 0xAA, 8);
    bwrite(b1, true /*sync*/, false);

    /* ── Scenario 1: same block → cache hit ─────────────────── */
    banner("Scenario 1 — bread hit: block already in cache");
    BufHdr *b2 = bread(0, 10);
    printf("  Got buffer for dev=0 blk=10  data[0]=0x%02x (expect 0xAA)\n",
           b2->data[0]);
    brelse(b2);

    /* ── Delayed write (bdwrite) ─────────────────────────────── */
    banner("Delayed Write (bdwrite)");
    BufHdr *b3 = bread(0, 20);
    memset(b3->data, 0xBB, 8);
    bdwrite(b3);    /* mark delayed; do NOT write to disk yet  */
    printf("  Block 20 marked delayed-write\n");

    /* ── Scenario 3: getblk encounters a delayed-write buffer ── */
    banner("Scenario 3 — getblk flushes delayed-write buffer");
    /*
     * Exhaust the free list so that getblk must pick the
     * delayed-write buffer for dev=0 blk=20.
     * We do this by locking every other free buffer.
     */
    BufHdr *locked[NUM_BUFFERS];
    int     nlocked = 0;

    /* Lock all currently free buffers except the DW one */
    BufHdr *cur = free_head.free_next;
    while (cur != &free_head && nlocked < NUM_BUFFERS - 1) {
        if (!(cur->status & BUF_DELWRITE)) {
            BufHdr *next = cur->free_next;
            //fl_remove(cur);
            cur->status |= BUF_LOCKED;
            locked[nlocked++] = cur;
            cur = next;
        } else {
            cur = cur->free_next;
        }
    }
    printf("  Locked %d buffers to expose delayed-write scenario\n", nlocked);

    /* This request should trigger scenario 3 */
    BufHdr *b4 = getblk(0, 99);
    printf("  getblk for blk=99 returned buffer (dev=%u blk=%u)\n",
           b4->dev, b4->blkno);
    brelse(b4);

    /* Release our locked buffers */
    for (int i = 0; i < nlocked; i++) {
        locked[i]->status &= (uint32_t)~BUF_LOCKED;
        //fl_insert_before(&free_head, locked[i]);
    }

    /* ── breada: read blk 30 + prefetch blk 31 ──────────────── */
    banner("breada — read-ahead (Algorithm 4)");
    BufHdr *b5 = breada(0, 30, 31);
    printf("  breada returned blk=%u  valid=%s\n",
           b5->blkno, (b5->status & BUF_VALID) ? "YES" : "NO");
    brelse(b5);

    /* blk 31 should now be in cache from the async prefetch */
    banner("bread blk=31 — should hit cache from read-ahead");
    BufHdr *b6 = bread(0, 31);
    printf("  blk=31 valid=%s\n",
           (b6->status & BUF_VALID) ? "YES" : "NO");
    brelse(b6);

    /* ── Async write ─────────────────────────────────────────── */
    banner("Asynchronous write (bwrite sync=false)");
    BufHdr *b7 = bread(0, 40);
    memset(b7->data, 0xCC, 8);
    bwrite(b7, false /*async*/, false);
    /* Buffer is now at head of free list (BUF_OLD path) */

    /* ── bflush — flush all delayed writes for dev 0 ─────────── */
    banner("bflush — flush delayed-write buffers");
    bflush(0);

    /* ── Final pool state and statistics ──────────────────────── */
    banner("Final State");
    bcache_print();
    bcache_stats_print();

    return 0;
}
