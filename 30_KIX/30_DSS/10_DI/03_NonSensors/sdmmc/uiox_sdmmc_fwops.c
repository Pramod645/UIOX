/**
 * @file    uiox_sdmmc_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_sdmmc_hw_ops_t.
 *
 * BOUNDARY
 *   Above: uiox_sdmmc_if.c calls through hw->ops and never sees a
 *          firmware symbol.
 *   Below: 02_FwHal builds to libuiox_fw_<arch>.a; this file calls its
 *          flat uiox_fw_sd_*() API and never includes its sources.
 *   This file is the joint.
 *
 * ── the read/write slots return -ENOSYS, and that is deliberate ──────
 * uiox_fw_sd.c's sdh_read()/sdh_write() are stubs: they bounds-check
 * against capacity_sectors and return UIOX_FW_OK, but the `(void)buf`
 * at the top of each is the whole body — not one byte is copied.  A
 * caller that received OK would believe its buffer held sector data.
 *
 * So this adapter does NOT forward those two.  It reports -ENOSYS —
 * "not implemented" — which is what uiox_fw_sd.c is until its
 * transfer path lands.  detect/card_init/reg_* are real and forward
 * normally: the clock, presence and geometry all work today.
 */
#include "uiox_sdmmc_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_sd.h"
#include "uiox_klibc.h"

#define UIOX_SDMMC_FWBIND_MAX   2u

typedef struct {
    uiox_sd_dev_t      fw;      /* the FwHal descriptor, by value */
    uiox_sdmmc_hw_t   *hw;
    bool               in_use;
} uiox_sdmmc_fwbind_t;

static uiox_sdmmc_fwbind_t s_sdmmc_bind[UIOX_SDMMC_FWBIND_MAX];

static uiox_sdmmc_fwbind_t *bind_of(uiox_sdmmc_hw_t *hw)
{
    if (!hw) return NULL;
    for (uint32_t i = 0u; i < UIOX_SDMMC_FWBIND_MAX; i++)
        if (s_sdmmc_bind[i].in_use && s_sdmmc_bind[i].hw == hw)
            return &s_sdmmc_bind[i];
    return NULL;
}

/* ── vtable slots ─────────────────────────────────────────────────── */

static int fw_sdmmc_init(uiox_sdmmc_hw_t *hw)
{
    uiox_sdmmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    /* sdh_init() enables the clock and returns OK; real. */
    return uiox_fw_sd_init(&b->fw, NULL) == UIOX_FW_OK ? 0 : -EIO;
}

static void fw_sdmmc_deinit(uiox_sdmmc_hw_t *hw)
{
    uiox_sdmmc_fwbind_t *b = bind_of(hw);
    if (b) uiox_fw_sd_deinit(&b->fw);
}

static int fw_sdmmc_power_on(uiox_sdmmc_hw_t *hw)
{
    return bind_of(hw) ? 0 : -ENODEV;
}

static void fw_sdmmc_power_off(uiox_sdmmc_hw_t *hw) { (void)hw; }

static uint32_t fw_sdmmc_reg_read(uiox_sdmmc_hw_t *hw, uint32_t offset)
{
    uiox_sdmmc_fwbind_t *b = bind_of(hw);
    if (!b || !b->fw.base) return 0u;
    return *((volatile uint32_t *)(b->fw.base + offset));
}

static void fw_sdmmc_reg_write(uiox_sdmmc_hw_t *hw, uint32_t offset, uint32_t val)
{
    uiox_sdmmc_fwbind_t *b = bind_of(hw);
    if (!b || !b->fw.base) return;
    *((volatile uint32_t *)(b->fw.base + offset)) = val;
}

/* uiox_fw_sd.c exposes no clock/bus-width setter, so these program the
 * SDHCI registers the same way sdh_init() does.  Real: CLK_CTRL at
 * 0x2C is the documented register. */
