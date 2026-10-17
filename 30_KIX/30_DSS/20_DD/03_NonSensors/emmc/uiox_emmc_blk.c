/**
 * @file    uiox_emmc_blk.c
 * @brief   The BLOCK-family vtable for eMMC — the one place the
 *          filesystem's dispatch meets the driver.
 *
 * PATH    30_KIX/30_DSS/20_DD/03_NonSensors/emmc/uiox_emmc_blk.c
 *
 * WHY THIS IS A SEPARATE FILE, NOT A BLOCK INSIDE uiox_emmc_if.c
 *
 * uiox_emmc_if.c is the driver: it talks to uiox_emmc_hw_t, sends CMDs,
 * moves DAT lines, reads EXT_CSD.  It takes a uiox_emmc_if_t * and knows
 * everything about eMMC.
 *
 * Everything above it knows nothing about eMMC.  bcache holds a uint8_t
 * dev; the filesystem holds a (cls, unit) pair; the registry returns a
 * uiox_dev_dev_t.  None of them can name uiox_emmc_if_t.
 *
 * So this file is the seam, with one job: unwrap dev->drv_priv back to
 * the driver's typed pointer and hand the call over.  Five functions,
 * each a cast and a forward.
 *
 * It lives under 20_DD/ because that is the layer that owns a driver's
 * implementation — 10_DI declares the ops and never performs them.
 *
 * THE PATH IT COMPLETES
 *
 *     bcache read
 *      → bcache_plat_read_block(dev, blkno, buf)          [10_BSP]
 *        → uiox_dev_lookup(UIOX_DEVCLASS_(BLOCK, dev), 0) [30_DSS/src]
 *          → ((uiox_blk_ops_t*)d->ops)->read(...)         [HERE]
 *            → uiox_emmc_if_read(eif, lba, buf, count)    [uiox_emmc_if.c]
 *              → uiox_fw_emmc_read(&b->fw, ...)           [02_FwHal]
 *
 * @version 1.0.0  @date 2026-10-16
 */

 #include "uiox_emmc_if.h"       /* uiox_emmc_if_t, uiox_emmc_if_read, …   */
 #include "uiox_emmc_hw.h"       /* uiox_emmc_hw_t, capacity_sectors       */
 #include "uiox_devclass.h"      /* uiox_dev_dev_t, UIOX_DEVCLASS_EMMC     */
 #include "uiox_devops.h"        /* uiox_blk_ops_t — the shape being filled*/
 #include "uiox_klibc.h"         /* NULL, errno under -nostdinc            */
 
 /* ═════════════════════════════════════════════════════════════════════
  * The one unwrap.
  *
  * `ops` is written once at bind time and `drv_priv` only by the driver —
  * uiox_devclass.h states both, and that split is what makes this cast
  * safe: nothing outside the driver ever puts a different kind of pointer
  * in drv_priv.
  *
  * Returns NULL rather than dereferencing a half-bound device, so every
  * entry point below can test once and return -ENODEV.
  * ═════════════════════════════════════════════════════════════════════ */
 static uiox_emmc_if_t *eif_of(uiox_dev_dev_t *dev)
 {
     if (!dev || !dev->drv_priv)
         return NULL;
 
     return (uiox_emmc_if_t *)dev->drv_priv;
 }
 
 /* ── read ─────────────────────────────────────────────────────────────
  * bcache counts in BCACHE_SECTOR_SIZE units and uiox_emmc_if_read takes
  * an lba plus a count in the card's own addressing.  Both are 512-byte
  * sectors for eMMC, so the values pass through — but the translation is
  * named here rather than assumed, so a card with a different sector size
  * has one place to change.
  *
  * `primed` is the driver's own readiness flag (uiox_emmc_if.h:41).  A
  * device that has not been through uiox_emmc_if_start() has no valid
  * CSD, no bus width and no clock — reading now would send CMD17 into an
  * uninitialised controller.  -ENODEV is the honest answer.
  * ──────────────────────────────────────────────────────────────────── */
 static int emmc_blk_read(uiox_dev_dev_t *dev, uint32_t blkno,
                          uint32_t count, uint8_t *buf)
 {
     uiox_emmc_if_t *eif = eif_of(dev);
     if (!eif || !eif->primed)
         return -ENODEV;
 
     return uiox_emmc_if_read(eif, blkno, buf, count);
 }
 
 static int emmc_blk_write(uiox_dev_dev_t *dev, uint32_t blkno,
                           uint32_t count, const uint8_t *buf)
 {
     uiox_emmc_if_t *eif = eif_of(dev);
     if (!eif || !eif->primed)
         return -ENODEV;
 
     return uiox_emmc_if_write(eif, blkno, buf, count);
 }
 
 /* ── flush ────────────────────────────────────────────────────────────
  * uiox_emmc_if.h has no flush entry point today, so this is honest about
  * what it can do: return 0.
  *
  * NOT a silent lie.  bcache calls flush at unmount; returning 0 says
  * "there is no write-back state above the driver to lose".  eMMC's own
  * cache is flushed by the driver's CMD path when it issues writes, and a
  * card with a volatile cache needs a CMD with FLUSH (which the driver
  * will grow).  Until then, 0 is correct for a driver whose writes are
  * synchronous.
  * ──────────────────────────────────────────────────────────────────── */
 static int emmc_blk_flush(uiox_dev_dev_t *dev)
 {
     uiox_emmc_if_t *eif = eif_of(dev);
     if (!eif || !eif->primed)
         return -ENODEV;
 
     return 0;
 }
 
 /* ── geometry ─────────────────────────────────────────────────────────
  * capacity_sectors is EXT_CSD SEC_COUNT (uiox_emmc_hw.h:303), already in
  * 512-byte units — the same unit BCACHE_SECTOR_SIZE counts — so no
  * arithmetic is needed.
  *
  * Two things stated plainly rather than hidden:
  *
  *   · 0 means UNKNOWN, per bcache_internal.h.  An unbound device, or one
  *     that has not read EXT_CSD yet, returns 0 and bcache skips its bound
  *     check rather than treating every block as past the end.
  *
  *   · The hook returns uint32_t while the field is uint64_t.  The cast
  *     truncates above ~2 TiB.  A real limit, not a rounding detail, and
  *     widening it means changing bcache_types.h — a deliberate change.
  * ──────────────────────────────────────────────────────────────────── */
 static uint32_t emmc_blk_num_blocks(uiox_dev_dev_t *dev)
 {
     uiox_emmc_if_t *eif = eif_of(dev);
     if (!eif || !eif->hw)
         return 0u;                      /* unknown, not zero-sized */
 
     return (uint32_t)eif->hw->ident.capacity_sectors;
 }
 
 /* ── ioctl ────────────────────────────────────────────────────────────
  * Reached from uiox_kix_scfs_ioctl.c after uiox_dev_lookup(cls, unit)
  * resolved the inode's idev_class.  The card has no control surface
  * defined yet, so -ENOSYS is the correct answer: exactly what the ioctl
  * path returns for a device that takes none, and how a caller learns to
  * stop asking.
  *
  * When the class grows a vocabulary — partition select, bus width, speed
  * mode — the switch goes here, and uiox_emmc_hw.h's MMC_* definitions
  * supply the command numbers.
  * ──────────────────────────────────────────────────────────────────── */
 static int emmc_blk_ioctl(uiox_dev_dev_t *dev, unsigned long cmd, void *arg);
 
 /* ═════════════════════════════════════════════════════════════════════
  * The vtable.
  *
  * `const` because it is written once and never again — the convention
  * uiox_dev_dev_t.ops states: "the vtables — written once at bind time".
  *
  * Not `static`: the bind code in this layer needs the address to put in
  * dev->ops.  If the bind lives in this file too, make it static and
  * expose only the count of one.
  * ═════════════════════════════════════════════════════════════════════ */
 const uiox_blk_ops_t s_emmc_blk_ops = {
     .read       = emmc_blk_read,
     .write      = emmc_blk_write,
     .flush      = emmc_blk_flush,
     .num_blocks = emmc_blk_num_blocks,
     .ioctl      = emmc_blk_ioctl,
 };
 
 static int emmc_blk_ioctl(uiox_dev_dev_t *dev, unsigned long cmd, void *arg)
 {
     uiox_emmc_if_t *eif = eif_of(dev);
     (void)cmd; (void)arg;
 
     if (!eif || !eif->primed)
         return -ENODEV;
 
     /* No command vocabulary for the card yet.  See the note above: this
      * is the answer a device with no control surface gives, and the
      * filesystem's ioctl path is written to accept it. */
     return -ENOSYS;
 }
 