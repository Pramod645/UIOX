/**
 * @file  uiox_fw_spi.c
 * @brief UIOX Firmware HAL — PL022 SSP SPI master driver.
 *        No libc — bare MMIO only.
 * @date  2026-07-07
 */

 #include "../include/uiox_fw_spi.h"
 #include "../include/uiox_fw_hw.h"
 
 #define OPS(d) ((const uiox_spi_ops_t *)(d)->priv)
 
 static inline void s_wr(uintptr_t b, uint32_t o, uint32_t v)
 { *((volatile uint32_t *)(b + o)) = v; }
 static inline uint32_t s_rd(uintptr_t b, uint32_t o)
 { return *((volatile uint32_t *)(b + o)); }
 
 /* =========================================================================
  * PL022 driver
  * ====================================================================== */
 
 static uiox_fw_err_t pl022_init(uiox_spi_dev_t *dev,
                                   uiox_spi_mode_t mode,
                                   uiox_spi_bits_t bits,
                                   uint32_t speed_hz)
 {
     uintptr_t b = dev->base;
     /* Disable SSP */
     s_wr(b, PL022_SSPCR1, 0u);
 
     /* Clock prescale: CPSDVSR must be even, 2–254 */
     uint32_t cpsdvsr = dev->clk_hz / speed_hz;
     if (cpsdvsr < 2u)   cpsdvsr = 2u;
     if (cpsdvsr > 254u) cpsdvsr = 254u;
     cpsdvsr &= ~1u;  /* force even */
     uint32_t scr = (dev->clk_hz / (cpsdvsr * speed_hz)) - 1u;
     if (scr > 255u) scr = 255u;
 
     s_wr(b, PL022_SSPCPSR, cpsdvsr);
 
     /* CR0: data size, SPI frame, CPOL, CPHA, SCR */
     uint32_t dss = (uint32_t)bits - 1u;
     uint32_t cr0 = (dss & PL022_CR0_DSS_MASK) | PL022_CR0_FRF_SPI;
     if (mode & 0x2u) cr0 |= PL022_CR0_SPO;
     if (mode & 0x1u) cr0 |= PL022_CR0_SPH;
     cr0 |= (scr << PL022_CR0_SCR_SHIFT);
     s_wr(b, PL022_SSPCR0, cr0);
 
     /* Enable SSP as master */
     s_wr(b, PL022_SSPCR1, PL022_CR1_SSE);
 
     dev->mode        = mode;
     dev->bits        = bits;
     dev->speed_hz    = speed_hz;
     dev->initialized = true;
     return UIOX_FW_OK;
 }
 
 static void pl022_deinit(uiox_spi_dev_t *dev)
 {
     s_wr(dev->base, PL022_SSPCR1, 0u);
     dev->initialized = false;
 }
 
 static void pl022_cs_assert(uiox_spi_dev_t *dev, uint32_t cs)
 {
     /* CS is GPIO-controlled — caller wires GPIO write here */
     (void)dev; (void)cs;
 }
 
 static void pl022_cs_release(uiox_spi_dev_t *dev, uint32_t cs)
 {
     (void)dev; (void)cs;
 }
 
 static uiox_fw_err_t pl022_transfer(uiox_spi_dev_t *dev,
                                       const uint8_t *tx, uint8_t *rx,
                                       uint32_t len)
 {
     uintptr_t b = dev->base;
     for (uint32_t i = 0u; i < len; i++) {
         /* Wait for TX FIFO not full */
         for (uint32_t w = 0u; w < 100000u; w++)
             if (s_rd(b, PL022_SSPSR) & PL022_SR_TNF) break;
         s_wr(b, PL022_SSPDR, tx ? (uint32_t)tx[i] : 0xFFu);
         /* Wait for RX FIFO not empty */
         for (uint32_t w = 0u; w < 100000u; w++)
             if (s_rd(b, PL022_SSPSR) & PL022_SR_RNE) break;
         uint32_t rxd = s_rd(b, PL022_SSPDR) & 0xFFu;
         if (rx) rx[i] = (uint8_t)rxd;
         dev->stats.bytes_tx++;
         dev->stats.bytes_rx++;
     }
     /* Wait until not busy */
     for (uint32_t w = 0u; w < 100000u; w++)
         if (!(s_rd(b, PL022_SSPSR) & PL022_SR_BSY)) break;
     dev->stats.transfers++;
     return UIOX_FW_OK;
 }
 
 static uiox_fw_err_t pl022_write(uiox_spi_dev_t *dev,
                                    const uint8_t *buf, uint32_t len)
 { return pl022_transfer(dev, buf, NULL, len); }
 
 static uiox_fw_err_t pl022_read(uiox_spi_dev_t *dev,
                                   uint8_t *buf, uint32_t len)
 { return pl022_transfer(dev, NULL, buf, len); }
 
 static void pl022_isr(uiox_spi_dev_t *dev)
 { s_wr(dev->base, PL022_SSPICR, 0x3u); }  /* clear RX/ROR interrupts */
 
 static const uiox_spi_ops_t pl022_ops = {
     .init       = pl022_init,
     .deinit     = pl022_deinit,
     .cs_assert  = pl022_cs_assert,
     .cs_release = pl022_cs_release,
     .transfer   = pl022_transfer,
     .write      = pl022_write,
     .read       = pl022_read,
     .isr        = pl022_isr,
 };
 
 /* =========================================================================
  * Public API
  * ====================================================================== */
 
 uiox_fw_err_t uiox_fw_spi_init(uiox_spi_dev_t *dev,
                                   const uiox_spi_ops_t *ops,
                                   uiox_spi_mode_t mode,
                                   uiox_spi_bits_t bits,
                                   uint32_t speed_hz)
 {
     if (!dev || !ops || !ops->init) return UIOX_FW_ERR_INVAL;
     dev->priv = (void *)ops;
     return ops->init(dev, mode, bits, speed_hz);
 }
 
 void uiox_fw_spi_deinit(uiox_spi_dev_t *dev)
 { if (dev && dev->priv && OPS(dev)->deinit) OPS(dev)->deinit(dev); }
 
 void uiox_fw_spi_cs_assert(uiox_spi_dev_t *dev, uint32_t cs)
 { if (dev && dev->priv && OPS(dev)->cs_assert) OPS(dev)->cs_assert(dev, cs); }
 
 void uiox_fw_spi_cs_release(uiox_spi_dev_t *dev, uint32_t cs)
 { if (dev && dev->priv && OPS(dev)->cs_release) OPS(dev)->cs_release(dev, cs); }
 
 uiox_fw_err_t uiox_fw_spi_transfer(uiox_spi_dev_t *dev,
                                       const uint8_t *tx, uint8_t *rx,
                                       uint32_t len)
 {
     if (!dev || !dev->priv || !OPS(dev)->transfer) return UIOX_FW_ERR_INVAL;
     return OPS(dev)->transfer(dev, tx, rx, len);
 }
 
 uiox_fw_err_t uiox_fw_spi_write(uiox_spi_dev_t *dev,
                                    const uint8_t *buf, uint32_t len)
 {
     if (!dev || !dev->priv || !OPS(dev)->write) return UIOX_FW_ERR_INVAL;
     return OPS(dev)->write(dev, buf, len);
 }
 
 uiox_fw_err_t uiox_fw_spi_read(uiox_spi_dev_t *dev,
                                   uint8_t *buf, uint32_t len)
 {
     if (!dev || !dev->priv || !OPS(dev)->read) return UIOX_FW_ERR_INVAL;
     return OPS(dev)->read(dev, buf, len);
 }
 
 uiox_fw_err_t uiox_fw_spi_init_pl022(uiox_spi_dev_t *dev,
                                         uintptr_t base, uint32_t clk_hz,
                                         uint32_t irq)
 {
     if (!dev) return UIOX_FW_ERR_INVAL;
     uint8_t *p = (uint8_t *)dev;
     for (size_t i = 0u; i < sizeof(*dev); i++) p[i] = 0u;
     dev->base   = base;
     dev->clk_hz = clk_hz;
     dev->irq    = irq;
     const char *m = "ARM PL022 SSP";
     for (int i = 0; m[i] && i < (int)UIOX_SPI_MODEL_LEN - 1; i++)
         dev->model[i] = m[i];
     return uiox_fw_spi_init(dev, &pl022_ops,
                               UIOX_SPI_MODE_0, UIOX_SPI_BITS_8, 1000000u);
 }
 