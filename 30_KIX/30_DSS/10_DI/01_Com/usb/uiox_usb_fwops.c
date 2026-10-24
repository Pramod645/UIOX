/**
 * @file    uiox_usb_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_usb_hw_ops_t.
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/01_Com/usb/uiox_usb_fwops.c
 * LAYER  20_DriverInterfaces -- hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_usb_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuiox_fw_<arch>.a; this file calls its flat
 *          uiox_fw_usb_*() API and never includes its sources.
 *   This file is the joint.
 *
 * REGENERATED -- every stub carries the exact signature of its vtable slot
 * (23 slots).  Bodies return -ENOSYS: an honest "not yet
 * bound", never a silent success.  Every slot parameter is explicitly cast
 * to void so the strict -Werror set stays intact for the driver units that
 * do carry logic.
 */

#include "uiox_usb_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_usb.h"
#include "uiox_fw_i2c.h"

#define UIOX_USB_FWBIND_MAX   2u

typedef struct {
    uiox_usb_hc_dev_t  fw;
    uiox_i2c_dev_t     i2c;
    uiox_usb_hw_t             *hw;
    bool                in_use;
} uiox_usb_fwbind_t;

static uiox_usb_fwbind_t s_usb_bind[UIOX_USB_FWBIND_MAX];

static uiox_usb_fwbind_t *bind_of(uiox_usb_hw_t *hw)
{
    if (!hw) return NULL;
    for (uint32_t i = 0u; i < UIOX_USB_FWBIND_MAX; i++)
        if (s_usb_bind[i].in_use && s_usb_bind[i].hw == hw)
            return &s_usb_bind[i];
    return NULL;
}
/* Exported so a class driver can read the enumerated list.  bind_of()
 * above is file-static; this is its public face. */
struct uiox_usb_hc_dev *uiox_usb_hw_fwdev(const uiox_usb_hw_t *hw)
{
    uiox_usb_fwbind_t *b = bind_of((uiox_usb_hw_t *)hw);
    return b ? &b->fw : NULL;
}

/* ---- vtable slots ---- */
static int fw_usb_init(uiox_usb_hw_t *hw)
{
    (void)hw;
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* deinit: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_usb_deinit(uiox_usb_hw_t *hw) { (void)hw; }
static int fw_usb_start(uiox_usb_hw_t *hw)
{
    (void)hw;
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* stop: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_usb_stop(uiox_usb_hw_t *hw) { (void)hw; }
static int fw_usb_set_role(uiox_usb_hw_t *hw, uiox_usb_role_t role)
{
    (void)hw; (void)role;
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_usb_set_address(uiox_usb_hw_t *hw, uint8_t addr)
{
    (void)hw; (void)addr;
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_usb_ep_config(uiox_usb_hw_t *hw, uint8_t ep_addr, uiox_usb_ep_type_t type, uint16_t mps, uint8_t interval)
{
    (void)hw; (void)ep_addr; (void)type; (void)mps; (void)interval;
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_usb_ep_disable(uiox_usb_hw_t *hw, uint8_t ep_addr)
{
    (void)hw; (void)ep_addr;
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_usb_ep_stall(uiox_usb_hw_t *hw, uint8_t ep_addr, bool stall)
{
    (void)hw; (void)ep_addr; (void)stall;
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_usb_ep_flush(uiox_usb_hw_t *hw, uint8_t ep_addr)
{
    (void)hw; (void)ep_addr;
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_usb_tx_submit(uiox_usb_hw_t *hw, uint8_t ep_addr, uintptr_t phys, uint32_t len)
{
    (void)hw; (void)ep_addr; (void)phys; (void)len;
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_usb_rx_submit(uiox_usb_hw_t *hw, uint8_t ep_addr, uintptr_t phys, uint32_t len)
{
    (void)hw; (void)ep_addr; (void)phys; (void)len;
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_usb_tx_complete(uiox_usb_hw_t *hw, uint8_t ep_addr, uint32_t *bytes_done)
{
    (void)hw; (void)ep_addr; (void)bytes_done;
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_usb_rx_complete(uiox_usb_hw_t *hw, uint8_t ep_addr, uint32_t *bytes_done)
{
    (void)hw; (void)ep_addr; (void)bytes_done;
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static bool fw_usb_vbus_sense(uiox_usb_hw_t *hw)
{
    (void)hw;
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_usb_remote_wakeup(uiox_usb_hw_t *hw)
{
    (void)hw;
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* isr_sof: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_usb_isr_sof(uiox_usb_hw_t *hw) { (void)hw; }

/* isr_reset: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_usb_isr_reset(uiox_usb_hw_t *hw) { (void)hw; }

/* isr_suspend: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_usb_isr_suspend(uiox_usb_hw_t *hw) { (void)hw; }

/* isr_resume: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_usb_isr_resume(uiox_usb_hw_t *hw) { (void)hw; }

/* isr_ep: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_usb_isr_ep(uiox_usb_hw_t *hw, uint8_t ep_addr) { (void)hw; (void)ep_addr; }

/* isr_connect: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_usb_isr_connect(uiox_usb_hw_t *hw) { (void)hw; }

/* isr_disconnect: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_usb_isr_disconnect(uiox_usb_hw_t *hw) { (void)hw; }

static const uiox_usb_hw_ops_t s_usb_fwops = {
    .init           = fw_usb_init,
    .deinit         = fw_usb_deinit,
    .start          = fw_usb_start,
    .stop           = fw_usb_stop,
    .set_role       = fw_usb_set_role,
    .set_address    = fw_usb_set_address,
    .ep_config      = fw_usb_ep_config,
    .ep_disable     = fw_usb_ep_disable,
    .ep_stall       = fw_usb_ep_stall,
    .ep_flush       = fw_usb_ep_flush,
    .tx_submit      = fw_usb_tx_submit,
    .rx_submit      = fw_usb_rx_submit,
    .tx_complete    = fw_usb_tx_complete,
    .rx_complete    = fw_usb_rx_complete,
    .vbus_sense     = fw_usb_vbus_sense,
    .remote_wakeup  = fw_usb_remote_wakeup,
    .isr_sof        = fw_usb_isr_sof,
    .isr_reset      = fw_usb_isr_reset,
    .isr_suspend    = fw_usb_isr_suspend,
    .isr_resume     = fw_usb_isr_resume,
    .isr_ep         = fw_usb_isr_ep,
    .isr_connect    = fw_usb_isr_connect,
    .isr_disconnect = fw_usb_isr_disconnect,
};

/* ---- binding entry point ---- */
int uiox_usb_fwbind(uiox_usb_hw_t *hw, struct uiox_i2c_dev *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_usb_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_USB_FWBIND_MAX; i++)
        if (!s_usb_bind[i].in_use) { b = &s_usb_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_usb_init_xhci(&b->fw, hw->base_addr, hw->irq);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    int rc = uiox_usb_hw_init(hw, &s_usb_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
