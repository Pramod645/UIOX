/**
 * @file    uiox_nvme_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_nvme_hw_ops_t.
 *
 * BOUNDARY
 *   Above: uiox_nvme_if.c calls through hw->ops and never sees a firmware
 *          symbol.
 *   Below: 02_FwHal builds to libuiox_fw_<arch>.a; this file calls its
 *          flat uiox_fw_nvme_*() API and never includes its sources.
 *   This file is the joint.
 *
 * BOUND FOR REAL, unlike uiox_emmc_fwops.c's placeholders:
 * uiox_fw_nvme.c carries a working sequence (CC ← 0, wait !CSTS.RDY, AQA
 * 0x001F001F, CC.EN, wait RDY / abort on CFS).  Every slot below calls it
 * and maps uiox_fw_err_t down to errno — the one place FwHal's error
 * space crosses into the kernel's.
 */
#include "uiox_nvme_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_nvme.h"
#include "uiox_klibc.h"

#define UIOX_NVME_FWBIND_MAX   2u

typedef struct {
    uiox_nvme_dev_t    fw;      /* the FwHal descriptor, by value */
    uiox_nvme_hw_t    *hw;
    bool               in_use;
} uiox_nvme_fwbind_t;

static uiox_nvme_fwbind_t s_nvme_bind[UIOX_NVME_FWBIND_MAX];

static uiox_nvme_fwbind_t *bind_of(uiox_nvme_hw_t *hw)
{
    if (!hw) return NULL;
    for (uint32_t i = 0u; i < UIOX_NVME_FWBIND_MAX; i++)
        if (s_nvme_bind[i].in_use && s_nvme_bind[i].hw == hw)
            return &s_nvme_bind[i];
    return NULL;
}

/* ── vtable slots ─────────────────────────────────────────────────── */

static int fw_nvme_init(uiox_nvme_hw_t *hw)
{
    uiox_nvme_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    /* The BAR was handed over before this call (uiox_fw_nvme_init_bar0),
     * so init() is the controller sequence, not the address setup. */
    return uiox_fw_nvme_init(&b->fw, NULL) == UIOX_FW_OK ? 0 : -EIO;
}

static void fw_nvme_deinit(uiox_nvme_hw_t *hw)
{
    uiox_nvme_fwbind_t *b = bind_of(hw);
    if (b) uiox_fw_nvme_deinit(&b->fw);
}

static int fw_nvme_power_on(uiox_nvme_hw_t *hw)
{
    uiox_nvme_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    /* PCIe assigned and enabled the function already; the device-side
     * "power on" is the controller enable, which has its own slot. */
    return 0;
}

static void fw_nvme_power_off(uiox_nvme_hw_t *hw) { (void)hw; }

static uint32_t fw_nvme_reg_read(uiox_nvme_hw_t *hw, uint32_t offset)
{
    uiox_nvme_fwbind_t *b = bind_of(hw);
    if (!b || !b->fw.bar0) return 0u;
    return *((volatile uint32_t *)(b->fw.bar0 + offset));
}

static void fw_nvme_reg_write(uiox_nvme_hw_t *hw, uint32_t offset, uint32_t val)
{
    uiox_nvme_fwbind_t *b = bind_of(hw);
    if (!b || !b->fw.bar0) return;
    *((volatile uint32_t *)(b->fw.bar0 + offset)) = val;
}

static int fw_nvme_enable(uiox_nvme_hw_t *hw, bool on)
{
    uiox_nvme_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;

    if (!on) {
        fw_nvme_reg_write(hw, NVME_REG_CC, 0u);
        return 0;
    }

    /* Disable, wait for !RDY, size the admin queues, enable, wait RDY or
     * abort on CFS — the sequence uiox_fw_nvme.c implements for init. */
    fw_nvme_reg_write(hw, NVME_REG_CC, 0u);
    for (uint32_t i = 0u; i < 100000u; i++)
        if (!(fw_nvme_reg_read(hw, NVME_REG_CSTS) & NVME_CSTS_RDY)) break;

    fw_nvme_reg_write(hw, NVME_REG_AQA, 0x001F001Fu);
    fw_nvme_reg_write(hw, NVME_REG_CC,
                      NVME_CC_EN | (6u << 16u) | (4u << 20u));

    for (uint32_t i = 0u; i < 1000000u; i++) {
        uint32_t csts = fw_nvme_reg_read(hw, NVME_REG_CSTS);
        if (csts & NVME_CSTS_CFS) return -EIO;
        if (csts & NVME_CSTS_RDY) break;
    }
    return 0;
}

