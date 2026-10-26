/**
 * @file    uiox_sata_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-10 uiox_sata_hw_ops_t.
 *
 * BOUNDARY
 *   Above: uiox_sata_if.c calls through hw->priv and never sees a
 *          firmware symbol.
 *   Below: 02_FwHal builds to libuiox_fw_<arch>.a; this file calls its
 *          flat uiox_fw_sata_*() API and never includes its sources.
 *   This file is the joint.
 *
 * ── the read/write slots return -ENOSYS, and that is deliberate ──────
 * uiox_fw_sata.c:ahci_read()/ahci_write() are stubs.  Each bounds-checks
 * `lba + sectors > capacity_sectors` and then returns UIOX_FW_OK, with
 * `(void)buf;` as the whole body — not one byte is copied.  A caller
 * that received OK would believe its buffer held sector data.
 *
 * So this adapter does NOT forward those two.  It reports -ENOSYS —
 * "not implemented" — which is what uiox_fw_sata.c is until its transfer
 * path lands.  This mirrors uiox_sdmmc_fwops.c, which refuses to forward
 * sdh_read/sdh_write for exactly the same reason.
 *
 * ── and identify() is not forwarded either ──────────────────────────
 * ahci_identify() invents the geometry: it writes "UIOX SATA SSD" as the
 * model, sets capacity_sectors = 0x800000 (exactly 4 GB, SDHC's default),
 * and claims ncq_supported = true, smart_supported = true — none of it
 * read from the drive.  The fabricated capacity also defeats the bounds
 * check in the two stubs above, since a ceiling larger than the media
 * accepts every LBA.
 *
 * The port is still brought up for real: ahci_init() enables GHC.AE,
 * starts the command engine, checks PxSSTS.DET == 3 and validates the ATA
 * signature in PxSIG.  That part is honest, and it is what makes -ENODEV
 * for an empty port correct.
 */
#include "uiox_sata_hw.h"
#include "uiox_fw_types.h"
#include "uiox_fw_sata.h"
#include "uiox_klibc.h"

#define UIOX_SATA_FWBIND_MAX   2u

typedef struct {
    uiox_sata_dev_t    fw;      /* the FwHal descriptor, by value */
    uiox_sata_hw_t    *hw;
    bool               in_use;
} uiox_sata_fwbind_t;

static uiox_sata_fwbind_t s_sata_bind[UIOX_SATA_FWBIND_MAX];

/* Always the FIRST free slot, not a search by hw: a fresh bind has no
 * hw pointer to search with until this function writes one. */
static uiox_sata_fwbind_t *slot_alloc(void)
{
    for (uint32_t i = 0u; i < UIOX_SATA_FWBIND_MAX; i++)
        if (!s_sata_bind[i].in_use) return &s_sata_bind[i];
    return NULL;
}

static uiox_sata_fwbind_t *bind_of(uiox_sata_hw_t *hw)
{
    if (!hw) return NULL;
    for (uint32_t i = 0u; i < UIOX_SATA_FWBIND_MAX; i++)
        if (s_sata_bind[i].in_use && s_sata_bind[i].hw == hw)
            return &s_sata_bind[i];
    return NULL;
}

/* ── vtable slots ─────────────────────────────────────────────────── */

static int fw_sata_init(uiox_sata_hw_t *hw)
{
    uiox_sata_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    /* ahci_init() is real: GHC.AE, port start, DET check, signature. */
    return uiox_fw_sata_init(&b->fw, NULL) == UIOX_FW_OK ? 0 : -EIO;
}

static void fw_sata_deinit(uiox_sata_hw_t *hw)
{
    uiox_sata_fwbind_t *b = bind_of(hw);
    if (b) uiox_fw_sata_deinit(&b->fw);
}

static int fw_sata_port_start(uiox_sata_hw_t *hw, uint8_t port)
{
    (void)port;                     /* the FwHal descriptor holds its own */
    return fw_sata_init(hw);
}

static void fw_sata_port_stop(uiox_sata_hw_t *hw, uint8_t port)
{
    (void)port;
    fw_sata_deinit(hw);
}

/* Register access: the FwHal header exposes the offsets but no accessor
 * functions, so these go through hw->priv's mmio helpers when 02_FwHal
 * provides them, and otherwise report nothing rather than reading a
 * fabricated address.  0 for a read and a no-op for a write is the same
 * contract uiox_sata_hw_ghc_read() already has when priv is unset. */
static uint32_t fw_sata_ghc_read(uiox_sata_hw_t *hw, uint32_t off)
{
    uiox_sata_fwbind_t *b = bind_of(hw);
    if (!b || !b->fw.bar5) return 0u;
    return *(volatile uint32_t *)(b->fw.bar5 + off);
}

static void fw_sata_ghc_write(uiox_sata_hw_t *hw, uint32_t off, uint32_t val)
{
    uiox_sata_fwbind_t *b = bind_of(hw);
    if (!b || !b->fw.bar5) return;
    *(volatile uint32_t *)(b->fw.bar5 + off) = val;
}

static uint32_t fw_sata_px_read(uiox_sata_hw_t *hw, uint8_t port, uint32_t off)
{
    uiox_sata_fwbind_t *b = bind_of(hw);
    if (!b || !b->fw.bar5) return 0u;
    /* AHCI_PORT_BASE comes from the FwHal header — the port stride is
     * the controller's, not this file's to restate. */
    return *(volatile uint32_t *)(b->fw.bar5 + AHCI_PORT_BASE(port) + off);
}

