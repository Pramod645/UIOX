/**
 * @file    uiox_devclass.h
 * @brief   The device vocabulary every 34_DSS layer speaks.
 *
 * This is the piece the four DSS layers share: the device classes, the
 * six driver families they group into, the bus a device sits on, the
 * descriptor a driver binds to, and the two ops vtables that were only
 * sketches until now.
 *
 * Why it exists here rather than per class: a driver's class, its
 * family, its buffer requirement and its ops shape are all one decision.
 * Splitting them across per-class headers is how a tree ends up with two
 * vocabularies for one NIC — which this tree has, in uiox_fw_eth.h and
 * uiox_fw_net.h.
 *
 * No consumer references these names yet (verified by grep), so the
 * numbering below is free.
 *
 * @version 1.0.0  @date 2026-10-06
 */
#ifndef UIOX_DEVCLASS_H
#define UIOX_DEVCLASS_H

#include "uiox_klibc.h"     /* uintptr_t, uint16_t, bool under -nostdinc */

#ifdef __cplusplus
extern "C" {
#endif

/* ═════════════════════════════════════════════════════════════════════
 * SECTION 1 — the six driver families
 *
 * A family is what a device does with data, and it decides three things
 * at once: which buffer pool serves it, which ops vtable describes it,
 * and whether the filesystem caches it.
 *
 *   BLOCK   addressed   — a unit index locates the bytes, so 32_FS's
 *                         00_bcache can key them.  Cached, never pooled.
 *   STREAM  unaddressed — bytes or frames with no index.  Pooled by
 *                         31_drvbuff.  Symmetric: tx and rx both exist.
 *   EVENT   records     — fixed-size, produced only.  Queued by
 *                         31_drvbuff (small).  Producer-only.
 *   REG     registers   — a bus read returns a value.  NO buffer.
 *   CMD     commands    — MMIO writes and mode sets.  NO buffer.
 *   BUS     tunnels     — creates other devices.  NO buffer.
 * ═════════════════════════════════════════════════════════════════════ */
#define UIOX_DEVFAM_SHIFT   12u
#define UIOX_DEVFAM_MASK    0xF000u
#define UIOX_DEVINDEX_MASK  0x0FFFu

typedef enum {
    UIOX_DEVFAM_BLOCK  = 0u,   /* addressed; cached by 00_bcache        */
    UIOX_DEVFAM_STREAM = 1u,   /* bytes/frames; pooled by 31_drvbuff    */
    UIOX_DEVFAM_EVENT  = 2u,   /* records; queued by 31_drvbuff         */
    UIOX_DEVFAM_REG    = 3u,   /* bus register reads; no buffer         */
    UIOX_DEVFAM_CMD    = 4u,   /* MMIO / command; no buffer             */
    UIOX_DEVFAM_BUS    = 5u,   /* tunnel / host controller; no buffer   */
    UIOX_DEVFAM__COUNT
} uiox_devfam_t;

/* ── the packing macro ───────────────────────────────────────────────
 * A class value is (family << 12) | index.  That is what lets a reader
 * answer "which vtable is this?" without a lookup table — see the
 * predicates in section 5. */
#define UIOX_DEVCLASS_(fam, n)  \
    ((uiox_devclass_t)((((uint32_t)(fam)) << UIOX_DEVFAM_SHIFT) | ((uint32_t)(n))))

/* ═════════════════════════════════════════════════════════════════════
 * SECTION 2 — the classes, grouped by family
 *
 * Ordering is by family on purpose.  An earlier draft was alphabetical,
 * which said nothing about the device and grouped ETH with EMMC as if
 * they were alike.
 *
 * USB is three classes, not one: mass storage, HID and the host
 * controller are three drivers with three interfaces.
 *
 * tb4 is a BUS, not a block device.  Thunderbolt 4 tunnels PCIe — what
 * it exposes is storage, not what it is.
 * ═════════════════════════════════════════════════════════════════════ */
typedef enum {
    /* ── family 1 — BLOCK; buffer = 00_bcache ──────────────────────── */
    UIOX_DEVCLASS_EMMC       = UIOX_DEVCLASS_(UIOX_DEVFAM_BLOCK, 0),
    UIOX_DEVCLASS_SDMMC      = UIOX_DEVCLASS_(UIOX_DEVFAM_BLOCK, 1),
    UIOX_DEVCLASS_NVME       = UIOX_DEVCLASS_(UIOX_DEVFAM_BLOCK, 2),
    UIOX_DEVCLASS_USBMS      = UIOX_DEVCLASS_(UIOX_DEVFAM_BLOCK, 3),

    /* ── family 2 — STREAM; buffer = 31_drvbuff ──────────────────────
     * Only ETH needs headroom (the protocol stack prepends headers).
     * CAMERA is the exception among these: it produces FRAMES, so its
     * pool is sized in frames and has no headroom. */
    UIOX_DEVCLASS_ETH        = UIOX_DEVCLASS_(UIOX_DEVFAM_STREAM, 0),
    UIOX_DEVCLASS_WIFI       = UIOX_DEVCLASS_(UIOX_DEVFAM_STREAM, 1),
    UIOX_DEVCLASS_BT         = UIOX_DEVCLASS_(UIOX_DEVFAM_STREAM, 2),
    UIOX_DEVCLASS_USBHID     = UIOX_DEVCLASS_(UIOX_DEVFAM_STREAM, 3),
    UIOX_DEVCLASS_MIC        = UIOX_DEVCLASS_(UIOX_DEVFAM_STREAM, 4),
    UIOX_DEVCLASS_SPEAKER    = UIOX_DEVCLASS_(UIOX_DEVFAM_STREAM, 5),
    UIOX_DEVCLASS_CAMERA     = UIOX_DEVCLASS_(UIOX_DEVFAM_STREAM, 6),

    /* ── family 3 — EVENT; buffer = 31_drvbuff (small) ─────────────── */
    UIOX_DEVCLASS_KBD        = UIOX_DEVCLASS_(UIOX_DEVFAM_EVENT, 0),
    UIOX_DEVCLASS_MOUSE      = UIOX_DEVCLASS_(UIOX_DEVFAM_EVENT, 1),
    UIOX_DEVCLASS_TOUCHPWD   = UIOX_DEVCLASS_(UIOX_DEVFAM_EVENT, 2),

    /* ── family 4 — REG; NO buffer ───────────────────────────────────
     * These seven are I2C/SPI register reads.  Their _buf.c/_buf.h pairs
     * under 31_drvbuff are dead files — a lux value has no stream. */
    UIOX_DEVCLASS_ALS        = UIOX_DEVCLASS_(UIOX_DEVFAM_REG, 0),
    UIOX_DEVCLASS_THERMAL    = UIOX_DEVCLASS_(UIOX_DEVFAM_REG, 1),
    UIOX_DEVCLASS_BMS        = UIOX_DEVCLASS_(UIOX_DEVFAM_REG, 2),
    UIOX_DEVCLASS_CHG        = UIOX_DEVCLASS_(UIOX_DEVFAM_REG, 3),
    UIOX_DEVCLASS_PMIC       = UIOX_DEVCLASS_(UIOX_DEVFAM_REG, 4),
    UIOX_DEVCLASS_RTC        = UIOX_DEVCLASS_(UIOX_DEVFAM_REG, 5),
    UIOX_DEVCLASS_FAN        = UIOX_DEVCLASS_(UIOX_DEVFAM_REG, 6),

    /* ── family 5 — CMD; NO buffer ──────────────────────────────────── */
    UIOX_DEVCLASS_GPU        = UIOX_DEVCLASS_(UIOX_DEVFAM_CMD, 0),
    UIOX_DEVCLASS_HDMI       = UIOX_DEVCLASS_(UIOX_DEVFAM_CMD, 1),
    UIOX_DEVCLASS_MONITOR    = UIOX_DEVCLASS_(UIOX_DEVFAM_CMD, 2),

    /* ── family 6 — BUS; NO buffer ───────────────────────────────────
     * Output is a hotplug event, not payload.  A device that attaches
     * takes a class from one of the five families above. */
    UIOX_DEVCLASS_TB4        = UIOX_DEVCLASS_(UIOX_DEVFAM_BUS, 0),
    UIOX_DEVCLASS_USBHC      = UIOX_DEVCLASS_(UIOX_DEVFAM_BUS, 1),

    /* ── console — not an attachable device ─────────────────────────
     * UART is here because the kernel prints through it.  Index 15 keeps
     * it clear of every family's range, so a probe loop cannot mistake
     * the console for hardware to bind. */
    UIOX_DEVCLASS_UART       = UIOX_DEVCLASS_(UIOX_DEVFAM_CMD, 15),

    UIOX_DEVCLASS_NONE       = 0xFFFFu,
    UIOX_DEVCLASS__COUNT     = 0xFFFFu
} uiox_devclass_t;

/* ═════════════════════════════════════════════════════════════════════
 * SECTION 3 — the bus a device sits on
 *
 * Separate from the family: a PMIC and an ALS are both REG but may be on
 * different buses, and a block device may be MMIO (emmc), PCIe (nvme) or
 * USB (mass storage).  The bus decides which firmware driver in 02_FwHal
 * performs the transfer.
 * ═════════════════════════════════════════════════════════════════════ */
typedef enum {
    UIOX_BUS_NONE = 0,
    UIOX_BUS_MMIO,      /* a register window the kernel maps               */
    UIOX_BUS_I2C,       /* addressed slave, 7-bit addr in uiox_dev_dev_t   */
    UIOX_BUS_SPI,       /* chip-select in addr                             */
    UIOX_BUS_PCIE,      /* config space + BAR from the tunnel enumerator   */
    UIOX_BUS_USB        /* endpoint-addressed                              */
} uiox_dev_bus_t;

/* ═════════════════════════════════════════════════════════════════════
 * SECTION 4 — the device a driver binds to
 *
 * Carries identity (cls + unit), where it is, and two private slots with
 * two owners — the same convention uiox_hw_dev_t uses:
 *
 *     .ops       the vtables — written once at bind time
 *     .drv_priv  the driver's own state — written only by the driver
 *
 * `unit` is the same number as InCoreInode.idev_unit, and `cls` the same
 * as .idev_class, which is what lets /dev/kbd0 resolve through the
 * filesystem to this struct with no second lookup.
 * ═════════════════════════════════════════════════════════════════════ */
typedef struct {
    uiox_devclass_t cls;        /* family + class — see section 5        */
    uint16_t        unit;       /* instance index; the inode's idev_unit */

    uiox_dev_bus_t  bus;
    uintptr_t       base;       /* MMIO base; 0 for a bus-attached dev   */
    uint8_t         addr;       /* I2C slave / SPI CS; 0 for MMIO        */
    uint32_t        irq;

    const char     *name;       /* "eth0", "i2c0:0x48" — for /dev, logs  */

    const void     *ops;        /* the ops table — set at bind time      */
    void           *drv_priv;   /* driver state — the DRIVER owns this   */
} uiox_dev_dev_t;

/* ═════════════════════════════════════════════════════════════════════
 * SECTION 5 — predicates
 *
 * These are the reason the family lives in the class value.  Each one is
 * a shift and a compare, so a probe loop or a filesystem open can decide
 * what to do without consulting a table.
 * ═════════════════════════════════════════════════════════════════════ */
#define UIOX_DEVCLASS_FAMILY(c)   (((uint32_t)(c) & UIOX_DEVFAM_MASK) >> UIOX_DEVFAM_SHIFT)
#define UIOX_DEVCLASS_INDEX(c)    ((uint32_t)(c) & UIOX_DEVINDEX_MASK)

/* Which classes take a 31_drvbuff pool.
 * STREAM and EVENT do; BLOCK uses 00_bcache; REG, CMD and BUS take none.
 * The nine that answer true are exactly the directories 31_drvbuff keeps
 * after the prune. */
#define UIOX_DEVCLASS_HAS_POOL(c)                       \
    (UIOX_DEVCLASS_FAMILY(c) == UIOX_DEVFAM_STREAM ||   \
     UIOX_DEVCLASS_FAMILY(c) == UIOX_DEVFAM_EVENT)

/* Addressed, therefore cached rather than pooled. */
#define UIOX_DEVCLASS_IS_BLOCK(c) \
    (UIOX_DEVCLASS_FAMILY(c) == UIOX_DEVFAM_BLOCK)

/* A tunnel or host controller — its output enumerates other devices. */
#define UIOX_DEVCLASS_IS_BUS(c) \
    (UIOX_DEVCLASS_FAMILY(c) == UIOX_DEVFAM_BUS)

/* ═════════════════════════════════════════════════════════════════════
 * SECTION 6 — the common head every family's ops begin with
 *
 * A class's ops struct STARTS with this, so a probe loop can call probe()
 * and init() without knowing the family — the same trick uiox_boot_hw_ops_t
 * and uiox_boot_media_ops_t already use at the bootloader layer:
 *
 *     typedef struct { uiox_dev_ops_t head; uiox_event_ops_t ev; } kbd_ops_t;
 * ═════════════════════════════════════════════════════════════════════ */
typedef struct {
    const char *name;
    int   (*probe) (const uiox_dev_dev_t *dev);   /* is it present?      */
    int   (*init)  (uiox_dev_dev_t *dev);         /* bring it up         */
    void  (*deinit)(uiox_dev_dev_t *dev);
} uiox_dev_ops_t;

/* ═════════════════════════════════════════════════════════════════════
 * SECTION 7 — family 3's vtable: EVENT
 *
 * Producer-only.  There is no transmit path because a keyboard does not
 * send a key — it reports one.  Where a stream device is symmetric, this
 * is not, and the asymmetry is deliberate: adding tx_submit here would
 * make it look like a NIC.
 *
 * Buffer: 31_drvbuff/02_Sensors/<dev>/, a record QUEUE.  Records are
 * fixed size, so the queue is a plain ring of N slots.
 *
 * From userspace: read() on a SCFS_S_IFCHR inode whose idev_class is
 * KBD / MOUSE / TOUCHPWD.
 * ═════════════════════════════════════════════════════════════════════ */

/** One input event.  Named for what the three event classes need and
 *  nothing more — not after any other system's equivalent. */
typedef struct {
    uint16_t code;      /* key / button / axis identifier              */
    uint16_t value;     /* press-release for a key; delta for an axis  */
    uint32_t time_ms;   /* when the device reported it                 */
} uiox_event_rec_t;

typedef struct {
    int  (*init)          (uiox_dev_dev_t *dev);
    void (*deinit)        (uiox_dev_dev_t *dev);

    /* Is a record waiting?  Non-blocking, safe from a bottom half.
     * This is the event-side counterpart of rx_poll's "0 = nothing". */
    bool (*event_pending) (uiox_dev_dev_t *dev);

    /* Pop one record.  The caller has already tested event_pending().
     * Returns bytes written (sizeof(uiox_event_rec_t)) or negative errno
     * — never 0: an empty queue is what event_pending() is for, and
     * conflating the two makes a spurious read indistinguishable from a
     * real one. */
    int  (*event_read)    (uiox_dev_dev_t *dev, void *rec, uint16_t maxlen);

    void (*isr)           (uiox_dev_dev_t *dev);
} uiox_event_ops_t;

/* ═════════════════════════════════════════════════════════════════════
 * SECTION 8 — family 4's vtable: REG
 *
 * A sensor or PMIC on an I2C/SPI bus.  There is no data path: the device
 * is asked for a value and answers, so there is no buffer and no pool.
 *
 * read_reg/write_reg rather than one transfer(): the common I2C idiom for
 * reading a register is a write of the register number followed by a
 * read, and a driver can compose that from these two.  A single
 * transfer() would force the bus layer to support an arbitrary message
 * list to serve one device.
 *
 * From userspace: ioctl() on a char inode — a lux value has no stream.
 * ═════════════════════════════════════════════════════════════════════ */
typedef struct {
    int (*init)       (uiox_dev_dev_t *dev);
    void (*deinit)    (uiox_dev_dev_t *dev);

    /* reg is the device's own register number; val is host-order and n
     * bytes wide.  Both return 0 or a negative errno. */
    int (*read_reg)   (uiox_dev_dev_t *dev, uint8_t reg, void *val, uint16_t n);
    int (*write_reg)  (uiox_dev_dev_t *dev, uint8_t reg, const void *val, uint16_t n);
} uiox_busdev_ops_t;

#ifdef __cplusplus
}
#endif
#endif /* UIOX_DEVCLASS_H */
