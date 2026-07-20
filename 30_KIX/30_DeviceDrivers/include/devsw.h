#ifndef UIOX_DEVSW_H
#define UIOX_DEVSW_H

#include "dev_types.h"

/* =============================================================
 * Block device switch table entry
 *
 *   entry  open      close      strategy
 *   ─────  ────────  ─────────  ───────────
 *   0      gdopen    gdclose    gdstrategy   (disk)
 *   1      gtopen    gtclose    gtstrategy   (tape)
 * ============================================================= */
typedef struct {
    int  (*bde_open)    (unsigned int minor, int mode);
    int  (*bde_close)   (unsigned int minor);
    int  (*bde_strategy)(BufHdr *bp);
} BlkDevEntry;

/* =============================================================
 * Character device switch table entry
 *
 *   entry  open      close      read      write      ioctl
 *   ─────  ────────  ─────────  ────────  ─────────  ────────
 *   0      conopen   conclose   conread   conwrite   conioctl
 *   1      dzbopen   dzbclose   dzbread   dzbwrite   dzbioctl
 *   2      syopen    nulldev    syread    sywrite    syioctl
 *   3      nulldev   nulldev    mmread    mmwrite    nodev
 *   4      gdopen    gdclose    gdread    gdwrite    nodev
 *   5      gtopen    gtclose    gtread    gtwrite    nodev
 * ============================================================= */
typedef struct {
    int  (*cde_open)    (unsigned int minor, int mode);
    int  (*cde_close)   (unsigned int minor);
    int  (*cde_read)    (unsigned int minor, char *buf, int count);
    int  (*cde_write)   (unsigned int minor, const char *buf, int count);
    int  (*cde_ioctl)   (unsigned int minor, unsigned int cmd,
                          unsigned long arg);
} ChrDevEntry;

/* =============================================================
 * Global switch tables
 * ============================================================= */
extern BlkDevEntry blk_dev_sw[MAJOR_BLK_MAX];
extern ChrDevEntry chr_dev_sw[MAJOR_CHR_MAX];

/* =============================================================
 * Null / no-op device helpers
 * (used to fill switch table entries with no real operation)
 * ============================================================= */
int nulldev_open  (unsigned int minor, int mode);
int nulldev_close (unsigned int minor);
int nulldev_read  (unsigned int minor, char *buf, int count);
int nulldev_write (unsigned int minor, const char *buf, int count);
int nodev_ioctl   (unsigned int minor, unsigned int cmd, unsigned long arg);
int nodev_strategy(BufHdr *bp);

/* =============================================================
 * Algorithm 1  —  dev_open
 *
 * input:  dev (major:minor encoded), openmode, dev_type
 * output: 0 on success; negative errno on failure
 *
 * Converts pathname to inode (done by VFS above us), gets
 * major/minor from the inode, saves setjmp context, then
 * dispatches to the appropriate driver open via the switch table.
 * On failure the caller decrements file-table and inode counts.
 * ============================================================= */
int dev_open(uint32_t dev, int mode, int dev_type);

/* =============================================================
 * Algorithm 2  —  dev_close
 *
 * input:  dev, dev_type, is_mounted (block devices only)
 * output: 0 on success; negative errno on failure
 *
 * Calls driver close only on the last close (refcount → 0 and
 * no other descriptor refers to same device).  For block devices:
 * flush dirty cache buffers first, then invalidate cache entries.
 * ============================================================= */
int dev_close(uint32_t dev, int dev_type, int is_mounted);

/* =============================================================
 * Algorithm 3  —  dev_read
 *
 * Character device: invoke the driver read procedure directly.
 * Block device:     build a BufHdr and call the strategy interface.
 * ============================================================= */
int dev_read(uint32_t dev, int dev_type,
             char *buf, int count, uint32_t *pos);

/* =============================================================
 * Algorithm 4  —  dev_write
 *
 * Symmetric to dev_read; transfer direction becomes B_WRITE.
 * ============================================================= */
int dev_write(uint32_t dev, int dev_type,
              const char *buf, int count, uint32_t *pos);

/* =============================================================
 * Algorithm 5  —  dev_ioctl
 *
 * Provides device-specific control for character devices only.
 * Not applicable to regular files or block devices — returns
 * -ENOTTY in those cases.
 * ============================================================= */
int dev_ioctl(uint32_t dev, unsigned int cmd, unsigned long arg);

/* =============================================================
 * Strategy interface  —  dev_strategy
 *
 * The kernel passes a BufHdr to the driver strategy routine.
 * The header contains a scatter-gather list of (address, size)
 * pairs.  Strategy may queue the job on a work list for elevator
 * scheduling, or execute it immediately.  Sets B_DONE on
 * completion; sets B_ERROR on failure.
 * ============================================================= */
int dev_strategy(BufHdr *bp);

/* =============================================================
 * Interrupt handler dispatcher
 *
 * Called from the interrupt vector table when a device interrupt
 * occurs.  Identifies the hardware unit and calls the appropriate
 * device-specific completion routine.
 *
 * dev_id encodes (major << 8 | hw_unit).
 * ============================================================= */
void dev_interrupt_handler(int irq, void *dev_id);

/* Initialise the switch tables and driver stubs */
void devsw_init(void);

/* Debug: print switch table contents */
void devsw_print(void);

#endif /* UIOX_DEVSW_H */