static void fw_sata_px_write(uiox_sata_hw_t *hw, uint8_t port,
                             uint32_t off, uint32_t val)
{
    uiox_sata_fwbind_t *b = bind_of(hw);
    if (!b || !b->fw.bar5) return;
    *(volatile uint32_t *)(b->fw.bar5 + AHCI_PORT_BASE(port) + off) = val;
}

/* FIS framing needs a command list and PRDT in DMA memory that
 * 02_FwHal does not allocate.  -ENOSYS until that exists. */
static int fw_sata_cmd_issue(uiox_sata_hw_t *hw, uint8_t port, uint8_t slot,
                             const uiox_sata_fis_h2d_t *fis, bool write,
                             uintptr_t data_phys, uint32_t len)
{
    (void)hw; (void)port; (void)slot; (void)fis;
    (void)write; (void)data_phys; (void)len;
    return -ENOSYS;
}

/* NOT forwarded — see the file banner.  ahci_read() discards buf. */
static int fw_sata_read_sectors(uiox_sata_hw_t *hw, uint64_t lba,
                                uint8_t *buf, uint32_t count)
{
    uiox_sata_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    if (!buf || !count) return -EINVAL;
    (void)lba;
    return -ENOSYS;
}

/* NOT forwarded — same reason. */
static int fw_sata_write_sectors(uiox_sata_hw_t *hw, uint64_t lba,
                                 const uint8_t *buf, uint32_t count)
{
    uiox_sata_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    if (!buf || !count) return -EINVAL;
    (void)lba;
    return -ENOSYS;
}
/* ── NCQ — no tag queue in 02_FwHal ────────────────────────────────
 * Both delegate to the non-NCQ slots, which return -ENOSYS, so an NCQ
 * request fails identifiably rather than silently falling back to a
 * path that reports success without transferring. */
static int fw_sata_ncq_read(uiox_sata_hw_t *hw, uint64_t lba, uint8_t *buf,
    uint32_t count, uint8_t tag)
{
(void)tag;
return fw_sata_read_sectors(hw, lba, buf, count);
}

static int fw_sata_ncq_write(uiox_sata_hw_t *hw, uint64_t lba,
     const uint8_t *buf, uint32_t count, uint8_t tag)
{
(void)tag;
return fw_sata_write_sectors(hw, lba, buf, count);
}

/* ── GPIO (DEVSLP / SGPIO status LEDs) ─────────────────────────────
* None wired on this SoC; a no-op is correct, since there is no pin
* whose state could be misreported. */
static void fw_sata_gpio_write(uiox_sata_hw_t *hw, uint32_t pin, bool val)
{ (void)hw; (void)pin; (void)val; }

/* Driver-level ISR — the 20_DD layer handles IRQs through
* uiox_sata_if_irq_handle().  Nothing to do at this boundary. */
static void fw_sata_isr(uiox_sata_hw_t *hw)
{ (void)hw; }
/* No COMRESET sequence exists in 02_FwHal — ahci_init() starts the
 * engine but never pulses PxSCTL.  Reporting success here would claim a
 * link renegotiation that did not happen. */
static int fw_sata_port_reset(uiox_sata_hw_t *hw, uint8_t port)
{
    (void)port;
    if (!bind_of(hw)) return -ENODEV;
    return -ENOSYS;
}

/* ahci_smart() is `{ (void)d; (void)buf; return UIOX_FW_OK; }` — it
 * fills nothing, so forwarding would hand the caller an uninitialised
 * buffer as SMART data. */
static int fw_sata_smart_read(uiox_sata_hw_t *hw, uint8_t *buf)
{
    uiox_sata_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    if (!buf) return -EINVAL;
    return -ENOSYS;
}

static const uiox_sata_hw_ops_t s_sata_fw_ops = {
    .init          = fw_sata_init,
    .deinit        = fw_sata_deinit,
    .port_start    = fw_sata_port_start,
    .port_stop     = fw_sata_port_stop,
    .ghc_read      = fw_sata_ghc_read,
    .ghc_write     = fw_sata_ghc_write,
    .px_read       = fw_sata_px_read,
    .px_write      = fw_sata_px_write,
    .cmd_issue     = fw_sata_cmd_issue,
    .read_sectors  = fw_sata_read_sectors,
    .write_sectors = fw_sata_write_sectors,
    .ncq_read      = fw_sata_ncq_read,
    .ncq_write     = fw_sata_ncq_write,
    .port_reset    = fw_sata_port_reset,
    .gpio_write    = fw_sata_gpio_write,
    .smart_read    = fw_sata_smart_read,
    .isr           = fw_sata_isr,
};
/* ── the bind the probe calls ─────────────────────────────────────── */

int uiox_sata_fwbind(uiox_sata_hw_t *hw, void *i2c_unused)
{
    (void)i2c_unused;
    if (!hw) return -EINVAL;

    uiox_sata_fwbind_t *b = bind_of(hw);
    if (b) return 0;                     /* already bound — idempotent */

    b = slot_alloc();
    if (!b) return -ENOSPC;              /* table full */

    uiox_memset(&b->fw, 0, sizeof(b->fw));
    b->fw.bar5   = hw->bar5;
    b->fw.irq    = hw->irq;
    b->fw.port   = hw->port;
    b->hw        = hw;
    b->in_use    = true;

    hw->priv = (void *)&s_sata_fw_ops;
    return 0;
}
