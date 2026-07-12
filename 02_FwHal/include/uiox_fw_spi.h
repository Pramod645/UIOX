/**
 * @file  uiox_fw_spi.h
 * @brief UIOX Firmware HAL — SPI master controller.
 *
 * Supports:
 *   ARM64/ARM32 — ARM PrimeCell PL022 SSP
 *   x86_64      — Intel ICH SPI controller
 *
 * Used by: NOR flash, displays (ST7789/ILI9341), ALS (TSL2591),
 *          IMU (BMI270), SD cards in SPI fallback mode.
 *
 * @version 1.0.0
 * @date    2026-07-07
 */

 #ifndef UIOX_FW_SPI_H
 #define UIOX_FW_SPI_H
 
 #include "uiox_fw_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * SPI mode (CPOL + CPHA)
  * ====================================================================== */
 
 typedef enum {
     UIOX_SPI_MODE_0 = 0,  /**< CPOL=0, CPHA=0 — idle low,  sample rising  */
     UIOX_SPI_MODE_1 = 1,  /**< CPOL=0, CPHA=1 — idle low,  sample falling */
     UIOX_SPI_MODE_2 = 2,  /**< CPOL=1, CPHA=0 — idle high, sample falling */
     UIOX_SPI_MODE_3 = 3,  /**< CPOL=1, CPHA=1 — idle high, sample rising  */
 } uiox_spi_mode_t;
 
 typedef enum {
     UIOX_SPI_BITS_4  =  4u,
     UIOX_SPI_BITS_8  =  8u,
     UIOX_SPI_BITS_16 = 16u,
 } uiox_spi_bits_t;
 
 /* =========================================================================
  * PL022 SSP register offsets
  * ====================================================================== */
 
 #define PL022_SSPCR0        0x000u  /**< Control register 0              */
 #define PL022_SSPCR1        0x004u  /**< Control register 1              */
 #define PL022_SSPDR         0x008u  /**< Data register                   */
 #define PL022_SSPSR         0x00Cu  /**< Status register                 */
 #define PL022_SSPCPSR       0x010u  /**< Clock prescale                  */
 #define PL022_SSPIMSC       0x014u  /**< Interrupt mask set/clear        */
 #define PL022_SSPRIS        0x018u  /**< Raw interrupt status            */
 #define PL022_SSPMIS        0x01Cu  /**< Masked interrupt status         */
 #define PL022_SSPICR        0x020u  /**< Interrupt clear register        */
 #define PL022_SSPDMACR      0x024u  /**< DMA control                     */
 
 /* SSPCR0 bits */
 #define PL022_CR0_DSS_MASK  0x0Fu   /**< Data size select [3:0]         */
 #define PL022_CR0_FRF_SPI   (0u<<4) /**< Frame format: SPI              */
 #define PL022_CR0_SPO       (1u<<6) /**< CPOL                           */
 #define PL022_CR0_SPH       (1u<<7) /**< CPHA                           */
 #define PL022_CR0_SCR_SHIFT 8u      /**< Serial clock rate [15:8]       */
 
 /* SSPCR1 bits */
 #define PL022_CR1_LBM       (1u<<0) /**< Loopback mode                  */
 #define PL022_CR1_SSE       (1u<<1) /**< SSP enable                     */
 #define PL022_CR1_MS        (1u<<2) /**< Master/slave (0=master)        */
 #define PL022_CR1_SOD       (1u<<3) /**< Slave output disable           */
 
 /* SSPSR bits */
 #define PL022_SR_TFE        (1u<<0) /**< TX FIFO empty                  */
 #define PL022_SR_TNF        (1u<<1) /**< TX FIFO not full               */
 #define PL022_SR_RNE        (1u<<2) /**< RX FIFO not empty              */
 #define PL022_SR_RFF        (1u<<3) /**< RX FIFO full                   */
 #define PL022_SR_BSY        (1u<<4) /**< Busy                           */
 
 /* =========================================================================
  * QEMU virt SPI addresses
  * ====================================================================== */
 
 #define UIOX_SPI_ARM64_BASE  0x09050000u  /**< PL022 on QEMU virt       */
 #define UIOX_SPI_ARM32_BASE  0x10115000u  /**< versatilepb SSP          */
 #define UIOX_SPI_ARM32_CLK   24000000u
 #define UIOX_SPI_ARM64_CLK   24000000u
 
 /* =========================================================================
  * SPI statistics
  * ====================================================================== */
 
 typedef struct {
     uint64_t bytes_tx;
     uint64_t bytes_rx;
     uint32_t transfers;
     uint32_t errors;
 } uiox_spi_stats_t;
 
 /* =========================================================================
  * SPI device context
  * ====================================================================== */
 
 #define UIOX_SPI_MODEL_LEN  32u
 #define UIOX_SPI_CS_MAX      4u  /**< Max chip-select lines             */
 
 typedef struct {
     uintptr_t        base;
     uint32_t         clk_hz;
     uint32_t         speed_hz;
     uiox_spi_mode_t  mode;
     uiox_spi_bits_t  bits;
     uint32_t         irq;
     uint32_t         cs_gpio[UIOX_SPI_CS_MAX]; /**< CS GPIO pin IDs    */
     uint32_t         num_cs;
     char             model[UIOX_SPI_MODEL_LEN];
     uiox_spi_stats_t stats;
     bool             initialized;
     void            *priv;
 } uiox_spi_dev_t;
 
 /* =========================================================================
  * HAL operations vtable
  * ====================================================================== */
 
 typedef struct {
     uiox_fw_err_t (*init)      (uiox_spi_dev_t *dev,
                                   uiox_spi_mode_t mode,
                                   uiox_spi_bits_t bits,
                                   uint32_t speed_hz);
     void          (*deinit)    (uiox_spi_dev_t *dev);
     void          (*cs_assert) (uiox_spi_dev_t *dev, uint32_t cs);
     void          (*cs_release)(uiox_spi_dev_t *dev, uint32_t cs);
     uiox_fw_err_t (*transfer)  (uiox_spi_dev_t *dev,
                                   const uint8_t *tx, uint8_t *rx,
                                   uint32_t len);
     uiox_fw_err_t (*write)     (uiox_spi_dev_t *dev,
                                   const uint8_t *buf, uint32_t len);
     uiox_fw_err_t (*read)      (uiox_spi_dev_t *dev,
                                   uint8_t *buf, uint32_t len);
     void          (*isr)       (uiox_spi_dev_t *dev);
 } uiox_spi_ops_t;
 
 /* =========================================================================
  * SPI HAL public API
  * ====================================================================== */
 
 uiox_fw_err_t uiox_fw_spi_init      (uiox_spi_dev_t *dev,
                                        const uiox_spi_ops_t *ops,
                                        uiox_spi_mode_t mode,
                                        uiox_spi_bits_t bits,
                                        uint32_t speed_hz);
 void          uiox_fw_spi_deinit     (uiox_spi_dev_t *dev);
 void          uiox_fw_spi_cs_assert  (uiox_spi_dev_t *dev, uint32_t cs);
 void          uiox_fw_spi_cs_release (uiox_spi_dev_t *dev, uint32_t cs);
 uiox_fw_err_t uiox_fw_spi_transfer  (uiox_spi_dev_t *dev,
                                        const uint8_t *tx, uint8_t *rx,
                                        uint32_t len);
 uiox_fw_err_t uiox_fw_spi_write     (uiox_spi_dev_t *dev,
                                        const uint8_t *buf, uint32_t len);
 uiox_fw_err_t uiox_fw_spi_read      (uiox_spi_dev_t *dev,
                                        uint8_t *buf, uint32_t len);
 
 uiox_fw_err_t uiox_fw_spi_init_pl022(uiox_spi_dev_t *dev,
                                        uintptr_t base, uint32_t clk_hz,
                                        uint32_t irq);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_SPI_H */
 