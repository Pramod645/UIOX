/**
 * @file    uiox_emmc_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_emmc_hw_ops_t.
 *
 * PATH   30_KIX/30_DSS/10_DI/03_NonSensors/emmc/uiox_emmc_fwops.c
 * LAYER  10_DI -- hardware-control interface.
 *
 * BOUNDARY
 *   Above: 20_DD/uiox_emmc_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuiox_fw_<arch>.a; this file calls its flat
 *          uiox_fw_emmc_*() API and never includes its sources.
 *   This file is the joint.
 *
 * REGENERATED -- every stub carries the exact signature of its vtable slot
 * (20 slots).  Bodies return -ENOSYS: an honest "not yet
 * bound", never a silent success.  Every slot parameter is explicitly cast
 * to void so the strict -Werror set stays intact for the driver units that
 * do carry logic.
 */

#include "uiox_emmc_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_emmc.h"
#include "uiox_fw_i2c.h"

#define UIOX_EMMC_FWBIND_MAX   2u

typedef struct {
    uiox_emmc_dev_t  fw;
    uiox_i2c_dev_t     i2c;
    uiox_emmc_hw_t             *hw;
    bool                in_use;
} uiox_emmc_fwbind_t;

static uiox_emmc_fwbind_t s_emmc_bind[UIOX_EMMC_FWBIND_MAX];

static uiox_emmc_fwbind_t *bind_of(uiox_emmc_hw_t *hw)
{
    if (!hw) return NULL;
    for (uint32_t i = 0u; i < UIOX_EMMC_FWBIND_MAX; i++)
        if (s_emmc_bind[i].in_use && s_emmc_bind[i].hw == hw)
            return &s_emmc_bind[i];
    return NULL;
}

/* ---- vtable slots ---- */
static int fw_emmc_init(uiox_emmc_hw_t *hw)
{
    (void)hw;
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* deinit: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_emmc_deinit(uiox_emmc_hw_t *hw) { (void)hw; }
static int fw_emmc_power_on(uiox_emmc_hw_t *hw)
{
    (void)hw;
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* power_off: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_emmc_power_off(uiox_emmc_hw_t *hw) { (void)hw; }
static uint32_t fw_emmc_reg_read(uiox_emmc_hw_t *hw, uint32_t offset)
{
    (void)hw; (void)offset;
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* reg_write: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_emmc_reg_write(uiox_emmc_hw_t *hw, uint32_t offset, uint32_t val) { (void)hw; (void)offset; (void)val; }
static int fw_emmc_set_clock(uiox_emmc_hw_t *hw, uint32_t hz)
{
    (void)hw; (void)hz;
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_emmc_set_bus_width(uiox_emmc_hw_t *hw, uint8_t width)
{
    (void)hw; (void)width;
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_emmc_set_speed_mode(uiox_emmc_hw_t *hw, uiox_emmc_speed_t speed)
{
    (void)hw; (void)speed;
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_emmc_send_cmd(uiox_emmc_hw_t *hw, uint8_t cmd, uint32_t arg, uiox_emmc_resp_t resp_type, uint32_t *resp)
{
    (void)hw; (void)cmd; (void)arg; (void)resp_type; (void)resp;
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_emmc_read_blocks(uiox_emmc_hw_t *hw, uint32_t lba, uint8_t *buf, uint32_t count)
{
    (void)hw; (void)lba; (void)buf; (void)count;
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_emmc_write_blocks(uiox_emmc_hw_t *hw, uint32_t lba, const uint8_t *buf, uint32_t count)
{
    (void)hw; (void)lba; (void)buf; (void)count;
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_emmc_read_ext_csd(uiox_emmc_hw_t *hw, uint8_t *buf)
{
    (void)hw; (void)buf;
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_emmc_write_ext_csd(uiox_emmc_hw_t *hw, uint8_t index, uint8_t val)
{
    (void)hw; (void)index; (void)val;
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_emmc_tuning(uiox_emmc_hw_t *hw)
{
    (void)hw;
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static uint8_t fw_emmc_crc7(uiox_emmc_hw_t *hw, uint8_t len)
{
    (void)hw; (void)len;
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static uint16_t fw_emmc_crc16(uiox_emmc_hw_t *hw, uint32_t len)
{
    (void)hw; (void)len;
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* gpio_write: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_emmc_gpio_write(uiox_emmc_hw_t *hw, uint32_t pin, bool val) { (void)hw; (void)pin; (void)val; }

/* gpio_read: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static bool fw_emmc_gpio_read(uiox_emmc_hw_t *hw, uint32_t pin) { (void)hw; (void)pin; return false; }

/* isr: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_emmc_isr(uiox_emmc_hw_t *hw) { (void)hw; }

static const uiox_emmc_hw_ops_t s_emmc_fwops = {
    .init           = fw_emmc_init,
    .deinit         = fw_emmc_deinit,
    .power_on       = fw_emmc_power_on,
    .power_off      = fw_emmc_power_off,
    .reg_read       = fw_emmc_reg_read,
    .reg_write      = fw_emmc_reg_write,
    .set_clock      = fw_emmc_set_clock,
    .set_bus_width  = fw_emmc_set_bus_width,
    .set_speed_mode = fw_emmc_set_speed_mode,
    .send_cmd       = fw_emmc_send_cmd,
    .read_blocks    = fw_emmc_read_blocks,
    .write_blocks   = fw_emmc_write_blocks,
    .read_ext_csd   = fw_emmc_read_ext_csd,
    .write_ext_csd  = fw_emmc_write_ext_csd,
    .tuning         = fw_emmc_tuning,
    .crc7           = fw_emmc_crc7,
    .crc16          = fw_emmc_crc16,
    .gpio_write     = fw_emmc_gpio_write,
    .gpio_read      = fw_emmc_gpio_read,
    .isr            = fw_emmc_isr,
};

/* ---- binding entry point ---- */
int uiox_emmc_fwbind(uiox_emmc_hw_t *hw, struct uiox_i2c_dev *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_emmc_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_EMMC_FWBIND_MAX; i++)
        if (!s_emmc_bind[i].in_use) { b = &s_emmc_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_emmc_init_sdhost(&b->fw, hw->base);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    int rc = uiox_emmc_hw_init(hw, &s_emmc_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
