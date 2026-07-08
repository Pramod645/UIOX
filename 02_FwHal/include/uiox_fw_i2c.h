/**
 * @file  uiox_fw_i2c.h
 * @brief UIOX Firmware HAL — I2C master controller.
 *
 * Supports:
 *   ARM64/ARM32 — ARM PrimeCell PL031 / Synopsys DesignWare I2C
 *   x86_64      — Intel ICH/PCH SMBus (port 0xEFA0 on QEMU)
 *
 * Used by: RTC (CR2032), ALS sensors, PMIC, temperature sensors,
 *          touch controllers, audio codecs.
 *
 * Integrates with: uiox_fw_sensor.h, 30_DeviceDrivers/03_Sensors
 *
 * @version 1.0.0
 * @date    2026-07-07
 */

 #ifndef UIOX_FW_I2C_H
 #define UIOX_FW_I2C_H
 
 #include "uiox_fw_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * I2C speed modes
  * ====================================================================== */
 
 typedef enum {
     UIOX_I2C_SPEED_STANDARD  = 100000u,   /**< 100 kHz standard mode   */
     UIOX_I2C_SPEED_FAST      = 400000u,   /**< 400 kHz fast mode        */
     UIOX_I2C_SPEED_FAST_PLUS = 1000000u,  /**< 1 MHz fast-plus mode     */
     UIOX_I2C_SPEED_HIGH      = 3400000u,  /**< 3.4 MHz high-speed       */
 } uiox_i2c_speed_t;
 
 /* =========================================================================
  * I2C capability flags
  * ====================================================================== */
 
 #define UIOX_I2C_CAP_10BIT_ADDR    (1u << 0)  /**< 10-bit addressing    */
 #define UIOX_I2C_CAP_DMA           (1u << 1)  /**< DMA transfers        */
 #define UIOX_I2C_CAP_SMBUS         (1u << 2)  /**< SMBus compatible      */
 #define UIOX_I2C_CAP_MULTI_MASTER  (1u << 3)  /**< Multi-master support  */
 
 /* =========================================================================
  * DesignWare I2C (DW APB) register offsets — used on ARM QEMU virt
  * ====================================================================== */
 
 #define DW_I2C_CON              0x000u  /**< Control register             */
 #define DW_I2C_TAR              0x004u  /**< Target address               */
 #define DW_I2C_SAR              0x008u  /**< Slave address                */
 #define DW_I2C_DATA_CMD         0x010u  /**< Rx/Tx data + command         */
 #define DW_I2C_SS_SCL_HCNT     0x014u  /**< Standard speed SCL high cnt  */
 #define DW_I2C_SS_SCL_LCNT     0x018u  /**< Standard speed SCL low cnt   */
 #define DW_I2C_FS_SCL_HCNT     0x01Cu  /**< Fast speed SCL high cnt      */
 #define DW_I2C_FS_SCL_LCNT     0x020u  /**< Fast speed SCL low cnt       */
 #define DW_I2C_INTR_STAT        0x02Cu  /**< Interrupt status (RO)        */
 #define DW_I2C_INTR_MASK        0x030u  /**< Interrupt mask               */
 #define DW_I2C_RAW_INTR_STAT   0x034u  /**< Raw interrupt status         */
 #define DW_I2C_RX_TL            0x038u  /**< RX FIFO threshold level      */
 #define DW_I2C_TX_TL            0x03Cu  /**< TX FIFO threshold level      */
 #define DW_I2C_CLR_INTR         0x040u  /**< Clear all interrupts         */
 #define DW_I2C_CLR_RX_UNDER    0x044u
 #define DW_I2C_CLR_RX_OVER     0x048u
 #define DW_I2C_CLR_TX_OVER     0x04Cu
 #define DW_I2C_CLR_RD_REQ      0x050u
 #define DW_I2C_CLR_TX_ABRT     0x054u
 #define DW_I2C_CLR_ACTIVITY    0x05Cu
 #define DW_I2C_CLR_STOP_DET    0x060u
 #define DW_I2C_CLR_START_DET   0x064u
 #define DW_I2C_ENABLE           0x06Cu  /**< Enable register              */
 #define DW_I2C_STATUS           0x070u  /**< Status register              */
 #define DW_I2C_TXFLR            0x074u  /**< TX FIFO level                */
 #define DW_I2C_RXFLR            0x078u  /**< RX FIFO level                */
 #define DW_I2C_SDA_HOLD         0x07Cu
 #define DW_I2C_TX_ABRT_SRC     0x080u  /**< TX abort source              */
 #define DW_I2C_COMP_PARAM1     0x0F4u  /**< Component parameters         */
 #define DW_I2C_COMP_VERSION    0x0F8u
 #define DW_I2C_COMP_TYPE       0x0FCu
 
 /* CON register bits */
 #define DW_I2C_CON_MASTER         (1u << 0)
 #define DW_I2C_CON_SPEED_STD      (1u << 1)
 #define DW_I2C_CON_SPEED_FAST     (2u << 1)
 #define DW_I2C_CON_SPEED_HIGH     (3u << 1)
 #define DW_I2C_CON_10BIT_ADDR     (1u << 4)
 #define DW_I2C_CON_RESTART_EN     (1u << 5)
 #define DW_I2C_CON_SLAVE_DISABLE  (1u << 6)
 
 /* DATA_CMD bits */
 #define DW_I2C_CMD_READ           (1u << 8)
 #define DW_I2C_CMD_STOP           (1u << 9)
 #define DW_I2C_CMD_RESTART        (1u << 10)
 
 /* STATUS bits */
 #define DW_I2C_STATUS_ACTIVITY    (1u << 0)
 #define DW_I2C_STATUS_TFNF        (1u << 1)  /**< TX FIFO not full       */
 #define DW_I2C_STATUS_TFE         (1u << 2)  /**< TX FIFO empty          */
 #define DW_I2C_STATUS_RFNE        (1u << 3)  /**< RX FIFO not empty      */
 #define DW_I2C_STATUS_RFF         (1u << 4)  /**< RX FIFO full           */
 #define DW_I2C_STATUS_MST_ACTV    (1u << 5)  /**< Master activity        */
 
 /* INTR bits */
 #define DW_I2C_INTR_RX_UNDER      (1u << 0)
 #define DW_I2C_INTR_RX_OVER       (1u << 1)
 #define DW_I2C_INTR_RX_FULL       (1u << 2)
 #define DW_I2C_INTR_TX_OVER       (1u << 3)
 #define DW_I2C_INTR_TX_EMPTY      (1u << 4)
 #define DW_I2C_INTR_RD_REQ        (1u << 5)
 #define DW_I2C_INTR_TX_ABRT       (1u << 6)
 #define DW_I2C_INTR_RX_DONE       (1u << 7)
 #define DW_I2C_INTR_ACTIVITY      (1u << 8)
 #define DW_I2C_INTR_STOP_DET      (1u << 9)
 #define DW_I2C_INTR_START_DET     (1u << 10)
 #define DW_I2C_INTR_GEN_CALL      (1u << 11)
 
 /* =========================================================================
  * QEMU virt I2C base addresses
  * ====================================================================== */
 
 #define UIOX_I2C_ARM64_BASE     0x09040000u  /**< DW I2C on QEMU virt    */
 #define UIOX_I2C_ARM32_BASE     0x10002000u  /**< versatilepb I2C        */
 #define UIOX_I2C_X86_PORT       0xEFA0u      /**< ICH SMBus I/O port     */
 
 /* =========================================================================
  * Transfer descriptor
  * ====================================================================== */
 
 typedef struct {
     uint8_t   addr;         /**< 7-bit slave address                    */
     bool      addr_10bit;   /**< True = 10-bit addressing               */
     uint8_t  *tx_buf;       /**< Transmit data (NULL = read only)       */
     uint32_t  tx_len;
     uint8_t  *rx_buf;       /**< Receive buffer  (NULL = write only)    */
     uint32_t  rx_len;
     bool      repeated_start; /**< Keep bus after TX (for reg-then-read) */
 } uiox_i2c_xfer_t;
 
 /* =========================================================================
  * I2C statistics
  * ====================================================================== */
 
 typedef struct {
     uint64_t  bytes_tx;
     uint64_t  bytes_rx;
     uint32_t  transfers;
     uint32_t  nack_errors;
     uint32_t  arb_lost;
     uint32_t  timeout_errors;
 } uiox_i2c_stats_t;
 
 /* =========================================================================
  * I2C device context
  * ====================================================================== */
 
 #define UIOX_I2C_MODEL_LEN   32u
 
 typedef struct {
     uintptr_t        base;
     uint32_t         irq;
     uint32_t         clk_hz;     /**< Input clock Hz (e.g. 100 MHz)    */
     uiox_i2c_speed_t speed;
     uint32_t         caps;
     char             model[UIOX_I2C_MODEL_LEN];
     uiox_i2c_stats_t stats;
     bool             initialized;
     void            *priv;
 } uiox_i2c_dev_t;
 
 /* =========================================================================
  * HAL operations vtable
  * ====================================================================== */
 
 typedef struct {
     uiox_fw_err_t (*init)       (uiox_i2c_dev_t *dev,
                                   uiox_i2c_speed_t speed);
     void          (*deinit)     (uiox_i2c_dev_t *dev);
     uiox_fw_err_t (*transfer)   (uiox_i2c_dev_t *dev,
                                   uiox_i2c_xfer_t *xfer);
     uiox_fw_err_t (*write_reg)  (uiox_i2c_dev_t *dev,
                                   uint8_t addr, uint8_t reg,
                                   const uint8_t *buf, uint32_t len);
     uiox_fw_err_t (*read_reg)   (uiox_i2c_dev_t *dev,
                                   uint8_t addr, uint8_t reg,
                                   uint8_t *buf, uint32_t len);
     bool          (*bus_busy)   (uiox_i2c_dev_t *dev);
     uiox_fw_err_t (*recover_bus)(uiox_i2c_dev_t *dev);
     void          (*isr)        (uiox_i2c_dev_t *dev);
 } uiox_i2c_ops_t;
 
 /* =========================================================================
  * I2C HAL public API
  * ====================================================================== */
 
 uiox_fw_err_t uiox_fw_i2c_init      (uiox_i2c_dev_t *dev,
                                        const uiox_i2c_ops_t *ops,
                                        uiox_i2c_speed_t speed);
 void          uiox_fw_i2c_deinit     (uiox_i2c_dev_t *dev);
 uiox_fw_err_t uiox_fw_i2c_transfer  (uiox_i2c_dev_t *dev,
                                        uiox_i2c_xfer_t *xfer);
 uiox_fw_err_t uiox_fw_i2c_write_reg (uiox_i2c_dev_t *dev,
                                        uint8_t addr, uint8_t reg,
                                        const uint8_t *buf, uint32_t len);
 uiox_fw_err_t uiox_fw_i2c_read_reg  (uiox_i2c_dev_t *dev,
                                        uint8_t addr, uint8_t reg,
                                        uint8_t *buf, uint32_t len);
 bool          uiox_fw_i2c_bus_busy   (uiox_i2c_dev_t *dev);
 uiox_fw_err_t uiox_fw_i2c_recover   (uiox_i2c_dev_t *dev);
 void          uiox_fw_i2c_stats_get (const uiox_i2c_dev_t *dev,
                                        uiox_i2c_stats_t *out);
 
 /* Platform init helpers */
 uiox_fw_err_t uiox_fw_i2c_init_dw   (uiox_i2c_dev_t *dev,
                                        uintptr_t base, uint32_t clk_hz,
                                        uint32_t irq,
                                        uiox_i2c_speed_t speed);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_I2C_H */
 