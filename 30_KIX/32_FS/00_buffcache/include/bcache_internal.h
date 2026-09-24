/*
 *  31_BufferCache/00_FileBuff/include/bcache_internal.h
 *
 *  Internals shared by the five buffer-allocation algorithms.
 *  Not part of the public interface — 01_fsa includes bcache.h only.
 *
 *  ── why these are shared rather than static ───────────────────────────
 *  Bach presents getblk/brelse/bread/breada/bwrite as five algorithms
 *  over one pool.  Splitting them into five files means the pool and the
 *  list operations each have to be declared in one place, or each file
 *  would need its own copy — and a second copy of the free list is how
 *  two algorithms come to disagree about the pool's state.
 *
 *  ── FIXED in this revision ─────────────────────────────────────────────
 *  bcache_plat_num_blocks is declared EXTERN here.
 *
 *  bcache_types.h declares it as `static inline __attribute__((weak))`.
 *  A static inline is a private copy in every translation unit that
 *  includes the header, so a BSP cannot replace it at link time — the
 *  header's copy wins inside each unit, and bread.c's past-device-end
 *  check therefore always read NUM_DISK_BLOCKS_DEFAULT.
 *
 *  The declaration below is the one that counts.  To make it effective,
 *  bcache_types.h must DELETE its static inline definition and keep only
 *  the geometry macros; the single definition then lives in
 *  bcache_init.c (or in 10_BSP, which overrides it by not linking ours).
 *
 *  @version 2.1.0  @date 2026-09-24
 */
#ifndef UIOX_BCACHE_INTERNAL_H
#define UIOX_BCACHE_INTERNAL_H

#include "bcache.h"

/* ═════════════════════════════════════════════════════════════════════
 * The pool
 *
 * One array, one sentinel, one hash table.  Defined in bcache_init.c so
 * exactly one translation unit owns the storage.
 * ═════════════════════════════════════════════════════════════════════ */
extern BufHdr bcache_pool[NUM_BUFFERS];

/* ═════════════════════════════════════════════════════════════════════
 * Platform I/O
 *
 * Reads and writes one block.  Overridden from 10_BSP for a real device;
 * the default backs the pool with a DRAM region so the filesystem can be
 * exercised before a driver exists.
 *
 * These are ORDINARY functions, so the BSP overrides them by simply not
 * linking bcache_init.c's definitions — no weak attribute needed.
 * ═════════════════════════════════════════════════════════════════════ */
void bcache_plat_read_block (uint8_t dev, uint32_t blkno, uint8_t *buf);
void bcache_plat_write_block(uint8_t dev, uint32_t blkno, const uint8_t *buf);

/* ═════════════════════════════════════════════════════════════════════
 * Platform geometry — the extern declaration that makes override work
 *
 * FIXES the static-inline problem described in the header note above.
 * One definition, in bcache_init.c; 10_BSP provides its own by not
 * linking ours.
 *
 * A device reports its size in BLOCKS at BCACHE_SECTOR_SIZE.  A return
 * of 0 means "unknown" and callers skip the bound check rather than
 * treating every block as past the end.
 * ═════════════════════════════════════════════════════════════════════ */
uint32_t bcache_plat_num_blocks(uint8_t dev);

/* The DRAM stand-in's base address, for the default backing. */
uintptr_t bcache_plat_dram_base(void);

/* ═════════════════════════════════════════════════════════════════════
 * Hash queue  —  getblk.c
 * ═════════════════════════════════════════════════════════════════════ */
uint32_t bcache_hash_slot  (uint8_t dev, uint32_t blkno);
void     bcache_hash_insert(BufHdr *b);
void     bcache_hash_remove(BufHdr *b);
BufHdr  *bcache_hash_lookup(uint8_t dev, uint32_t blkno);

/* ═════════════════════════════════════════════════════════════════════
 * Free list  —  getblk.c
 *
 * HEAD = least-recently-used (taken first)
 * TAIL = most-recently-used (kept longest)
 * ═════════════════════════════════════════════════════════════════════ */
void     bcache_fl_remove     (BufHdr *b);
void     bcache_fl_insert_tail(BufHdr *b);   /* MRU */
void     bcache_fl_insert_head(BufHdr *b);   /* LRU — evict next */
BufHdr  *bcache_fl_pop_head   (void);
int      bcache_fl_empty      (void);

#endif /* UIOX_BCACHE_INTERNAL_H */
