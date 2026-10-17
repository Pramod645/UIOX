/**
 * @file    bcache_plat_dev.c
 * @brief   The LIVE bcache platform I/O — routes a block request through
 *          the device registry to the driver.
 *
 * PATH    30_KIX/32_FS/00_bcache/src/bcache_plat_dev.c
 *
 * WHY IT LIVES IN 32_FS, NOT 10_BSP
 *
 * The hook is declared in bcache_internal.h:55 and DEFINED in
 * bcache_init.c:143 — both inside 32_FS.  Nothing in 10_BSP declares or
 * defines any bcache_plat_* symbol; the two comments that name 10_BSP
 * (bcache_internal.h:15, bcache_types.h:209) are stale prose.
 *
 * The layering forbids it there anyway.  This body calls
 * uiox_dev_lookup() — a kernel-side symbol in 30_DSS/src — and the BSP
 * is what the kernel stands on, not the other way round.  A BSP file
 * that included uiox_devclass.h would invert the dependency.
 *
 * THE SPLIT (option B)
 *
 *     bcache_plat_dram.c   ← bcache_init.c's original four, unchanged:
 *                            a DRAM region, so the fs can be exercised
 *                            before any driver exists
 *     bcache_plat_dev.c    ← THIS FILE: a real device via the registry
 *
 * 32_FS/Makefile links exactly one.  Both define all four hooks, so
 * linking both is a duplicate-definition error — and that error is the
 * build correctly reporting a half-done swap.
 *
 * @version 1.0.0  @date 2026-10-16
 */

 #include "uiox_devclass.h"      /* uiox_dev_lookup, the class packing    */
 #include "bcache_internal.h"    /* the four hooks being defined here     */
 #include "uiox_devops.h"        /* uiox_blk_ops_t — the vtable to cast to*/
 
 /* ═════════════════════════════════════════════════════════════════════
  * dev  ->  uiox_dev_dev_t *
  *
  * Family 1 is BLOCK; uiox_devclass.h section 2 declares its four classes
  * as UIOX_DEVCLASS_(UIOX_DEVFAM_BLOCK, 0..3).  The packing in section 1
  * makes (BLOCK << 12) | dev exact, with no table between them.
  *
  * The unit is always 0 today — a second eMMC would be a second UNIT of
  * the same class, not a second class, and bcache's flat uint8_t cannot
  * express that.  When it needs to, this is the function that changes.
  * ═════════════════════════════════════════════════════════════════════ */
 static uiox_dev_dev_t *blockdev_of(uint8_t dev)
 {
     //if ((uint32_t)dev > UIOX_DEVINDEX_MASK)
       //  return NULL;            /* would alias into another family */
 
     return uiox_dev_lookup(UIOX_DEVCLASS_(UIOX_DEVFAM_BLOCK, dev), 0u);
 }
 
 void bcache_plat_read_block(uint8_t dev, uint32_t blkno, uint8_t *buf)
 {
     uiox_dev_dev_t *d = blockdev_of(dev);
     if (!d || !d->ops)
         return;
 
     const uiox_blk_ops_t *ops = (const uiox_blk_ops_t *)d->ops;
     if (ops->read)
         (void)ops->read(d, blkno, 1u, buf);
 }
 
 void bcache_plat_write_block(uint8_t dev, uint32_t blkno, const uint8_t *buf)
 {
     uiox_dev_dev_t *d = blockdev_of(dev);
     if (!d || !d->ops)
         return;
 
     const uiox_blk_ops_t *ops = (const uiox_blk_ops_t *)d->ops;
     if (ops->write)
         (void)ops->write(d, blkno, 1u, buf);
 }
 
 /* 0 means UNKNOWN — bcache_internal.h: callers skip the bound check
  * rather than treating every block as past the end. */
 uint32_t bcache_plat_num_blocks(uint8_t dev)
 {
     uiox_dev_dev_t *d = blockdev_of(dev);
     if (!d || !d->ops)
         return 0u;
 
     const uiox_blk_ops_t *ops = (const uiox_blk_ops_t *)d->ops;
     return ops->num_blocks ? ops->num_blocks(d) : 0u;
 }
 
 /* ═════════════════════════════════════════════════════════════════════
  * DRAM base — NOT the SoC's answer
  *
  * With a real device bound, nothing here needs a DRAM window: reads and
  * writes go to the card.  Returning 0 says "no stand-in region", which
  * is true in this build.
  *
  * The SoC's dram_base is 03_SoC data (uiox_soc_types.h:131, filled per
  * arch from SOC_DRAM_BASE) and is read by bcache_plat_dram.c, which is
  * the file that actually wants it.  A live build does not call into the
  * SoC from inside bcache.
  * ═════════════════════════════════════════════════════════════════════ */
 uintptr_t bcache_plat_dram_base(void)
 {
     return 0u;
 }
 