static int fw_sdmmc_set_clock(uiox_sdmmc_hw_t *hw, uint32_t hz)
{
    uiox_sdmmc_fwbind_t *b = bind_of(hw);
    if (!b || !b->fw.base) return -ENODEV;
    if (hz == 0u) return -EINVAL;
    /* Internal clock on, then SD clock on — the two-step sdh_init uses. */
    fw_sdmmc_reg_write(hw, 0x2Cu, 0x05u);
    fw_sdmmc_reg_write(hw, 0x2Cu, 0x07u);
    return 0;
}

static int fw_sdmmc_set_bus_width(uiox_sdmmc_hw_t *hw, uint8_t width)
{
    if (width != 1u && width != 4u) return -EINVAL;
    /* HOST_CTRL1 bit 1 = 4-bit.  Real register, standard bit. */
    uint32_t hc1 = fw_sdmmc_reg_read(hw, 0x28u);
    if (width == 4u) hc1 |= SDMMC_HC1_4BIT;
    else             hc1 &= ~SDMMC_HC1_4BIT;
    fw_sdmmc_reg_write(hw, 0x28u, hc1);
    return 0;
}

static int fw_sdmmc_set_speed_mode(uiox_sdmmc_hw_t *hw,
                                   uiox_sdmmc_hw_speed_t speed)
{
    if (!bind_of(hw)) return -ENODEV;
    /* HOST_CTRL2's UHS select.  Written, not verified: a controller that
     * rejects the mode reports it through the later CMD path rather than
     * here, which is the same bargain uiox_fw_sd.c makes. */
    uint32_t hc2 = fw_sdmmc_reg_read(hw, 0x3Eu);
    hc2 &= ~0x7u;
    switch (speed) {
    case UIOX_SDMMC_SPEED_SDR50:  hc2 |= SDMMC_HC2_SDR50;  break;
    case UIOX_SDMMC_SPEED_SDR104: hc2 |= SDMMC_HC2_SDR104; break;
    case UIOX_SDMMC_SPEED_DDR50:  hc2 |= SDMMC_HC2_DDR50;  break;
    default: break;                 /* DS / HS need no UHS select */
    }
    fw_sdmmc_reg_write(hw, 0x3Eu, hc2);
    return 0;
}

/* Real: sdh_detect() reads PRESENT_STATE and sets card_present /
 * write_protect on the FwHal descriptor.  Copy both into the HAL. */
static int fw_sdmmc_detect(uiox_sdmmc_hw_t *hw)
{
    uiox_sdmmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;

    bool present = uiox_fw_sd_card_present(&b->fw);
    hw->card_present  = present;
    hw->write_protect = b->fw.write_protect;
    hw->ident.card_present  = present;
    hw->ident.write_protect = b->fw.write_protect;
    return present ? 0 : -ENODEV;
}

/* Real: sdh_card_init() runs CMD0 → ACMD41 → CMD2 → CMD3 and fills rca,
 * card_type, capacity_sectors and capacity_bytes. */
static int fw_sdmmc_card_init(uiox_sdmmc_hw_t *hw)
{
    uiox_sdmmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;

    if (uiox_fw_sd_card_init(&b->fw) != UIOX_FW_OK) return -EIO;

    hw->ident.rca              = b->fw.rca;
    hw->ident.capacity_sectors = b->fw.capacity_sectors;
    hw->ident.capacity_bytes   = b->fw.capacity_bytes;
    switch (b->fw.card_type) {
    case UIOX_SD_SDSC: hw->ident.card = UIOX_SDMMC_CARD_SDSC; break;
    case UIOX_SD_SDHC: hw->ident.card = UIOX_SDMMC_CARD_SDHC; break;
    case UIOX_SD_SDXC: hw->ident.card = UIOX_SDMMC_CARD_SDXC; break;
    default:           hw->ident.card = UIOX_SDMMC_CARD_NONE; break;
    }
    return 0;
}

