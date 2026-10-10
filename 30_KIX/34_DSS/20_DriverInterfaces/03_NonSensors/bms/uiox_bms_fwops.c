/**
 * @file    uiox_bms_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_bms_hw_ops_t.
 * @date    2026-10-10
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/03_NonSensors/bms/uiox_bms_fwops.c
 * LAYER  20_DriverInterfaces — hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_bms_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuioxfw<arch>.a; this file calls its flat
 *          uiox_fw_bms_*() API and never includes its sources.
 *   This file is the joint: it includes the FwHal public header for the
 *   declarations, and the archive supplies the definitions at link time.
 *
 * GENERATED — every slot in uiox_bms_hw_ops_t has an entry below.
 * Slots marked ENOSYS are the interrupt/GPIO entry points: the firmware
 * layer owns the IRQ controller but does not route device interrupts into
 * class handlers (that is 10_BSP/10_Arch/*/arch_runtime.c).  Reporting
 * unsupported is deliberate — silently accepting would swallow an IRQ
 * nothing services.
 */

#include "uiox_bms_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_bms.h"
#include "uiox_fw_i2c.h"

#define UIOX_BMS_FWBIND_MAX   2u

typedef struct {
    uiox_fw_bms_dev_t  fw;
    uiox_i2c_dev_t                     i2c;
    uiox_bms_hw_t                  *hw;
    bool                               in_use;
} uiox_bms_fwbind_t;

static uiox_bms_fwbind_t s_bms_bind[UIOX_BMS_FWBIND_MAX];

static uiox_bms_fwbind_t *bind_of(uiox_bms_hw_t *hw)
{
    if (!hw || !hw->base) return NULL;
    uiox_bms_fwbind_t *b = (uiox_bms_fwbind_t *)hw->base;
    return (b->in_use && b->hw == hw) ? b : NULL;
}

/* ---- vtable slots ---- */

static int fw_bms_init(uiox_bms_hw_t *hw)
{
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_bms_deinit(uiox_bms_hw_t *hw)
{
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_bms_reg_read(uiox_bms_hw_t *hw, uint8_t reg, uint16_t *val)
{
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    if (!val) return -EINVAL;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_bms_reg_write(uiox_bms_hw_t *hw, uint8_t reg, uint16_t val)
{
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_bms_reg_read16(uiox_bms_hw_t *hw, uint8_t reg, uint16_t *val)
{
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    if (!val) return -EINVAL;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_bms_reg_write16(uiox_bms_hw_t *hw, uint8_t reg, uint16_t val)
{
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_bms_bulk_read(uiox_bms_hw_t *hw)
{
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_bms_measure_cells(uiox_bms_hw_t *hw)
{
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_bms_measure_current(uiox_bms_hw_t *hw)
{
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_bms_measure_temp(uiox_bms_hw_t *hw)
{
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_bms_read_coulombs(uiox_bms_hw_t *hw)
{
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_bms_set_chg_fet(uiox_bms_hw_t *hw)
{
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_bms_set_dsg_fet(uiox_bms_hw_t *hw)
{
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_bms_set_balance(uiox_bms_hw_t *hw)
{
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_bms_get_balance(uiox_bms_hw_t *hw)
{
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_bms_fault_status(uiox_bms_hw_t *hw)
{
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_bms_fault_clear(uiox_bms_hw_t *hw)
{
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_bms_pack_present(uiox_bms_hw_t *hw)
{
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_bms_ship_mode(uiox_bms_hw_t *hw)
{
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_bms_isr(uiox_bms_hw_t *hw)
{ (void)hw; }

static const uiox_bms_hw_ops_t s_bms_fwops = {
    .init          = fw_bms_init,
    .deinit        = fw_bms_deinit,
    .reg_read      = fw_bms_reg_read,
    .reg_write     = fw_bms_reg_write,
    .reg_read16    = fw_bms_reg_read16,
    .reg_write16   = fw_bms_reg_write16,
    .bulk_read     = fw_bms_bulk_read,
    .measure_cells = fw_bms_measure_cells,
    .measure_current= fw_bms_measure_current,
    .measure_temp  = fw_bms_measure_temp,
    .read_coulombs = fw_bms_read_coulombs,
    .set_chg_fet   = fw_bms_set_chg_fet,
    .set_dsg_fet   = fw_bms_set_dsg_fet,
    .set_balance   = fw_bms_set_balance,
    .get_balance   = fw_bms_get_balance,
    .fault_status  = fw_bms_fault_status,
    .fault_clear   = fw_bms_fault_clear,
    .pack_present  = fw_bms_pack_present,
    .ship_mode     = fw_bms_ship_mode,
    .isr           = fw_bms_isr,
};

/* ---- binding entry points (one per 02_FwHal chip helper) ---- */
int uiox_bms_fwbind   (uiox_bms_hw_t *hw, uiox_i2c_dev_t *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_bms_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_BMS_FWBIND_MAX; i++)
        if (!s_bms_bind[i].in_use) { b = &s_bms_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_bms_init(&b->fw, i2c);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    hw->base = (uintptr_t)b;
    int rc = uiox_bms_hw_init(hw, &s_bms_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
