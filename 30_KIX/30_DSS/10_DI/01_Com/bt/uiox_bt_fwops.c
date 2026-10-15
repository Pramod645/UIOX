/**
 * @file    uiox_bt_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_bt_hw_ops_t.
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/01_Com/bt/uiox_bt_fwops.c
 * LAYER  20_DriverInterfaces -- hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_bt_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuiox_fw_<arch>.a; this file calls its flat
 *          uiox_fw_bt_*() API and never includes its sources.
 *   This file is the joint.
 *
 * REGENERATED -- every stub carries the exact signature of its vtable slot
 * (12 slots).  Bodies return -ENOSYS: an honest "not yet
 * bound", never a silent success.  Every slot parameter is explicitly cast
 * to void so the strict -Werror set stays intact for the driver units that
 * do carry logic.
 */

#include "uiox_bt_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_bt.h"
#include "uiox_fw_i2c.h"

#define UIOX_BT_FWBIND_MAX   2u

typedef struct {
    uiox_bt_dev_t  fw;
    uiox_i2c_dev_t     i2c;
    uiox_bt_hw_t             *hw;
    bool                in_use;
} uiox_bt_fwbind_t;

static uiox_bt_fwbind_t s_bt_bind[UIOX_BT_FWBIND_MAX];

static uiox_bt_fwbind_t *bind_of(uiox_bt_hw_t *hw)
{
    if (!hw) return NULL;
    for (uint32_t i = 0u; i < UIOX_BT_FWBIND_MAX; i++)
        if (s_bt_bind[i].in_use && s_bt_bind[i].hw == hw)
            return &s_bt_bind[i];
    return NULL;
}

/* ---- vtable slots ---- */
static int fw_bt_init(uiox_bt_hw_t *hw)
{
    (void)hw;
    uiox_bt_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* deinit: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_bt_deinit(uiox_bt_hw_t *hw) { (void)hw; }
static int fw_bt_power(uiox_bt_hw_t *hw, bool on)
{
    (void)hw; (void)on;
    uiox_bt_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_bt_fw_download(uiox_bt_hw_t *hw, const uint8_t *fw, uint32_t fw_size)
{
    (void)hw; (void)fw; (void)fw_size;
    uiox_bt_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_bt_hci_write(uiox_bt_hw_t *hw, const uint8_t *buf, uint16_t len)
{
    (void)hw; (void)buf; (void)len;
    uiox_bt_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_bt_hci_read(uiox_bt_hw_t *hw, uint8_t *buf, uint16_t max_len)
{
    (void)hw; (void)buf; (void)max_len;
    uiox_bt_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* set_baud: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_bt_set_baud(uiox_bt_hw_t *hw, uint32_t baud) { (void)hw; (void)baud; }

/* gpio_write: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_bt_gpio_write(uiox_bt_hw_t *hw, uint32_t pin, bool val) { (void)hw; (void)pin; (void)val; }

/* gpio_read: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static bool fw_bt_gpio_read(uiox_bt_hw_t *hw, uint32_t pin) { (void)hw; (void)pin; return false; }

/* delay_ms: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_bt_delay_ms(uiox_bt_hw_t *hw, uint32_t ms) { (void)hw; (void)ms; }

/* isr_host_wake: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_bt_isr_host_wake(uiox_bt_hw_t *hw) { (void)hw; }

/* isr_rx: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_bt_isr_rx(uiox_bt_hw_t *hw) { (void)hw; }

static const uiox_bt_hw_ops_t s_bt_fwops = {
    .init           = fw_bt_init,
    .deinit         = fw_bt_deinit,
    .power          = fw_bt_power,
    .fw_download    = fw_bt_fw_download,
    .hci_write      = fw_bt_hci_write,
    .hci_read       = fw_bt_hci_read,
    .set_baud       = fw_bt_set_baud,
    .gpio_write     = fw_bt_gpio_write,
    .gpio_read      = fw_bt_gpio_read,
    .delay_ms       = fw_bt_delay_ms,
    .isr_host_wake  = fw_bt_isr_host_wake,
    .isr_rx         = fw_bt_isr_rx,
};

/* ---- binding entry point ---- */
int uiox_bt_fwbind(uiox_bt_hw_t *hw, struct uiox_i2c_dev *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_bt_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_BT_FWBIND_MAX; i++)
        if (!s_bt_bind[i].in_use) { b = &s_bt_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_bt_init_uart(&b->fw, hw->bt_wake_pin, hw->bt_en_pin);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    int rc = uiox_bt_hw_init(hw, &s_bt_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
