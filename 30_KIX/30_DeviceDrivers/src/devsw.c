/*
 * src/devsw.c
 *
 * Device switch tables and Algorithms 1–5 + strategy + interrupt.
 *
 *   Algorithm 1  dev_open      — open a device special file
 *   Algorithm 2  dev_close     — close a device special file
 *   Algorithm 3  dev_read      — read from a device
 *   Algorithm 4  dev_write     — write to a device
 *   Algorithm 5  dev_ioctl     — device-specific control (char only)
 *                dev_strategy  — strategy interface (block I/O)
 *                dev_interrupt_handler — interrupt vector dispatcher
 */

 #include "devsw.h"
 #include <stdio.h>
 #include <string.h>
 #include <errno.h>
 
 /* =============================================================
  * Forward declarations for per-driver stub routines
  * ============================================================= */
 
 /* ── console (chr major 0) ─────────────────────────────────── */
 static int conopen (unsigned int minor, int mode);
 static int conclose(unsigned int minor);
 static int conread (unsigned int minor, char *buf, int count);
 static int conwrite(unsigned int minor, const char *buf, int count);
 static int conioctl (unsigned int minor, unsigned int cmd, unsigned long arg);
 
 /* ── dzb async terminal (chr major 1) ──────────────────────── */
 static int dzbopen (unsigned int minor, int mode);
 static int dzbclose(unsigned int minor);
 static int dzbread (unsigned int minor, char *buf, int count);
 static int dzbwrite(unsigned int minor, const char *buf, int count);
 static int dzbioctl(unsigned int minor, unsigned int cmd, unsigned long arg);
 
 /* ── /dev/tty (chr major 2) ────────────────────────────────── */
 static int syopen (unsigned int minor, int mode);
 static int syread (unsigned int minor, char *buf, int count);
 static int sywrite(unsigned int minor, const char *buf, int count);
 static int syioctl(unsigned int minor, unsigned int cmd, unsigned long arg);
 
 /* ── /dev/mem (chr major 3) ────────────────────────────────── */
 static int mmread (unsigned int minor, char *buf, int count);
 static int mmwrite(unsigned int minor, const char *buf, int count);
 
 /* ── gd disk (blk major 0, chr major 4) ────────────────────── */
 static int gdopen    (unsigned int minor, int mode);
 static int gdclose   (unsigned int minor);
 static int gdstrategy(BufHdr *bp);
 static int gdread    (unsigned int minor, char *buf, int count);
 static int gdwrite   (unsigned int minor, const char *buf, int count);
 
 /* ── gt tape (blk major 1, chr major 5) ────────────────────── */
 static int gtopen    (unsigned int minor, int mode);
 static int gtclose   (unsigned int minor);
 static int gtstrategy(BufHdr *bp);
 static int gtread    (unsigned int minor, char *buf, int count);
 static int gtwrite   (unsigned int minor, const char *buf, int count);
 
 /* =============================================================
  * Device switch tables
  *
  * Block device switch table:
  *   entry  open      close      strategy
  *   ─────  ────────  ─────────  ───────────
  *   0      gdopen    gdclose    gdstrategy
  *   1      gtopen    gtclose    gtstrategy
  *
  * Character device switch table:
  *   entry  open      close      read      write      ioctl
  *   ─────  ────────  ─────────  ────────  ─────────  ────────
  *   0      conopen   conclose   conread   conwrite   conioctl
  *   1      dzbopen   dzbclose   dzbread   dzbwrite   dzbioctl
  *   2      syopen    nulldev    syread    sywrite    syioctl
  *   3      nulldev   nulldev    mmread    mmwrite    nodev
  *   4      gdopen    gdclose    gdread    gdwrite    nodev
  *   5      gtopen    gtclose    gtread    gtwrite    nodev
  * ============================================================= */
 
 BlkDevEntry blk_dev_sw[MAJOR_BLK_MAX] = {
     [0] = { gdopen, gdclose, gdstrategy },
     [1] = { gtopen, gtclose, gtstrategy },
     /* [2]..[7] zero-initialised → NULL → -ENODEV at dispatch */
 };
 
 ChrDevEntry chr_dev_sw[MAJOR_CHR_MAX] = {
     [0] = { conopen,      conclose,      conread,  conwrite, conioctl  },
     [1] = { dzbopen,      dzbclose,      dzbread,  dzbwrite, dzbioctl  },
     [2] = { syopen,       nulldev_close, syread,   sywrite,  syioctl   },
     [3] = { nulldev_open, nulldev_close, mmread,   mmwrite,  nodev_ioctl },
     [4] = { gdopen,       gdclose,       gdread,   gdwrite,  nodev_ioctl },
     [5] = { gtopen,       gtclose,       gtread,   gtwrite,  nodev_ioctl },
 };
 
 /* =============================================================
  * Internal helpers
  * ============================================================= */
 
 static int validate_blk(unsigned int major)
 {
     if (major >= MAJOR_BLK_MAX) {
         fprintf(stderr, "[devsw] block major %u out of range\n", major);
         return -ENODEV;
     }
     return 0;
 }
 
 static int validate_chr(unsigned int major)
 {
     if (major >= MAJOR_CHR_MAX) {
         fprintf(stderr, "[devsw] char major %u out of range\n", major);
         return -ENODEV;
     }
     return 0;
 }
 
 /* =============================================================
  * Algorithm 1 — dev_open
  *
  * input:  dev (major:minor), openmode, dev_type
  * output: 0 on success; negative errno on failure
  *
  *   convert pathname to inode (done by VFS above us);
  *   get major, minor number from inode;
  *   save context (setjmp) in case of long jump from driver;
  *   if (block device)
  *       use major to index block device switch table;
  *       call driver open, pass minor and open mode;
  *   else
  *       use major to index character device switch table;
  *       call driver open, pass minor and open mode;
  *   if (open fails in driver)
  *       decrement file table, inode counts;   ← VFS handles this
  * ============================================================= */
 int dev_open(uint32_t dev, int mode, int dev_type)
 {
     unsigned int major = DEV_MAJOR(dev);
     unsigned int minor = DEV_MINOR(dev);
     int          ret;
 
     printf("[dev_open] dev=%u:%u mode=0x%x type=%d\n",
            major, minor, mode, dev_type);
 
     /* save context (setjmp) — modelled as plain call; driver
      * signals failure via non-zero return instead of longjmp   */
 
     if (dev_type == DEV_TYPE_BLOCK) {
 
         ret = validate_blk(major);
         if (ret) return ret;
         if (!blk_dev_sw[major].bde_open) return -ENODEV;
 
         /* use major as index to block device switch table;
          * call driver open, pass minor and open modes          */
         ret = blk_dev_sw[major].bde_open(minor, mode);
 
     } else if (dev_type == DEV_TYPE_CHAR) {
 
         ret = validate_chr(major);
         if (ret) return ret;
         if (!chr_dev_sw[major].cde_open) return -ENODEV;
 
         /* use major as index to character device switch table;
          * call driver open, pass minor and open modes          */
         ret = chr_dev_sw[major].cde_open(minor, mode);
 
     } else {
         return -EINVAL;
     }
 
     /* if open fails in driver: decrement file table, inode counts
      * — the VFS does this automatically on a non-zero return     */
     if (ret)
         fprintf(stderr, "[dev_open] driver refused %u:%u err=%d\n",
                 major, minor, ret);
 
     return ret;
 }
 
 /* =============================================================
  * Algorithm 2 — dev_close
  *
  * input:  dev, dev_type, is_mounted
  * output: 0 on success; negative errno on failure
  *
  *   do regular close algorithm();
  *   if (file table reference count not 0) goto finish;
  *   if (another open file has same major:minor) goto finish;
  *   if (character device)
  *       use major to index character device switch table;
  *       call driver close, parameter minor number;
  *   if (block device)
  *       if (device mounted) goto finish;
  *       write device blocks in buffer cache to device;
  *       use major to index block device switch table;
  *       call driver close, parameter minor number;
  *       invalidate device blocks still in buffer cache;
  * finish:
  *   release inode;
  * ============================================================= */
 int dev_close(uint32_t dev, int dev_type, int is_mounted)
 {
     unsigned int major = DEV_MAJOR(dev);
     unsigned int minor = DEV_MINOR(dev);
     int          ret   = 0;
 
     printf("[dev_close] dev=%u:%u type=%d mounted=%d\n",
            major, minor, dev_type, is_mounted);
 
     if (dev_type == DEV_TYPE_CHAR) {
 
         ret = validate_chr(major);
         if (ret) return ret;
         if (!chr_dev_sw[major].cde_close) return -ENODEV;
 
         /* use major to index character device switch table;
          * call driver close, parameter minor number           */
         ret = chr_dev_sw[major].cde_close(minor);
 
     } else if (dev_type == DEV_TYPE_BLOCK) {
 
         /* if device is still mounted, skip driver close        */
         if (is_mounted) {
             printf("[dev_close] blk %u:%u mounted — skipping\n",
                    major, minor);
             goto finish;
         }
 
         ret = validate_blk(major);
         if (ret) return ret;
         if (!blk_dev_sw[major].bde_close) return -ENODEV;
 
         /* write device blocks in buffer cache to device
          * — handled by the block layer / VFS above us          */
 
         /* use major to index block device switch table;
          * call driver close, parameter minor number            */
         ret = blk_dev_sw[major].bde_close(minor);
 
         /* invalidate device blocks still in buffer cache
          * — handled by the block layer on our return           */
 
     } else {
         return -EINVAL;
     }
 
 finish:
     /* release inode — VFS decrements refcount on return        */
     return ret;
 }
 
 /* =============================================================
  * Algorithm 3 — dev_read
  *
  * Character device: invoke the driver read procedure directly.
  * Block device:     build a BufHdr and call the strategy interface.
  * ============================================================= */
 int dev_read(uint32_t dev, int dev_type,
              char *buf, int count, uint32_t *pos)
 {
     unsigned int major = DEV_MAJOR(dev);
     unsigned int minor = DEV_MINOR(dev);
     int          ret;
 
     printf("[dev_read] dev=%u:%u count=%d pos=%u\n",
            major, minor, count, pos ? *pos : 0);
 
     if (dev_type == DEV_TYPE_CHAR) {
 
         ret = validate_chr(major);
         if (ret) return ret;
         if (!chr_dev_sw[major].cde_read) return -ENODEV;
         return chr_dev_sw[major].cde_read(minor, buf, count);
 
     } else if (dev_type == DEV_TYPE_BLOCK) {
 
         /* Build a buffer header and call strategy.
          * In a full kernel, bread() / the buffer cache sits here. */
         BufHdr bp;
         memset(&bp, 0, sizeof bp);
         bp.b_flags  = B_READ;
         bp.b_dev    = dev;
         bp.b_blkno  = pos ? (*pos / BLOCK_SIZE) : 0;
         bp.b_bcount = (uint32_t)count;
         bp.b_data   = buf;   /* buffer cache would supply the page  */
 
         return dev_strategy(&bp);
     }
 
     return -EINVAL;
 }
 
 /* =============================================================
  * Algorithm 4 — dev_write
  *
  * Symmetric to dev_read; direction becomes B_WRITE.
  * ============================================================= */
 int dev_write(uint32_t dev, int dev_type,
               const char *buf, int count, uint32_t *pos)
 {
     unsigned int major = DEV_MAJOR(dev);
     unsigned int minor = DEV_MINOR(dev);
     int          ret;
 
     printf("[dev_write] dev=%u:%u count=%d pos=%u\n",
            major, minor, count, pos ? *pos : 0);
 
     if (dev_type == DEV_TYPE_CHAR) {
 
         ret = validate_chr(major);
         if (ret) return ret;
         if (!chr_dev_sw[major].cde_write) return -ENODEV;
         return chr_dev_sw[major].cde_write(minor, buf, count);
 
     } else if (dev_type == DEV_TYPE_BLOCK) {
 
         BufHdr bp;
         memset(&bp, 0, sizeof bp);
         bp.b_flags  = B_WRITE;
         bp.b_dev    = dev;
         bp.b_blkno  = pos ? (*pos / BLOCK_SIZE) : 0;
         bp.b_bcount = (uint32_t)count;
         bp.b_data   = (void *)buf;
 
         return dev_strategy(&bp);
     }
 
     return -EINVAL;
 }
 
 /* =============================================================
  * Algorithm 5 — dev_ioctl
  *
  * Provides device-specific control for character devices only.
  * Not applicable to regular files or block devices.
  * Returns -ENOTTY when the operation is unavailable.
  * ============================================================= */
 int dev_ioctl(uint32_t dev, unsigned int cmd, unsigned long arg)
 {
     unsigned int major = DEV_MAJOR(dev);
     unsigned int minor = DEV_MINOR(dev);
     int          ret;
 
     printf("[dev_ioctl] dev=%u:%u cmd=0x%x\n", major, minor, cmd);
 
     ret = validate_chr(major);
     if (ret) return -ENOTTY;
     if (!chr_dev_sw[major].cde_ioctl) return -ENOTTY;
 
     return chr_dev_sw[major].cde_ioctl(minor, cmd, arg);
 }
 
 /* =============================================================
  * Strategy interface — dev_strategy
  *
  * Routes a BufHdr to the driver strategy routine for the device.
  * The header carries the scatter-gather list for DMA engines.
  * Strategy may queue the request (elevator algorithm) or execute
  * it immediately; sets B_DONE on completion, B_ERROR on failure.
  * ============================================================= */
 int dev_strategy(BufHdr *bp)
 {
     unsigned int major;
     int          ret;
 
     if (!bp) return -EINVAL;
 
     major = DEV_MAJOR(bp->b_dev);
 
     printf("[dev_strategy] dev=%u:%u blkno=%u flags=0x%x count=%u\n",
            DEV_MAJOR(bp->b_dev), DEV_MINOR(bp->b_dev),
            bp->b_blkno, bp->b_flags, bp->b_bcount);
 
     ret = validate_blk(major);
     if (ret) return ret;
     if (!blk_dev_sw[major].bde_strategy) return -ENODEV;
 
     return blk_dev_sw[major].bde_strategy(bp);
 }
 
 /* =============================================================
  * Interrupt handler dispatcher
  *
  * When a device interrupt occurs the system identifies the
  * interrupting device from the vector table offset and calls
  * this handler.  dev_id encodes (major << 8 | hw_unit).
  *
  * The hw_unit number in the interrupt identifies a hardware unit;
  * the driver maps that to the minor number used in device files.
  * ============================================================= */
 void dev_interrupt_handler(int irq, void *dev_id)
 {
     unsigned int hw_unit = (unsigned int)(size_t)dev_id & 0xFF;
     unsigned int major   = ((unsigned int)(size_t)dev_id >> 8) & 0xFF;
 
     printf("[interrupt] IRQ=%d major=%u hw_unit=%u\n",
            irq, major, hw_unit);
 
     /*
      * A real driver would here:
      *   1. Acknowledge the interrupt to the hardware.
      *   2. Map hw_unit → minor number.
      *   3. Complete pending I/O:
      *        bp->b_flags |= B_DONE;
      *        wake up processes sleeping on this bp.
      *   4. Schedule the next queued request (elevator).
      *   5. For tty: tty_receive_chars() on input interrupt.
      */
 }
 
 /* =============================================================
  * devsw_init
  * ============================================================= */
 void devsw_init(void)
 {
     printf("[devsw] block switch table: %d entries\n", MAJOR_BLK_MAX);
     printf("[devsw] char  switch table: %d entries\n", MAJOR_CHR_MAX);
 }
 
 /* =============================================================
  * devsw_print — debug dump of switch tables
  * ============================================================= */
 void devsw_print(void)
 {
     int i;
     printf("[devsw] Block device switch table:\n");
     printf("  entry  open      close     strategy\n");
     for (i = 0; i < MAJOR_BLK_MAX; i++) {
         if (blk_dev_sw[i].bde_open)
             printf("  %-6d %-9s %-9s %-9s\n", i,
                    blk_dev_sw[i].bde_open     ? "set" : "null",
                    blk_dev_sw[i].bde_close    ? "set" : "null",
                    blk_dev_sw[i].bde_strategy ? "set" : "null");
     }
     printf("[devsw] Char device switch table:\n");
     printf("  entry  open  close  read  write  ioctl\n");
     for (i = 0; i < MAJOR_CHR_MAX; i++) {
         if (chr_dev_sw[i].cde_open)
             printf("  %-6d %-5s %-6s %-5s %-6s %-5s\n", i,
                    chr_dev_sw[i].cde_open  ? "set" : "null",
                    chr_dev_sw[i].cde_close ? "set" : "null",
                    chr_dev_sw[i].cde_read  ? "set" : "null",
                    chr_dev_sw[i].cde_write ? "set" : "null",
                    chr_dev_sw[i].cde_ioctl ? "set" : "null");
     }
 }
 
 /* =============================================================
  * Null / no-op helpers
  * ============================================================= */
 int nulldev_open (unsigned int minor, int mode)
     { (void)minor;(void)mode; return 0; }
 int nulldev_close(unsigned int minor)
     { (void)minor; return 0; }
 int nulldev_read (unsigned int minor, char *buf, int count)
     { (void)minor;(void)buf; return count; }
 int nulldev_write(unsigned int minor, const char *buf, int count)
     { (void)minor;(void)buf; return count; }
 int nodev_ioctl  (unsigned int minor, unsigned int cmd, unsigned long arg)
     { (void)minor;(void)cmd;(void)arg; return -ENOTTY; }
 int nodev_strategy(BufHdr *bp)
     { (void)bp; return -ENODEV; }
 
 /* =============================================================
  * Per-driver stub implementations
  * Replace each body with real hardware-specific code.
  * ============================================================= */
 
 /* ── console ──────────────────────────────────────────────── */
 static int conopen(unsigned int minor, int mode)
 { printf("  [con] open  minor=%u mode=0x%x\n",minor,mode); return 0; }
 static int conclose(unsigned int minor)
 { printf("  [con] close minor=%u\n",minor); return 0; }
 static int conread(unsigned int minor, char *buf, int count)
 { printf("  [con] read  minor=%u count=%d\n",minor,count);
   (void)buf; return 0; }
 static int conwrite(unsigned int minor, const char *buf, int count)
 { printf("  [con] write minor=%u count=%d\n",minor,count);
   (void)buf; return count; }
 static int conioctl(unsigned int minor, unsigned int cmd, unsigned long arg)
 { printf("  [con] ioctl minor=%u cmd=0x%x\n",minor,cmd);
   (void)arg; return 0; }
 
 /* ── dzb async terminal ───────────────────────────────────── */
 static int dzbopen(unsigned int minor, int mode)
 { printf("  [dzb] open  minor=%u mode=0x%x\n",minor,mode); return 0; }
 static int dzbclose(unsigned int minor)
 { printf("  [dzb] close minor=%u\n",minor); return 0; }
 static int dzbread(unsigned int minor, char *buf, int count)
 { (void)buf; return 0; }
 static int dzbwrite(unsigned int minor, const char *buf, int count)
 { (void)buf; return count; }
 static int dzbioctl(unsigned int minor, unsigned int cmd, unsigned long arg)
 { (void)cmd;(void)arg; return 0; }
 
 /* ── /dev/tty ─────────────────────────────────────────────── */
 static int syopen(unsigned int minor, int mode)
 { printf("  [sy] open controlling tty minor=%u\n",minor);
   (void)mode; return 0; }
 static int syread(unsigned int minor, char *buf, int count)
 { (void)buf; return 0; }
 static int sywrite(unsigned int minor, const char *buf, int count)
 { (void)buf; return count; }
 static int syioctl(unsigned int minor, unsigned int cmd, unsigned long arg)
 { (void)cmd;(void)arg; return 0; }
 
 /* ── /dev/mem ─────────────────────────────────────────────── */
 static int mmread(unsigned int minor, char *buf, int count)
 { printf("  [mm] read %d bytes from phys mem\n",count);
   (void)buf; return 0; }
 static int mmwrite(unsigned int minor, const char *buf, int count)
 { printf("  [mm] write %d bytes to phys mem\n",count);
   (void)buf; return count; }
 
 /* ── gd disk ──────────────────────────────────────────────── */
 static int gdopen(unsigned int minor, int mode)
 { printf("  [gd] disk open  minor=%u\n",minor); (void)mode; return 0; }
 static int gdclose(unsigned int minor)
 { printf("  [gd] disk close minor=%u\n",minor); return 0; }
 static int gdstrategy(BufHdr *bp)
 { printf("  [gd] strategy blkno=%u flags=0x%x count=%u\n",
          bp->b_blkno, bp->b_flags, bp->b_bcount);
   bp->b_flags |= B_DONE; return 0; }
 static int gdread(unsigned int minor, char *buf, int count)
 { (void)buf; return 0; }
 static int gdwrite(unsigned int minor, const char *buf, int count)
 { (void)buf; return count; }
 
 /* ── gt tape ──────────────────────────────────────────────── */
 static int gtopen(unsigned int minor, int mode)
 { printf("  [gt] tape open  minor=%u\n",minor); (void)mode; return 0; }
 static int gtclose(unsigned int minor)
 { printf("  [gt] tape close minor=%u\n",minor); return 0; }
 static int gtstrategy(BufHdr *bp)
 { printf("  [gt] strategy blkno=%u flags=0x%x count=%u\n",
          bp->b_blkno, bp->b_flags, bp->b_bcount);
   bp->b_flags |= B_DONE; return 0; }
 static int gtread(unsigned int minor, char *buf, int count)
 { (void)buf; return 0; }
 static int gtwrite(unsigned int minor, const char *buf, int count)
 { (void)buf; return count; }
 