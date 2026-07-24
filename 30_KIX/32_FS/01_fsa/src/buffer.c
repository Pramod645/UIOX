/*
 *  30_KIX/32_FS/01_fsa/src/buffer.c
 *
 *  Freestanding fixes (v1.1):
 *    FIXED: #include "/Users/.../uiox_klibc.h"  →  #include "uiox_klibc.h"
 *    FIXED: fprintf(stderr, ...)                 →  printf(...)
 *    FIXED: MAX_BLOCKS undeclared               →  defined in fs_types.h
 *    FIXED: disk[] unused-variable warning       →  resolved by MAX_BLOCKS fix
 */
#include "buffer.h"
#include "uiox_klibc.h"

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
    else           free_head = b;
    free_tail = b;
}

static BufEntry *hash_lookup(uint32_t blkno)
{
    BufEntry *b = hash_table[blkno % MAX_BUFS];
    while (b) {
        if (b->blkno == blkno && b->valid) return b;
        b = b->next_hash;
    }
    return NULL;
}

static void hash_insert(BufEntry *b)
{
    uint32_t slot  = b->blkno % MAX_BUFS;
    b->next_hash   = hash_table[slot];
    hash_table[slot] = b;
}

static void hash_remove(BufEntry *b)
{
    uint32_t  slot = b->blkno % MAX_BUFS;
    BufEntry **pp  = &hash_table[slot];
    while (*pp) {
        if (*pp == b) { *pp = b->next_hash; b->next_hash = NULL; return; }
        pp = &(*pp)->next_hash;
    }
}

/* ─────────────────────────────────────────────────────────────
 * buf_init
 * ───────────────────────────────────────────────────────────── */
void buf_init(void)
{
    int i;
    memset(disk,        0, sizeof disk);
    memset(cache,       0, sizeof cache);
    memset(hash_table,  0, sizeof hash_table);
    free_head = free_tail = NULL;

    for (i = 0; i < MAX_BUFS; i++) {
        cache[i].blkno = (uint32_t)-1;
        free_list_append(&cache[i]);
    }
    printf("[buf] init: %d cache slots  disk=%d blocks x %d bytes\n",
           MAX_BUFS, MAX_BLOCKS, BLOCK_SIZE);
}

/* ─────────────────────────────────────────────────────────────
 * getblk
 * ───────────────────────────────────────────────────────────── */
BufEntry *getblk(uint32_t blkno)
{
    BufEntry *b = hash_lookup(blkno);
    if (b) {
        if (b->locked)
            printf("  [buf] getblk: blk %u locked — waiting\n", blkno);
        if (b->refcount == 0) free_list_remove(b);
        b->refcount++;
        b->locked = true;
        return b;
    }

    /* Not in cache — grab LRU free entry */
    b = free_head;
    if (!b) {
        printf("[buf] ERROR: getblk: no free buffers!\n");
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
 * buf_sync  — flush all dirty buffers to simulated disk
 * ───────────────────────────────────────────────────────────── */
void buf_sync(void)
{
    int i, n = 0;
    for (i = 0; i < MAX_BUFS; i++) {
        if (cache[i].dirty && cache[i].blkno < MAX_BLOCKS) {
            memcpy(disk[cache[i].blkno], cache[i].data, BLOCK_SIZE);
            cache[i].dirty = false;
            n++;
        }
    }
    printf("[buf] sync: flushed %d dirty buffers\n", n);
}

/* ─────────────────────────────────────────────────────────────
 * buf_print  — debug dump
 * ───────────────────────────────────────────────────────────── */
void buf_print(void)
{
    int i;
    printf("[buf] cache state:\n");
    for (i = 0; i < MAX_BUFS; i++) {
        if (cache[i].valid)
            printf("  [%3d] blk=%u  dirty=%d  locked=%d  ref=%d\n",
                   i, cache[i].blkno, cache[i].dirty,
                   cache[i].locked, cache[i].refcount);
    }
}
