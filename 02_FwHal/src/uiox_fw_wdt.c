/**
 * @file  uiox_fw_wdt.c
 * @brief UIOX Firmware HAL — SP805 watchdog driver. No libc.
 * @date  2026-07-07
 */

 #include "../include/uiox_fw_wdt.h"

 #define OPS(d) ((const uiox_wdt_ops_t *)(d)->priv)
 
 static inline void w_wr(uintptr_t b, uint32_t o, uint32_t v)
 { *((volatile uint32_t *)(b+o)) = v; }
 static inline uint32_t w_rd(uintptr_t b, uint32_t o)
 { return *((volatile uint32_t *)(b+o)); }
 
 static void sp805_unlock(uintptr_t b)
 { w_wr(b, SP805_WDT_LOCK, SP805_UNLOCK_MAGIC); }
 static void sp805_lock(uintptr_t b)
 { w_wr(b, SP805_WDT_LOCK, 0u); }
 
 static uiox_fw_err_t sp805_init(uiox_wdt_dev_t *dev, uint32_t timeout_ms)
 {
     uintptr_t b    = dev->base;
     uint32_t ticks = (dev->clk_hz / 1000u) * timeout_ms;
     sp805_unlock(b);
     w_wr(b, SP805_WDT_CTRL, 0u);     /* disable while configuring */
     w_wr(b, SP805_WDT_LOAD, ticks);
     w_wr(b, SP805_WDT_CTRL, SP805_CTRL_INTEN | SP805_CTRL_RESEN);
     sp805_lock(b);
     dev->timeout_ms  = timeout_ms;
     dev->initialized = true;
     return UIOX_FW_OK;
 }
 
 static void sp805_kick(uiox_wdt_dev_t *dev)
 {
     sp805_unlock(dev->base);
     w_wr(dev->base, SP805_WDT_LOAD,
          (dev->clk_hz / 1000u) * dev->timeout_ms);
     sp805_lock(dev->base);
 }
 
 static void sp805_stop(uiox_wdt_dev_t *dev)
 {
     sp805_unlock(dev->base);
     w_wr(dev->base, SP805_WDT_CTRL, 0u);
     sp805_lock(dev->base);
 }
 
 static void sp805_start(uiox_wdt_dev_t *dev)
 {
     sp805_unlock(dev->base);
     w_wr(dev->base, SP805_WDT_CTRL,
          SP805_CTRL_INTEN | SP805_CTRL_RESEN);
     sp805_lock(dev->base);
 }
 
 static uint32_t sp805_remaining(uiox_wdt_dev_t *dev)
 {
     uint32_t ticks = w_rd(dev->base, SP805_WDT_VALUE);
     return ticks / (dev->clk_hz / 1000u);
 }
 
 static const uiox_wdt_ops_t sp805_ops = {
     .init      = sp805_init,
     .kick      = sp805_kick,
     .stop      = sp805_stop,
     .start     = sp805_start,
     .remaining = sp805_remaining,
 };
 
 uiox_fw_err_t uiox_fw_wdt_init(uiox_wdt_dev_t *dev,
                                   const uiox_wdt_ops_t *ops,
                                   uint32_t timeout_ms)
 {
     if (!dev || !ops || !ops->init) return UIOX_FW_ERR_INVAL;
     dev->priv = (void *)ops;
     return ops->init(dev, timeout_ms);
 }
 void     uiox_fw_wdt_kick(uiox_wdt_dev_t *dev)
 { if (dev && dev->priv && OPS(dev)->kick) OPS(dev)->kick(dev); }
 void     uiox_fw_wdt_stop(uiox_wdt_dev_t *dev)
 { if (dev && dev->priv && OPS(dev)->stop) OPS(dev)->stop(dev); }
 void     uiox_fw_wdt_start(uiox_wdt_dev_t *dev)
 { if (dev && dev->priv && OPS(dev)->start) OPS(dev)->start(dev); }
 uint32_t uiox_fw_wdt_remaining(uiox_wdt_dev_t *dev)
 { return (dev && dev->priv && OPS(dev)->remaining)
          ? OPS(dev)->remaining(dev) : 0u; }
 
 uiox_fw_err_t uiox_fw_wdt_init_sp805(uiox_wdt_dev_t *dev,
                                         uintptr_t base, uint32_t clk_hz,
                                         uint32_t timeout_ms)
 {
     if (!dev) return UIOX_FW_ERR_INVAL;
     uint8_t *p = (uint8_t *)dev;
     for (size_t i = 0u; i < sizeof(*dev); i++) p[i] = 0u;
     dev->base   = base;
     dev->clk_hz = clk_hz;
     return uiox_fw_wdt_init(dev, &sp805_ops, timeout_ms);
 }
 