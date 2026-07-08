/**
 * @file  uiox_fw_i2c.c
 * @brief UIOX Firmware HAL — DesignWare APB I2C master driver.
 *        No libc — bare MMIO only.
 * @date  2026-07-07
 */

 #include "../include/uiox_fw_i2c.h"
 #include "../include/uiox_fw_hw.h"
 
 #define OPS(d) ((const uiox_i2c_ops_t *)(d)->priv)
 
 /* =========================================================================
  * MMIO helpers (reuse from uiox_fw_types.h)
  * ====================================================================== */
 
 static inline void i2c_wr(uintptr_t b, uint32_t off, uint32_t v)
 { *((volatile uint32_t *)(b + off)) = v; }
 
 static inline uint32_t i2c_rd(uintptr_t b, uint32_t off)
 { return *((volatile uint32_t *)(b + off)); }
 
 /* =========================================================================
  * DesignWare I2C driver
  * ====================================================================== */
 
 static uiox_fw_err_t dw_disable(uintptr_t b)
 {
     i2c_wr(b, DW_I2C_ENABLE, 0u);
     for (uint32_t i = 0u; i < 500u; i++) {
         if (!(i2c_rd(b, DW_I2C_ENABLE) & 1u)) return UIOX_FW_OK;
         volatile uint32_t n = 100u; while (n--) ;
     }
     return UIOX_FW_ERR_TIMEOUT;
 }
 
 static uiox_fw_err_t dw_init(uiox_i2c_dev_t *dev, uiox_i2c_speed_t speed)
 {
     uintptr_t b = dev->base;
 
     dw_disable(b);
     i2c_wr(b, DW_I2C_INTR_MASK, 0u);
 
     /* Configure CON: master, no 10-bit, restart enabled, slave disabled */
     uint32_t con = DW_I2C_CON_MASTER | DW_I2C_CON_SLAVE_DISABLE |
                    DW_I2C_CON_RESTART_EN;
     uint32_t hcnt, lcnt;
     uint32_t ic_clk = dev->clk_hz / 1000u; /* in kHz */
 
     if (speed >= UIOX_I2C_SPEED_FAST) {
         con |= DW_I2C_CON_SPEED_FAST;
         /* tHD;STA ≥ 0.6 µs, tLOW ≥ 1.3 µs, tHIGH ≥ 0.6 µs */
         hcnt = (ic_clk * 6u  / 10000u) + 1u;
         lcnt = (ic_clk * 13u / 10000u) + 1u;
         i2c_wr(b, DW_I2C_FS_SCL_HCNT, hcnt);
         i2c_wr(b, DW_I2C_FS_SCL_LCNT, lcnt);
     } else {
         con |= DW_I2C_CON_SPEED_STD;
         /* tLOW ≥ 4.7 µs, tHIGH ≥ 4.0 µs */
         hcnt = (ic_clk * 40u / 10000u) + 1u;
         lcnt = (ic_clk * 47u / 10000u) + 1u;
         i2c_wr(b, DW_I2C_SS_SCL_HCNT, hcnt);
         i2c_wr(b, DW_I2C_SS_SCL_LCNT, lcnt);
     }
 
     i2c_wr(b, DW_I2C_CON, con);
     i2c_wr(b, DW_I2C_RX_TL, 0u);  /* RX threshold = 1 byte */
     i2c_wr(b, DW_I2C_TX_TL, 0u);  /* TX threshold = empty   */
     i2c_wr(b, DW_I2C_ENABLE, 1u);
 
     dev->speed       = speed;
     dev->initialized = true;
     return UIOX_FW_OK;
 }
 
 static void dw_deinit(uiox_i2c_dev_t *dev)
 {
     dw_disable(dev->base);
     dev->initialized = false;
 }
 
 static uiox_fw_err_t dw_wait_tx_empty(uintptr_t b)
 {
     for (uint32_t i = 0u; i < 100000u; i++) {
         uint32_t st = i2c_rd(b, DW_I2C_STATUS);
         if ((st & DW_I2C_STATUS_TFE) && !(st & DW_I2C_STATUS_MST_ACTV))
             return UIOX_FW_OK;
         if (i2c_rd(b, DW_I2C_INTR_STAT) & DW_I2C_INTR_TX_ABRT) {
             i2c_rd(b, DW_I2C_CLR_TX_ABRT);  /* clear abort */
             return UIOX_FW_ERR_IO;
         }
     }
     return UIOX_FW_ERR_TIMEOUT;
 }
 
 static uiox_fw_err_t dw_transfer(uiox_i2c_dev_t *dev, uiox_i2c_xfer_t *xfer)
 {
     uintptr_t b = dev->base;
 
     /* Set target address */
     i2c_wr(b, DW_I2C_TAR, (uint32_t)xfer->addr & 0x7Fu);
 
     /* Transmit phase */
     for (uint32_t i = 0u; i < xfer->tx_len; i++) {
         uint32_t cmd = xfer->tx_buf[i];
         /* Last TX byte: send STOP unless repeated start follows */
         if (i == xfer->tx_len - 1u && xfer->rx_len == 0u &&
             !xfer->repeated_start)
             cmd |= DW_I2C_CMD_STOP;
         /* Wait for TX FIFO not full */
         for (uint32_t w = 0u; w < 100000u; w++) {
             if (i2c_rd(b, DW_I2C_STATUS) & DW_I2C_STATUS_TFNF) break;
         }
         i2c_wr(b, DW_I2C_DATA_CMD, cmd);
         dev->stats.bytes_tx++;
     }
 
     /* Receive phase */
     for (uint32_t i = 0u; i < xfer->rx_len; i++) {
         uint32_t cmd = DW_I2C_CMD_READ;
         if (i == xfer->rx_len - 1u) cmd |= DW_I2C_CMD_STOP;
         for (uint32_t w = 0u; w < 100000u; w++) {
             if (i2c_rd(b, DW_I2C_STATUS) & DW_I2C_STATUS_TFNF) break;
         }
         i2c_wr(b, DW_I2C_DATA_CMD, cmd);
         /* Wait for RX FIFO not empty */
         for (uint32_t w = 0u; w < 100000u; w++) {
             if (i2c_rd(b, DW_I2C_STATUS) & DW_I2C_STATUS_RFNE) break;
         }
         xfer->rx_buf[i] = (uint8_t)(i2c_rd(b, DW_I2C_DATA_CMD) & 0xFFu);
         dev->stats.bytes_rx++;
     }
 
     uiox_fw_err_t rc = dw_wait_tx_empty(b);
     if (rc != UIOX_FW_OK) dev->stats.nack_errors++;
     dev->stats.transfers++;
     return rc;
 }
 
 static uiox_fw_err_t dw_write_reg(uiox_i2c_dev_t *dev,
                                     uint8_t addr, uint8_t reg,
                                     const uint8_t *buf, uint32_t len)
 {
     /* Build a single TX buffer: [reg, data...] */
     uint8_t tx[64];
     if (len + 1u > 64u) return UIOX_FW_ERR_OVERFLOW;
     tx[0] = reg;
     for (uint32_t i = 0u; i < len; i++) tx[i + 1u] = buf[i];
     uiox_i2c_xfer_t xfer = {
         .addr           = addr,
         .tx_buf         = tx,
         .tx_len         = len + 1u,
         .rx_buf         = NULL,
         .rx_len         = 0u,
         .repeated_start = false,
     };
     return dw_transfer(dev, &xfer);
 }
 
 static uiox_fw_err_t dw_read_reg(uiox_i2c_dev_t *dev,
                                    uint8_t addr, uint8_t reg,
                                    uint8_t *buf, uint32_t len)
 {
     uint8_t tx_reg = reg;
     uiox_i2c_xfer_t xfer = {
         .addr           = addr,
         .tx_buf         = &tx_reg,
         .tx_len         = 1u,
         .rx_buf         = buf,
         .rx_len         = len,
         .repeated_start = true,
     };
     return dw_transfer(dev, &xfer);
 }
 
 static bool dw_bus_busy(uiox_i2c_dev_t *dev)
 {
     return !!(i2c_rd(dev->base, DW_I2C_STATUS) & DW_I2C_STATUS_MST_ACTV);
 }
 
 static uiox_fw_err_t dw_recover_bus(uiox_i2c_dev_t *dev)
 {
     /* Disable + re-enable */
     dw_disable(dev->base);
     i2c_wr(dev->base, DW_I2C_ENABLE, 1u);
     return UIOX_FW_OK;
 }
 
 static void dw_isr(uiox_i2c_dev_t *dev)
 {
     uint32_t stat = i2c_rd(dev->base, DW_I2C_INTR_STAT);
     if (stat & DW_I2C_INTR_TX_ABRT) {
         dev->stats.nack_errors++;
         i2c_rd(dev->base, DW_I2C_CLR_TX_ABRT);
     }
     i2c_wr(dev->base, DW_I2C_CLR_INTR, stat);
 }
 
 static const uiox_i2c_ops_t dw_i2c_ops = {
     .init        = dw_init,
     .deinit      = dw_deinit,
     .transfer    = dw_transfer,
     .write_reg   = dw_write_reg,
     .read_reg    = dw_read_reg,
     .bus_busy    = dw_bus_busy,
     .recover_bus = dw_recover_bus,
     .isr         = dw_isr,
 };
 
 /* =========================================================================
  * Public API
  * ====================================================================== */
 
 uiox_fw_err_t uiox_fw_i2c_init(uiox_i2c_dev_t *dev,
                                   const uiox_i2c_ops_t *ops,
                                   uiox_i2c_speed_t speed)
 {
     if (!dev || !ops || !ops->init) return UIOX_FW_ERR_INVAL;
     dev->priv = (void *)ops;
     return ops->init(dev, speed);
 }
 
 void uiox_fw_i2c_deinit(uiox_i2c_dev_t *dev)
 {
     if (!dev || !dev->priv) return;
     if (OPS(dev)->deinit) OPS(dev)->deinit(dev);
 }
 
 uiox_fw_err_t uiox_fw_i2c_transfer(uiox_i2c_dev_t *dev,
                                       uiox_i2c_xfer_t *xfer)
 {
     if (!dev || !dev->priv || !xfer) return UIOX_FW_ERR_INVAL;
     if (!OPS(dev)->transfer) return UIOX_FW_ERR_UNSUP;
     return OPS(dev)->transfer(dev, xfer);
 }
 
 uiox_fw_err_t uiox_fw_i2c_write_reg(uiox_i2c_dev_t *dev,
                                        uint8_t addr, uint8_t reg,
                                        const uint8_t *buf, uint32_t len)
 {
     if (!dev || !dev->priv) return UIOX_FW_ERR_INVAL;
     if (!OPS(dev)->write_reg) return UIOX_FW_ERR_UNSUP;
     return OPS(dev)->write_reg(dev, addr, reg, buf, len);
 }
 
 uiox_fw_err_t uiox_fw_i2c_read_reg(uiox_i2c_dev_t *dev,
                                       uint8_t addr, uint8_t reg,
                                       uint8_t *buf, uint32_t len)
 {
     if (!dev || !dev->priv) return UIOX_FW_ERR_INVAL;
     if (!OPS(dev)->read_reg) return UIOX_FW_ERR_UNSUP;
     return OPS(dev)->read_reg(dev, addr, reg, buf, len);
 }
 
 bool uiox_fw_i2c_bus_busy(uiox_i2c_dev_t *dev)
 {
     if (!dev || !dev->priv || !OPS(dev)->bus_busy) return false;
     return OPS(dev)->bus_busy(dev);
 }
 
 uiox_fw_err_t uiox_fw_i2c_recover(uiox_i2c_dev_t *dev)
 {
     if (!dev || !dev->priv || !OPS(dev)->recover_bus)
         return UIOX_FW_ERR_NODEV;
     return OPS(dev)->recover_bus(dev);
 }
 
 void uiox_fw_i2c_stats_get(const uiox_i2c_dev_t *dev,
                              uiox_i2c_stats_t *out)
 {
     if (!dev || !out) return;
     /* bare memcpy without libc */
     const uint8_t *s = (const uint8_t *)&dev->stats;
     uint8_t       *d = (uint8_t *)out;
     for (size_t i = 0u; i < sizeof(uiox_i2c_stats_t); i++) d[i] = s[i];
 }
 
 uiox_fw_err_t uiox_fw_i2c_init_dw(uiox_i2c_dev_t *dev,
                                      uintptr_t base, uint32_t clk_hz,
                                      uint32_t irq,
                                      uiox_i2c_speed_t speed)
 {
     if (!dev) return UIOX_FW_ERR_INVAL;
     /* zero-init without memset/libc */
     uint8_t *p = (uint8_t *)dev;
     for (size_t i = 0u; i < sizeof(*dev); i++) p[i] = 0u;
     dev->base   = base;
     dev->clk_hz = clk_hz;
     dev->irq    = irq;
     dev->caps   = UIOX_I2C_CAP_SMBUS;
     /* Copy model string */
     const char *m = "DW APB I2C";
     for (int i = 0; m[i] && i < (int)UIOX_I2C_MODEL_LEN - 1; i++)
         dev->model[i] = m[i];
     return uiox_fw_i2c_init(dev, &dw_i2c_ops, speed);
 }
 