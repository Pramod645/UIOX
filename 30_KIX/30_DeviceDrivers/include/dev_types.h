#ifndef UIOX_DEV_TYPES_H
#define UIOX_DEV_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

/* =============================================================
 * Device subsystem geometry constants
 * ============================================================= */

#define BLOCK_SIZE          512     /* bytes per disk block              */
#define MAX_BLOCKS          1024    /* simulated disk blocks             */

/* Device switch table sizes */
#define MAJOR_BLK_MAX       8       /* block device switch table entries */
#define MAJOR_CHR_MAX       8       /* char  device switch table entries */
#define MAX_MINOR           16      /* minor devices per major           */

/* cblock / clist sizing */
#define CBLOCK_DATA_SIZE    64      /* usable bytes per cblock           */
#define CBLOCK_POOL_SIZE    256     /* total cblocks in the free pool    */
#define TTY_FLOOD_HIGH      512     /* output clist high-water mark      */
#define TTY_FLOOD_LOW       256     /* output clist low-water mark       */

/* PTY */
#define MAX_PTY_PAIRS       8       /* pseudo-tty pairs for mpx          */
#define MPX_HDR_SIZE        4       /* bytes of window-ID header         */

/* Login */
#define LOGIN_MAX_ATTEMPTS  3

/* Device type codes — mirror inode mode file-type bits */
#define DEV_TYPE_BLOCK      0x01
#define DEV_TYPE_CHAR       0x02

/* Buffer state flags */
#define B_READ              0x001   /* direction: device → memory        */
#define B_WRITE             0x002   /* direction: memory → device        */
#define B_DONE              0x004   /* transfer complete                 */
#define B_ERROR             0x008   /* transfer ended in error           */
#define B_BUSY              0x010   /* buffer is claimed                 */
#define B_ASYNC             0x020   /* do not sleep for completion       */
#define B_INVAL             0x040   /* contents are stale                */
#define B_DELWRI            0x080   /* delayed write                     */

/* Scatter-gather list depth (for DMA transfers) */
#define BUF_MAX_SG          16

/* Open mode bits */
#define DEV_O_RDONLY        0x00
#define DEV_O_WRONLY        0x01
#define DEV_O_RDWR          0x02
#define DEV_O_NONBLOCK      0x04

/* ioctl command codes */
#define TIOCSETCANON        0x0001  /* enter canonical mode              */
#define TIOCSETRAW          0x0002  /* enter raw mode                    */
#define TIOCSETECHO         0x0003  /* enable echo                       */
#define TIOCCLRECHO         0x0004  /* disable echo                      */
#define TIOCGWINSZ          0x0005  /* get window size                   */
#define TIOCSWINSZ          0x0006  /* set window size                   */
#define TIOCSFLUSH          0x0007  /* flush queues                      */

/* =============================================================
 * Buffer header — passed to every strategy routine
 *
 * Contains a scatter-gather list of (physical address, size) pairs
 * so DMA engines can transfer across non-contiguous pages.
 * ============================================================= */
typedef struct BufHdr {
    uint32_t    b_flags;            /* B_READ / B_WRITE / B_DONE …       */
    uint32_t    b_dev;              /* (major << 8) | minor              */
    uint32_t    b_blkno;           /* logical block number              */
    void       *b_data;            /* kernel virtual address            */
    uint32_t    b_bcount;          /* bytes requested                   */
    uint32_t    b_resid;           /* bytes not transferred on error    */
    int         b_error;           /* errno if B_ERROR set              */

    /* Scatter-gather list */
    struct {
        uint32_t sg_addr;          /* physical page address             */
        uint32_t sg_len;           /* byte count for this segment       */
    }           b_sg[BUF_MAX_SG];
    int         b_nsg;             /* valid sg entries                  */

    struct BufHdr *b_next;         /* next request on driver work queue */
} BufHdr;

/* Encode / decode b_dev */
#define DEV_MAJOR(dev)   (((dev) >> 8) & 0xFF)
#define DEV_MINOR(dev)   ((dev) & 0xFF)
#define DEV_MAKE(ma,mi)  (((ma) << 8) | (mi))

#endif /* UIOX_DEV_TYPES_H */
