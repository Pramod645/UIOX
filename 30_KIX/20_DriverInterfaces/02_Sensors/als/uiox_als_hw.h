/**
 * @file  uiox_als_hw.h
 * @brief UIOX Ambient Light Sensor Hardware Abstraction Layer (HAL).
 *
 * Supports:
 *   - Vishay VEML7700   (I²C, 16-bit ALS + white channel, INT)
 *   - TI OPT3001        (I²C, 12-bit mantissa/exp, INT)
 *   - ROHM BH1750       (I²C, 16-bit, no INT)
 *   - AMS TSL2591       (I²C, dual channel CH0/CH1, INT)
 *
 * Owns:
 *   - I²C register read/write (16-bit register values, little-endian)
 *   - GPIO: INT# (interrupt, active-low)
 *   - Gain and integration-time register programming
 *   - ALS + white/IR raw count readback
 *   - High/low threshold window interrupt configuration
 *   - Power-on / shutdown / continuous vs. one-shot modes
 *
 * @version 1.0.0
 * @date    2026-06-11
 */

 #ifndef UIOX_ALS_HW_H
 #define UIOX_ALS_HW_H
 
#include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Sensor IC variant
  * ====================================================================== */
 
 typedef enum {
     UIOX_ALS_IC_VEML7700  = 0,  /**< Vishay VEML7700 (ALS + white)       */
     UIOX_ALS_IC_OPT3001,         /**< TI OPT3001 (auto-range)             */
     UIOX_ALS_IC_BH1750,          /**< ROHM BH1750 (simple, no INT)        */
     UIOX_ALS_IC_TSL2591,         /**< AMS TSL2591 (dual CH0/CH1, IR)      */
 } uiox_als_ic_t;
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_ALS_CAP_ALS_CH         (1u << 0)  /**< Visible ALS channel   */
 #define UIOX_ALS_CAP_WHITE_CH       (1u << 1)  /**< White / broadband ch  */
 #define UIOX_ALS_CAP_IR_CH          (1u << 2)  /**< IR channel            */
 #define UIOX_ALS_CAP_GAIN_CTRL      (1u << 3)  /**< Programmable gain     */
 #define UIOX_ALS_CAP_ITIME_CTRL     (1u << 4)  /**< Integration time ctrl */
 #define UIOX_ALS_CAP_THRESHOLD_INT  (1u << 5)  /**< Window threshold IRQ  */
 #define UIOX_ALS_CAP_PERSIST_FILTER (1u << 6)  /**< Persistence filter    */
 #define UIOX_ALS_CAP_POWER_SAVE     (1u << 7)  /**< Power-save / sleep    */
 #define UIOX_ALS_CAP_ONE_SHOT       (1u << 8)  /**< One-shot measurement  */
 #define UIOX_ALS_CAP_AUTO_GAIN      (1u << 9)  /**< HW auto-gain          */
 #define UIOX_ALS_CAP_LUX_FORMULA    (1u << 10) /**< On-chip lux output    */
 
 /* =========================================================================
  * VEML7700 register map (representative)
  * ====================================================================== */
 
 #define VEML7700_I2C_ADDR           0x10u
 
 #define VEML7700_REG_ALS_CONF       0x00u  /**< Config: gain, itime, int  */
 #define VEML7700_REG_ALS_WH         0x01u  /**< High threshold window     */
 #define VEML7700_REG_ALS_WL         0x02u  /**< Low  threshold window     */
 #define VEML7700_REG_PWR_SAVE       0x03u  /**< Power-save mode           */
 #define VEML7700_REG_ALS            0x04u  /**< ALS channel output (lsb)  */
 #define VEML7700_REG_WHITE          0x05u  /**< White channel output      */
 #define VEML7700_REG_ALS_INT        0x06u  /**< Interrupt status          */
 
 /* ALS_CONF bit fields */
 #define VEML7700_GAIN_SHIFT         11u
 #define VEML7700_GAIN_MASK          (0x03u << VEML7700_GAIN_SHIFT)
 #define VEML7700_GAIN_1X            (0x00u << VEML7700_GAIN_SHIFT)
 #define VEML7700_GAIN_2X            (0x01u << VEML7700_GAIN_SHIFT)
 #define VEML7700_GAIN_1_8X          (0x02u << VEML7700_GAIN_SHIFT)
 #define VEML7700_GAIN_1_4X          (0x03u << VEML7700_GAIN_SHIFT)
 
 #define VEML7700_ITIME_SHIFT        6u
 #define VEML7700_ITIME_MASK         (0x0Fu << VEML7700_ITIME_SHIFT)
 #define VEML7700_ITIME_25MS         (0x0Cu << VEML7700_ITIME_SHIFT)
 #define VEML7700_ITIME_50MS         (0x08u << VEML7700_ITIME_SHIFT)
 #define VEML7700_ITIME_100MS        (0x00u << VEML7700_ITIME_SHIFT)
 #define VEML7700_ITIME_200MS        (0x01u << VEML7700_ITIME_SHIFT)
 #define VEML7700_ITIME_400MS        (0x02u << VEML7700_ITIME_SHIFT)
 #define VEML7700_ITIME_800MS        (0x03u << VEML7700_ITIME_SHIFT)
 
 #define VEML7700_PERS_SHIFT         4u
 #define VEML7700_PERS_MASK          (0x03u << VEML7700_PERS_SHIFT)
 #define VEML7700_INT_EN             (1u << 1u)
 #define VEML7700_SD                 (1u << 0u)  /**< Shutdown              */
 
 /* ALS_INT bits */
 #define VEML7700_INT_TH_HIGH        (1u << 15u) /**< High threshold cross  */
 #define VEML7700_INT_TH_LOW         (1u << 14u) /**< Low  threshold cross  */
 
 /* =========================================================================
  * OPT3001 register map
  * ====================================================================== */
 
 #define OPT3001_I2C_ADDR            0x44u
 
 #define OPT3001_REG_RESULT          0x00u
 #define OPT3001_REG_CONFIG          0x01u
 #define OPT3001_REG_LIMIT_LOW       0x02u
 #define OPT3001_REG_LIMIT_HIGH      0x03u
 #define OPT3001_REG_MFR_ID          0x7Eu
 #define OPT3001_REG_DEVICE_ID       0x7Fu
 
 #define OPT3001_CFG_RANGE_AUTO      (0x0Cu << 12u)
 #define OPT3001_CFG_CT_100MS        (0u << 11u)
 #define OPT3001_CFG_CT_800MS        (1u << 11u)
 #define OPT3001_CFG_MODE_SINGLE     (0x01u << 9u)
 #define OPT3001_CFG_MODE_CONT       (0x02u << 9u)
 #define OPT3001_CFG_MODE_OFF        (0x00u << 9u)
 #define OPT3001_CFG_INT_EN          (1u << 2u)
 #define OPT3001_CFG_FLAG_HIGH       (1u << 5u)
 #define OPT3001_CFG_FLAG_LOW        (1u << 4u)
 
 /* =========================================================================
  * Gain enumeration (IC-agnostic)
  * ====================================================================== */
 
 typedef enum {
     UIOX_ALS_GAIN_1_8X = 0,  /**< 1/8×  (high brightness)               */
     UIOX_ALS_GAIN_1_4X,       /**< 1/4×                                  */
     UIOX_ALS_GAIN_1X,         /**< 1×    (default)                       */
     UIOX_ALS_GAIN_2X,         /**< 2×                                    */
     UIOX_ALS_GAIN_8X,         /**< 8×                                    */
     UIOX_ALS_GAIN_16X,        /**< 16×                                   */
     UIOX_ALS_GAIN_48X,        /**< 48×   (low light)                     */
     UIOX_ALS_GAIN_MAX,
 } uiox_als_gain_t;
 
 /* =========================================================================
  * Integration time enumeration (IC-agnostic)
  * ====================================================================== */
 
 typedef enum {
     UIOX_ALS_ITIME_25MS  = 0,
     UIOX_ALS_ITIME_50MS,
     UIOX_ALS_ITIME_100MS,     /**< Default                               */
     UIOX_ALS_ITIME_200MS,
     UIOX_ALS_ITIME_400MS,
     UIOX_ALS_ITIME_800MS,
     UIOX_ALS_ITIME_MAX,
 } uiox_als_itime_t;
 
 /* Integration time in milliseconds (lookup table index = uiox_als_itime_t) */
 static const uint16_t UIOX_ALS_ITIME_MS[UIOX_ALS_ITIME_MAX] = {
     25u, 50u, 100u, 200u, 400u, 800u
 };
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 #define UIOX_ALS_MODEL_LEN          48u
 
 typedef struct {
     uint8_t          i2c_addr;
     uint32_t         i2c_bus;
     uint32_t         irq;           /**< Platform IRQ (INT# pin)          */
     uint32_t         caps;
     uiox_als_ic_t    ic_type;
     char             model[UIOX_ALS_MODEL_LEN];
     /* Current hardware settings */
     uiox_als_gain_t  gain;
     uiox_als_itime_t itime;
     /* Raw ADC counts (last measurement) */
     uint16_t         raw_als;
     uint16_t         raw_white;
     uint16_t         raw_ir;
     /* Threshold registers (raw counts) */
     uint16_t         thresh_high;
     uint16_t         thresh_low;
     /* Pending IRQ */
     volatile uint32_t pending_irq;
     /* Private (ops vtable) */
     void            *priv;
 } uiox_als_hw_t;
 
 /* Pending IRQ bits */
 #define UIOX_ALS_IRQ_DATA_READY     (1u << 0)
 #define UIOX_ALS_IRQ_THRESH_HIGH    (1u << 1)
 #define UIOX_ALS_IRQ_THRESH_LOW     (1u << 2)
 
 /* =========================================================================
  * Hardware operations vtable  (16-op table)
  * ====================================================================== */
 
 typedef struct {
     /* Lifecycle */
     int  (*init)          (uiox_als_hw_t *hw);
     void (*deinit)        (uiox_als_hw_t *hw);
     int  (*power_on)      (uiox_als_hw_t *hw);
     void (*power_off)     (uiox_als_hw_t *hw);
 
     /* Raw register access (16-bit values, little-endian on wire) */
     int  (*reg_read)      (uiox_als_hw_t *hw,
                            uint8_t reg, uint16_t *val);
     int  (*reg_write)     (uiox_als_hw_t *hw,
                            uint8_t reg, uint16_t  val);
 
     /* Measurement configuration */
     int  (*set_gain)      (uiox_als_hw_t *hw, uiox_als_gain_t gain);
     int  (*set_itime)     (uiox_als_hw_t *hw, uiox_als_itime_t itime);
 
     /* Data fetch */
     int  (*read_als)      (uiox_als_hw_t *hw,
                            uint16_t *als, uint16_t *white);
     int  (*read_ir)       (uiox_als_hw_t *hw, uint16_t *ir);
 
     /* Threshold / interrupt */
     int  (*set_threshold) (uiox_als_hw_t *hw,
                            uint16_t low, uint16_t high);
     int  (*int_enable)    (uiox_als_hw_t *hw, bool en);
     int  (*int_clear)     (uiox_als_hw_t *hw);
 
     /* One-shot trigger */
     int  (*trigger)       (uiox_als_hw_t *hw);
 
     /* GPIO */
     void (*gpio_write)    (uiox_als_hw_t *hw, uint32_t pin, bool val);
     bool (*gpio_read)     (uiox_als_hw_t *hw, uint32_t pin);
 
     /* ISR */
     void (*isr)           (uiox_als_hw_t *hw);
 } uiox_als_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int  uiox_als_hw_init         (uiox_als_hw_t *hw,
                                 const uiox_als_hw_ops_t *ops);
 void uiox_als_hw_deinit       (uiox_als_hw_t *hw);
 int  uiox_als_hw_power_on     (uiox_als_hw_t *hw);
 void uiox_als_hw_power_off    (uiox_als_hw_t *hw);
 int  uiox_als_hw_reg_read     (uiox_als_hw_t *hw,
                                 uint8_t reg, uint16_t *val);
 int  uiox_als_hw_reg_write    (uiox_als_hw_t *hw,
                                 uint8_t reg, uint16_t  val);
 int  uiox_als_hw_set_gain     (uiox_als_hw_t *hw, uiox_als_gain_t g);
 int  uiox_als_hw_set_itime    (uiox_als_hw_t *hw, uiox_als_itime_t t);
 int  uiox_als_hw_read_als     (uiox_als_hw_t *hw,
                                 uint16_t *als, uint16_t *white);
 int  uiox_als_hw_read_ir      (uiox_als_hw_t *hw, uint16_t *ir);
 int  uiox_als_hw_set_threshold(uiox_als_hw_t *hw,
                                 uint16_t low, uint16_t high);
 int  uiox_als_hw_int_enable   (uiox_als_hw_t *hw, bool en);
 int  uiox_als_hw_int_clear    (uiox_als_hw_t *hw);
 int  uiox_als_hw_trigger      (uiox_als_hw_t *hw);
 
 static inline uint32_t uiox_als_caps(const uiox_als_hw_t *hw)
 { return hw ? hw->caps : 0u; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_ALS_HW_H */
 