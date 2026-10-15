/**
 * @file    uiox_wifi_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_wifi_hw_ops_t.
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/02_Sensors/WiFi/uiox_wifi_fwops.c
 * LAYER  20_DriverInterfaces -- hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_wifi_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuiox_fw_<arch>.a; this file calls its flat
 *          uiox_fw_wifi_*() API and never includes its sources.
 *   This file is the joint.
 *
 * REGENERATED -- every stub carries the exact signature of its vtable slot
 * (21 slots).  Bodies return -ENOSYS: an honest "not yet
 * bound", never a silent success.  Every slot parameter is explicitly cast
 * to void so the strict -Werror set stays intact for the driver units that
 * do carry logic.
 */

#include "uiox_wifi_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_wifi.h"
#include "uiox_fw_i2c.h"

#define UIOX_WIFI_FWBIND_MAX   2u

typedef struct {
    uiox_wifi_dev_t  fw;
    uiox_i2c_dev_t     i2c;
    uiox_wifi_hw_t             *hw;
    bool                in_use;
} uiox_wifi_fwbind_t;

static uiox_wifi_fwbind_t s_wifi_bind[UIOX_WIFI_FWBIND_MAX];

static uiox_wifi_fwbind_t *bind_of(uiox_wifi_hw_t *hw)
{
    if (!hw) return NULL;
    for (uint32_t i = 0u; i < UIOX_WIFI_FWBIND_MAX; i++)
        if (s_wifi_bind[i].in_use && s_wifi_bind[i].hw == hw)
            return &s_wifi_bind[i];
    return NULL;
}

/* ---- vtable slots ---- */
static int fw_wifi_init(uiox_wifi_hw_t *hw)
{
    (void)hw;
    uiox_wifi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* deinit: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_wifi_deinit(uiox_wifi_hw_t *hw) { (void)hw; }
static int fw_wifi_start(uiox_wifi_hw_t *hw)
{
    (void)hw;
    uiox_wifi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* stop: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_wifi_stop(uiox_wifi_hw_t *hw) { (void)hw; }
static int fw_wifi_set_mode(uiox_wifi_hw_t *hw, uiox_wifi_mode_t mode)
{
    (void)hw; (void)mode;
    uiox_wifi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_wifi_set_channel(uiox_wifi_hw_t *hw, const uiox_wifi_channel_t *ch)
{
    (void)hw; (void)ch;
    uiox_wifi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_wifi_set_mac(uiox_wifi_hw_t *hw, const uiox_wifi_mac_t mac)
{
    (void)hw; (void)mac;
    uiox_wifi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_wifi_set_tx_power(uiox_wifi_hw_t *hw, int8_t dbm)
{
    (void)hw; (void)dbm;
    uiox_wifi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_wifi_tx_submit(uiox_wifi_hw_t *hw, uintptr_t phys, uint32_t length, uint8_t rate_idx, bool ampdu)
{
    (void)hw; (void)phys; (void)length; (void)rate_idx; (void)ampdu;
    uiox_wifi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_wifi_tx_reclaim(uiox_wifi_hw_t *hw)
{
    (void)hw;
    uiox_wifi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_wifi_rx_poll(uiox_wifi_hw_t *hw, uintptr_t *phys_out, uint32_t *len_out, int8_t *rssi_out)
{
    (void)hw; (void)phys_out; (void)len_out; (void)rssi_out;
    uiox_wifi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_wifi_hw_key_install(uiox_wifi_hw_t *hw, uint8_t key_idx, const uint8_t *key, uint8_t key_len, const uiox_wifi_mac_t peer, bool is_group, uint8_t cipher)
{
    (void)hw; (void)key_idx; (void)key; (void)key_len; (void)peer; (void)is_group; (void)cipher;
    uiox_wifi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* hw_key_delete: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_wifi_hw_key_delete(uiox_wifi_hw_t *hw, uint8_t key_idx) { (void)hw; (void)key_idx; }
static int fw_wifi_get_rssi(uiox_wifi_hw_t *hw, int8_t *rssi_dbm)
{
    (void)hw; (void)rssi_dbm;
    uiox_wifi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_wifi_get_noise(uiox_wifi_hw_t *hw, int8_t *noise_dbm)
{
    (void)hw; (void)noise_dbm;
    uiox_wifi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* isr_tx: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_wifi_isr_tx(uiox_wifi_hw_t *hw) { (void)hw; }

/* isr_rx: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_wifi_isr_rx(uiox_wifi_hw_t *hw) { (void)hw; }

/* isr_bcn: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_wifi_isr_bcn(uiox_wifi_hw_t *hw) { (void)hw; }

/* isr_err: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_wifi_isr_err(uiox_wifi_hw_t *hw) { (void)hw; }
static int fw_wifi_bus_read(uiox_wifi_hw_t *hw, uint32_t addr, void *buf, uint16_t len)
{
    (void)hw; (void)addr; (void)buf; (void)len;
    uiox_wifi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_wifi_bus_write(uiox_wifi_hw_t *hw, uint32_t addr, const void *buf, uint16_t len)
{
    (void)hw; (void)addr; (void)buf; (void)len;
    uiox_wifi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static const uiox_wifi_hw_ops_t s_wifi_fwops = {
    .init           = fw_wifi_init,
    .deinit         = fw_wifi_deinit,
    .start          = fw_wifi_start,
    .stop           = fw_wifi_stop,
    .set_mode       = fw_wifi_set_mode,
    .set_channel    = fw_wifi_set_channel,
    .set_mac        = fw_wifi_set_mac,
    .set_tx_power   = fw_wifi_set_tx_power,
    .tx_submit      = fw_wifi_tx_submit,
    .tx_reclaim     = fw_wifi_tx_reclaim,
    .rx_poll        = fw_wifi_rx_poll,
    .hw_key_install = fw_wifi_hw_key_install,
    .hw_key_delete  = fw_wifi_hw_key_delete,
    .get_rssi       = fw_wifi_get_rssi,
    .get_noise      = fw_wifi_get_noise,
    .isr_tx         = fw_wifi_isr_tx,
    .isr_rx         = fw_wifi_isr_rx,
    .isr_bcn        = fw_wifi_isr_bcn,
    .isr_err        = fw_wifi_isr_err,
    .bus_read       = fw_wifi_bus_read,
    .bus_write      = fw_wifi_bus_write,
};

/* ---- binding entry point ---- */
int uiox_wifi_fwbind(uiox_wifi_hw_t *hw, struct uiox_i2c_dev *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_wifi_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_WIFI_FWBIND_MAX; i++)
        if (!s_wifi_bind[i].in_use) { b = &s_wifi_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_wifi_init_sdio(&b->fw, hw->base_addr, hw->irq);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    int rc = uiox_wifi_hw_init(hw, &s_wifi_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