static int fw_nvme_submit(uiox_nvme_hw_t *hw, uint8_t opcode, uint64_t lba,
                          uint8_t *buf, uint32_t sectors)
{
    (void)opcode; (void)lba; (void)buf; (void)sectors;
    if (!bind_of(hw)) return -ENODEV;
    /* Queue submission is not wired: uiox_fw_nvme.c exposes read/write/
     * flush directly, so those slots call it and this one reports the
     * gap rather than pretending a doorbell rang. */
    return -ENOSYS;
}

static int fw_nvme_read_blocks(uiox_nvme_hw_t *hw, uint64_t lba,
                               uint8_t *buf, uint32_t count)
{
    uiox_nvme_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return uiox_fw_nvme_read(&b->fw, lba, buf, count) == UIOX_FW_OK ? 0 : -EIO;
}

static int fw_nvme_write_blocks(uiox_nvme_hw_t *hw, uint64_t lba,
                                const uint8_t *buf, uint32_t count)
{
    uiox_nvme_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return uiox_fw_nvme_write(&b->fw, lba, buf, count) == UIOX_FW_OK ? 0 : -EIO;
}

static int fw_nvme_identify(uiox_nvme_hw_t *hw)
{
    uiox_nvme_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    if (uiox_fw_nvme_init(&b->fw, NULL) != UIOX_FW_OK) return -EIO;

    /* uiox_fw_nvme_init() fills the descriptor's identity fields.  Copy
     * them across into the HAL's ident block — the two structs are
     * deliberately separate; this is the translation. */
    hw->nsid            = b->fw.nsid;
    hw->ident.nsid      = b->fw.nsid;
    hw->ident.capacity_lba   = b->fw.capacity_lba;
    hw->ident.capacity_bytes = b->fw.capacity_bytes;
    hw->ident.volatile_wc    = b->fw.volatile_wc;
    hw->ident.trim_supported = b->fw.trim_supported;

    for (uint32_t i = 0u; i < (UIOX_NVME_MODEL_LEN - 1u) && b->fw.model[i]; i++)
        hw->ident.model[i] = b->fw.model[i];
    for (uint32_t i = 0u; i < (UIOX_NVME_SERIAL_LEN - 1u) && b->fw.serial[i]; i++)
        hw->ident.serial[i] = b->fw.serial[i];
    for (uint32_t i = 0u; i < (UIOX_NVME_FWREV_LEN - 1u) && b->fw.fw_rev[i]; i++)
        hw->ident.fw_rev[i] = b->fw.fw_rev[i];
    return 0;
}

static int fw_nvme_flush(uiox_nvme_hw_t *hw)
{
    uiox_nvme_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    if (!b->fw.volatile_wc) return 0;   /* nothing to flush */
    return uiox_fw_nvme_flush(&b->fw) == UIOX_FW_OK ? 0 : -EIO;
}

static void fw_nvme_isr(uiox_nvme_hw_t *hw)
{
    if (hw) hw->pending_irq |= UIOX_NVME_IRQ_IO_DONE;
}

/* ── the adapter vtable ───────────────────────────────────────────── */
static const uiox_nvme_hw_ops_t s_nvme_fwops = {
    .init         = fw_nvme_init,
    .deinit       = fw_nvme_deinit,
    .power_on     = fw_nvme_power_on,
    .power_off    = fw_nvme_power_off,
    .reg_read     = fw_nvme_reg_read,
    .reg_write    = fw_nvme_reg_write,
    .enable       = fw_nvme_enable,
    .submit       = fw_nvme_submit,
    .read_blocks  = fw_nvme_read_blocks,
    .write_blocks = fw_nvme_write_blocks,
    .identify     = fw_nvme_identify,
    .flush        = fw_nvme_flush,
    .isr          = fw_nvme_isr,
};

/* ── bind ─────────────────────────────────────────────────────────── */
int uiox_nvme_fwbind(uiox_nvme_hw_t *hw, struct uiox_i2c_dev *i2c)
{
    (void)i2c;   /* NVMe is PCIe; the parameter keeps the tree's uniform
                  * fwbind shape and is unused — no bus is claimed. */
    if (!hw) return -EINVAL;

    uiox_nvme_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_NVME_FWBIND_MAX; i++)
        if (!s_nvme_bind[i].in_use) { b = &s_nvme_bind[i]; break; }
    if (!b) return -ENOSPC;

    memset(b, 0, sizeof(*b));
    b->hw     = hw;
    b->in_use = true;

    int rc = uiox_nvme_hw_init(hw, &s_nvme_fwops);
    if (rc < 0) { b->in_use = false; return rc; }

    /* Hand the BAR over so every slot above has an address. */
    if (hw->bar0)
        (void)uiox_fw_nvme_init_bar0(&b->fw, hw->bar0, hw->irq);

    return 0;
}
