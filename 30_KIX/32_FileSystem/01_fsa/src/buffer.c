#include "buffer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ─────────────────────────────────────────────────────────────
 * Simulated disk storage
 * ───────────────────────────────────────────────────────────── */
static uint8_t  disk[MAX_BLOCKS][BLOCK_SIZE];
static BufEntry cache[MAX_BUFS];

/* LRU free list */
static BufEntry *free_head = NULL;
static BufEntry *free_tail = NULL;

/* Hash table: blkno % MAX_BUFS */
static BufEntry *hash_table[MAX_BUFS];

/* ─────────────────────────────────────────────────────────────
 * Internal helpers
 * ───────────────────────────────────────────────────────────── */
static void free_list_remove(BufEntry *b)
{
    if (b->prev_free) b->prev_free->next_free = b->next_free;
    else              free_head               = b->next_free;
    if (b->next_free) b->next_free->prev_free = b->prev_free;
    else              free_tail               = b->prev_free;
    b->prev_free = b->next_free = NULL;
}

static void free_list_append(BufEntry *b)
{
    b->prev_free = free_tail;
    b->next_free = NULL;
    if (free_tail) free_tail->next_free = b;
    else           free_head            = b;
    free_tail = b;
}

static void hash_insert(BufEntry *b)
{
    int slot = (int)(b->blkno % MAX_BUFS);
    b->next_hash      = hash_table[slot];
    hash_table[slot]  = b;
}

static void hash_remove(BufEntry *b)
{
    int       slot = (int)(b->blkno % MAX_BUFS);
    BufEntry *prev = NULL, *cur = hash_table[slot];
    while (cur) {
        if (cur == b) {
            if (prev) prev->next_hash  = cur->next_hash;
            else      hash_table[slot] = cur->next_hash;
            cur->next_hash = NULL;
            return;
        }
        prev = cur; cur = cur->next_hash;
    }
}

static BufEntry *hash_lookup(uint32_t blkno)
{
    int       slot = (int)(blkno % MAX_BUFS);
    BufEntry *b    = hash_table[slot];
    while (b) {
        if (b->blkno == blkno && b->valid) return b;
        b = b->next_hash;
    }
    return NULL;
}

/* ─────────────────────────────────────────────────────────────
 * buf_init
 * ───────────────────────────────────────────────────────────── */
void buf_init(void)
{
    memset(disk,        0, sizeof disk);
    memset(cache,       0, sizeof cache);
    memset(hash_table,  0, sizeof hash_table);
    free_head = free_tail = NULL;

    for (int i = 0; i < MAX_BUFS; i++) {
        cache[i].blkno = (uint32_t)-1;
        free_list_append(&cache[i]);
    }
    printf("[buf] init: %d cache slots  disk=%d blocks × %d bytes\n",
           MAX_BUFS, MAX_BLOCKS, BLOCK_SIZE);
}

/* ─────────────────────────────────────────────────────────────
 * getblk
 * ───────────────────────────────────────────────────────────── */
BufEntry *getblk(uint32_t blkno)
{
    /* Check cache first */
    BufEntry *b = hash_lookup(blkno);
    if (b) {
        if (b->locked) {
            /* In a real kernel: sleep until unlocked */
            printf("  [buf] getblk: blk %u locked — waiting\n", blkno);
        }
        if (b->refcount == 0) free_list_remove(b);
        b->refcount++;
        b->locked = true;
        return b;
    }

    /* Not in cache — grab LRU free entry */
    b = free_head;
    if (!b) {
        fprintf(stderr, "[buf] getblk: no free buffers!\n");
        return NULL;
    }
    free_list_remove(b);

    /* Flush dirty entry before reuse */
    if (b->dirty) {
        if (b->blkno < MAX_BLOCKS)
            memcpy(disk[b->blkno], b->data, BLOCK_SIZE);
        b->dirty = false;
    }

    /* Move to new hash slot */
    if (b->valid) hash_remove(b);

    b->blkno    = blkno;
    b->valid    = false;
    b->dirty    = false;
    b->locked   = true;
    b->refcount = 1;
    hash_insert(b);

    return b;
}

/* ─────────────────────────────────────────────────────────────
 * bread
 * ───────────────────────────────────────────────────────────── */
BufEntry *bread(uint32_t blkno)
{
    BufEntry *b = getblk(blkno);
    if (!b) return NULL;

    if (!b->valid) {
        if (blkno < MAX_BLOCKS)
            memcpy(b->data, disk[blkno], BLOCK_SIZE);
        else
            memset(b->data, 0, BLOCK_SIZE);
        b->valid = true;
    }
    return b;
}

/* ─────────────────────────────────────────────────────────────
 * bwrite
 * ───────────────────────────────────────────────────────────── */
void bwrite(BufEntry *buf)
{
    if (!buf) return;
    if (buf->blkno < MAX_BLOCKS)
        memcpy(disk[buf->blkno], buf->data, BLOCK_SIZE);
    buf->dirty = false;
    printf("  [buf] bwrite: blk %u flushed\n", buf->blkno);
}

/* ─────────────────────────────────────────────────────────────
 * brelse
 * ───────────────────────────────────────────────────────────── */
void brelse(BufEntry *buf)
{
    if (!buf) return;
    buf->locked = false;
    buf->refcount--;
    if (buf->refcount == 0)
        free_list_append(buf);
}

/* ─────────────────────────────────────────────────────────────
 * buf_sync
 * ───────────────────────────────────────────────────────────── */
void buf_sync(void)
{
    int n = 0;
    for (int i = 0; i < MAX_BUFS; i++) {
        if (cache[i].dirty && cache[i].blkno < MAX_BLOCKS) {
            memcpy(disk[cache[i].blkno], cache[i].data, BLOCK_SIZE);
            cache[i].dirty = false;
            n++;
        }
    }
    printf("[buf] sync: flushed %d dirty buffers\n", n);
}

void buf_print(void)
{
    printf("[buf] cache state:\n");
    for (int i = 0; i < MAX_BUFS; i++) {
        if (cache[i].valid)
            printf("  slot %2d  blk=%-4u  ref=%d  dirty=%d  locked=%d\n",
                   i, cache[i].blkno, cache[i].refcount,
                   cache[i].dirty, cache[i].locked);
    }
}