/* CMD framing: the ARG and CMD registers, the documented SDHCI pair. */
static int fw_sdmmc_send_cmd(uiox_sdmmc_hw_t *hw, uint8_t cmd,
                             uint32_t arg, uint32_t *resp)
{
    uiox_sdmmc_fwbind_t *b = bind_of(hw);
    if (!b || !b->fw.base) return -ENODEV;

    fw_sdmmc_reg_write(hw, 0x08u, arg);              /* ARG      */
    fw_sdmmc_reg_write(hw, 0x0Eu, (uint32_t)cmd);    /* CMD      */
    for (uint32_t i = 0u; i < 100000u; i++)
        if (fw_sdmmc_reg_read(hw, 0x30u) & SDMMC_INT_CMD_COMPLETE) break;
    if (resp) *resp = fw_sdmmc_reg_read(hw, 0x10u);  /* RESP0    */
    fw_sdmmc_reg_write(hw, 0x30u, SDMMC_INT_CMD_COMPLETE);
    return 0;
}

/* ── the two slots that report the gap ─────────────────────────────
 * uiox_fw_sd.c's sdh_read()/sdh_write() return UIOX_FW_OK without
 * copying a byte (`(void)buf` is the body).  Forwarding them would tell
 * bcache a sector arrived when the buffer still holds whatever was
 * there.  -ENOSYS is the honest answer until the transfer path lands. */
static int fw_sdmmc_read_blocks(uiox_sdmmc_hw_t *hw, uint32_t lba,
                                uint8_t *buf, uint32_t count)
{
    (void)lba; (void)buf; (void)count;
    if (!bind_of(hw)) return -ENODEV;
    return -ENOSYS;
}

static int fw_sdmmc_write_blocks(uiox_sdmmc_hw_t *hw, uint32_t lba,
                                 const uint8_t *buf, uint32_t count)
{
    (void)lba; (void)buf; (void)count;
    if (!bind_of(hw)) return -ENODEV;
    return -ENOSYS;
}

static void fw_sdmmc_isr(uiox_sdmmc_hw_t *hw)
{
    if (hw) {
        uint32_t st = fw_sdmmc_reg_read(hw, 0x30u);
        hw->pending_irq |= (st & SDMMC_INT_XFER_COMPLETE)
                         ? UIOX_SDMMC_IRQ_XFER_DONE
                         : UIOX_SDMMC_IRQ_ERROR;
    }
}

/* ── the adapter vtable ───────────────────────────────────────────── */
static const uiox_sdmmc_hw_ops_t s_sdmmc_fwops = {
    .init          = fw_sdmmc_init,
    .deinit        = fw_sdmmc_deinit,
    .power_on      = fw_sdmmc_power_on,
    .power_off     = fw_sdmmc_power_off,
    .reg_read      = fw_sdmmc_reg_read,
    .reg_write     = fw_sdmmc_reg_write,
    .set_clock     = fw_sdmmc_set_clock,
    .set_bus_width = fw_sdmmc_set_bus_width,
    .set_speed_mode= fw_sdmmc_set_speed_mode,
    .detect        = fw_sdmmc_detect,
    .card_init     = fw_sdmmc_card_init,
    .send_cmd      = fw_sdmmc_send_cmd,
    .read_blocks   = fw_sdmmc_read_blocks,
    .write_blocks  = fw_sdmmc_write_blocks,
    .isr           = fw_sdmmc_isr,
};

/* ── bind ─────────────────────────────────────────────────────────── */
int uiox_sdmmc_fwbind(uiox_sdmmc_hw_t *hw, struct uiox_i2c_dev *i2c)
{
    (void)i2c;   /* SD is MMIO; the parameter keeps fwbind's uniform
                  * shape and is unused — no bus is claimed. */
    if (!hw) return -EINVAL;

    uiox_sdmmc_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_SDMMC_FWBIND_MAX; i++)
        if (!s_sdmmc_bind[i].in_use) { b = &s_sdmmc_bind[i]; break; }
    if (!b) return -ENOSPC;

    memset(b, 0, sizeof(*b));
    b->hw     = hw;
    b->in_use = true;

    int rc = uiox_sdmmc_hw_init(hw, &s_sdmmc_fwops);
    if (rc < 0) { b->in_use = false; return rc; }

    /* Hand the host address over so every slot has somewhere to write. */
    if (hw->base)
        (void)uiox_fw_sd_init_sdhost(&b->fw, hw->base, hw->irq);

    return 0;
}
