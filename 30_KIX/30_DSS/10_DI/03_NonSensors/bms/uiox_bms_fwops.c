/**
 * @file    uiox_bms_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_bms_hw_ops_t.
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/03_NonSensors/bms/uiox_bms_fwops.c
 * LAYER  20_DriverInterfaces -- hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_bms_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuiox_fw_<arch>.a; this file calls its flat
 *          uiox_fw_bms_*() API and never includes its sources.
 *   This file is the joint.
 *
 * REGENERATED -- every stub carries the exact signature of its vtable slot
 * (20 slots).  Bodies return -ENOSYS: an honest "not yet
 * bound", never a silent success.  Every slot parameter is explicitly cast
 * to void so the strict -Werror set stays intact for the driver units that
 * do carry logic.
 */

#include "uiox_bms_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_bms.h"
#include "uiox_fw_i2c.h"

#define UIOX_BMS_FWBIND_MAX   2u

typedef struct {
    uiox_bms_dev_t  fw;
    uiox_i2c_dev_t     i2c;
    uiox_bms_hw_t             *hw;
    bool                in_use;
} uiox_bms_fwbind_t;

static uiox_bms_fwbind_t s_bms_bind[UIOX_BMS_FWBIND_MAX];

static uiox_bms_fwbind_t *bind_of(uiox_bms_hw_t *hw)
{
    if (!hw) return NULL;
    for (uint32_t i = 0u; i < UIOX_BMS_FWBIND_MAX; i++)
        if (s_bms_bind[i].in_use && s_bms_bind[i].hw == hw)
            return &s_bms_bind[i];
    return NULL;
}

/* ---- vtable slots ---- */
static int fw_bms_init(uiox_bms_hw_t *hw)
{
    (void)hw;
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* deinit: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_bms_deinit(uiox_bms_hw_t *hw) { (void)hw; }
static int fw_bms_reg_read(uiox_bms_hw_t *hw, uint8_t reg, uint8_t *val)
{
    (void)hw; (void)reg; (void)val;
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_bms_reg_write(uiox_bms_hw_t *hw, uint8_t reg, uint8_t val)
{
    (void)hw; (void)reg; (void)val;
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_bms_reg_read16(uiox_bms_hw_t *hw, uint8_t reg, uint16_t *val)
{
    (void)hw; (void)reg; (void)val;
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_bms_reg_write16(uiox_bms_hw_t *hw, uint8_t reg, uint16_t val)
{
    (void)hw; (void)reg; (void)val;
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_bms_bulk_read(uiox_bms_hw_t *hw, uint8_t reg, uint8_t *buf, uint8_t len)
{
    (void)hw; (void)reg; (void)buf; (void)len;
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_bms_measure_cells(uiox_bms_hw_t *hw)
{
    (void)hw;
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_bms_measure_current(uiox_bms_hw_t *hw)
{
    (void)hw;
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_bms_measure_temp(uiox_bms_hw_t *hw)
{
    (void)hw;
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_bms_read_coulombs(uiox_bms_hw_t *hw, int32_t *mah_out)
{
    (void)hw; (void)mah_out;
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_bms_set_chg_fet(uiox_bms_hw_t *hw, bool on)
{
    (void)hw; (void)on;
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_bms_set_dsg_fet(uiox_bms_hw_t *hw, bool on)
{
    (void)hw; (void)on;
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_bms_set_balance(uiox_bms_hw_t *hw, uint16_t cell_mask)
{
    (void)hw; (void)cell_mask;
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_bms_get_balance(uiox_bms_hw_t *hw, uint16_t *cell_mask)
{
    (void)hw; (void)cell_mask;
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_bms_fault_status(uiox_bms_hw_t *hw, uint32_t *flags_out)
{
    (void)hw; (void)flags_out;
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_bms_fault_clear(uiox_bms_hw_t *hw, uint32_t flags)
{
    (void)hw; (void)flags;
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static bool fw_bms_pack_present(uiox_bms_hw_t *hw)
{
    (void)hw;
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_bms_ship_mode(uiox_bms_hw_t *hw)
{
    (void)hw;
    uiox_bms_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* isr: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_bms_isr(uiox_bms_hw_t *hw) { (void)hw; }

static const uiox_bms_hw_ops_t s_bms_fwops = {
    .init           = fw_bms_init,
    .deinit         = fw_bms_deinit,
    .reg_read       = fw_bms_reg_read,
    .reg_write      = fw_bms_reg_write,
    .reg_read16     = fw_bms_reg_read16,
    .reg_write16    = fw_bms_reg_write16,
    .bulk_read      = fw_bms_bulk_read,
    .measure_cells  = fw_bms_measure_cells,
    .measure_current= fw_bms_measure_current,
    .measure_temp   = fw_bms_measure_temp,
    .read_coulombs  = fw_bms_read_coulombs,
    .set_chg_fet    = fw_bms_set_chg_fet,
    .set_dsg_fet    = fw_bms_set_dsg_fet,
    .set_balance    = fw_bms_set_balance,
    .get_balance    = fw_bms_get_balance,
    .fault_status   = fw_bms_fault_status,
    .fault_clear    = fw_bms_fault_clear,
    .pack_present   = fw_bms_pack_present,
    .ship_mode      = fw_bms_ship_mode,
    .isr            = fw_bms_isr,
};

/* ---- binding entry point ---- */
int uiox_bms_fwbind(uiox_bms_hw_t *hw, struct uiox_i2c_dev *i2c)
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
    int rc = uiox_bms_hw_init(hw, &s_bms_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
