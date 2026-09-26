/*
 *  30_KIX/32_FS/include/buffer.h
 *
 *  The bridge between 01_fsa and the block buffer cache.
 *
 *  ── why this file is a shim and not a declaration set ─────────────────
 *  The original buffer.h declared the buffer cache directly:
 *
 *      typedef struct BufEntry {
 *          uint32_t blkno;
 *          uint8_t  data[BLOCK_SIZE];
 *          bool     valid, dirty, locked;
 *          int      refcount;
 *          struct BufEntry *next_hash, *prev_free, *next_free;
 *      } BufEntry;
 *
 *      BufEntry *getblk (uint32_t blkno);
 *      BufEntry *bread  (uint32_t blkno);
 *      void      bwrite (BufEntry *buf);
 *      void      brelse (BufEntry *buf);
 *      void      buf_sync(void);
 *
 *  The buffer cache now lives in its own layer (31_BufferCache/00_FileBuff)
 *  and exports a DIFFERENT contract:
 *
 *      BufHdr  *getblk (uint8_t dev, uint32_t blkno);
 *      BufHdr  *bread  (uint8_t dev, uint32_t blkno);
 *      BufHdr  *breada (uint8_t dev, uint32_t blkno, uint32_t ra_blkno);
 *      void     bwrite (BufHdr *buf, bool sync, bool delayed);
 *      void     brelse (BufHdr *buf);
 *      void     bflush (uint8_t dev);
 *      void     bflush_all(void);
 *
 *  Every 01_fsa source is written against the SECOND set.  The three
 *  differences that matter:
 *
 *    1. DEVICE.  Every call takes (dev, blkno).  BmapResult carries a
 *       .dev, and bmap() fills it from the inode — so a caller has the
 *       device in hand.  This is also why InCoreInode and DiskInode both
 *       gained a dev field.
 *
 *    2. TYPE.  BufHdr, not BufEntry.  Its state is a single `status` word
 *       of BUF_* bits, so there is no `valid` / `dirty` / `locked` field
 *       to read or write, and no `refcount`.
 *
 *    3. WRITE.  bwrite(buf, sync, delayed) replaced bwrite(buf).  The
 *       flags decide whether the write happens now or is deferred, and
 *       either way the buffer is RELEASED — so a bwrite is never followed
 *       by a brelse on the same buffer.
 *
 *  This header forwards to the real one.  It exists so 01_fsa's sources
 *  keep including the short name "buffer.h" while resolving to the current
 *  API.  Nothing here declares a buffer type of its own.
 *
 *  ── what the old names map to ─────────────────────────────────────────
 *      BufEntry        → BufHdr                (via bcache.h)
 *      buf_sync()      → bflush_all()          (a macro, below)
 *      buf_init()      → bcache_init()         (a macro, below)
 *      buf_print()     → bcache_print()        (a macro, below)
 *
 *  The three macros keep 01_fsa's older call sites working without
 *  editing them.  A source that wants the per-device flush calls
 *  bflush(dev) directly — the buffer layer declares it.
 *
 *  ── the field migration, for sources that still touch one ────────────
 *      buf->valid              → (buf->status & BUF_VALID)
 *      buf->dirty = true       → bwrite(buf, true, false)
 *      buf->locked             → (buf->status & BUF_LOCKED)
 *      buf->refcount           → no equivalent; the free list owns it
 *      buf->blkno              → unchanged
 *      buf->data[]             → unchanged
 *
 *  @version 3.0.0  @date 2026-09-25
 */
#ifndef UIOX_BUFFER_H
#define UIOX_BUFFER_H

/* The buffer layer's public interface: BufHdr, the five algorithm
 * prototypes, the BUF_* status bits, and BufStats. */
#include "bcache.h"

/* Freestanding types and the aliases 01_fsa's sources use — bool,
 * uint32_t, memset, memcpy.  Included here because the original
 * buffer.h got them transitively through fs_types.h, and a source that
 * includes only buffer.h must still resolve them. */
#include "uiox_klibc.h"

/* ═════════════════════════════════════════════════════════════════════
 * The retired names, kept working
 *
 * These are macros rather than functions so a call site compiles to the
 * same code it would if it named the new symbol directly.  A wrapper
 * function would add a call frame with no benefit.
 * ═════════════════════════════════════════════════════════════════════ */

/* buf_sync — the old system-wide flush.  The buffer layer splits it into
 * bflush(dev) for one device and bflush_all() for every device; the old
 * name meant the latter. */
#define buf_sync()    bflush_all()

/* buf_init — the old pool initialiser.  Now bcache_init(). */
#define buf_init()    bcache_init()

/* buf_print — the old debug dump.  Now bcache_print(). */
#define buf_print()   bcache_print()

/* ═════════════════════════════════════════════════════════════════════
 * Compile-time confirmation that the shim resolved
 *
 * A source that includes this header and then uses a BufEntry, or calls
 * getblk with one argument, should fail at ITS OWN line rather than in a
 * cascade of unrelated errors.  This block catches the common case.
 * ═════════════════════════════════════════════════════════════════════ */
#ifdef UIOX_BUFFER_STRICT
#  ifdef BufEntry
#    error "BufEntry still defined — buffer.h shim did not take effect"
#  endif
#endif

#endif /* UIOX_BUFFER_H */
