/**
 * @file  uiox_rtc_hw.h
 * @brief UIOX RTC Hardware Abstraction Layer (HAL).
 *
 * Supports:
 *   - MC146818A (PC CMOS RTC, backed by CR2032 coin cell)
 *   - Intel / AMD chipset-integrated CMOS RTC
 *   - Any MC146818-compatible controller
 *
 * Owns:
 *   - Port I/O access to CMOS index/data ports (0x70/0x71)
 *   - Register address map and bit definitions
 *   - BCD <-> binary conversion
 *   - Battery status (Register D VRT bit)
 *   - GPIO: FOUT (frequency output), INT (IRQ pin)
 *   - NMI-safe register access
 *
 * @version 1.0.0
 * @date    2026-06-10
 */

 #ifndef UIOX_RTC_HW_H
 #define UIOX_RTC_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * RTC version / chip variant
  * ====================================================================== */
 
 typedef enum {
     UIOX_RTC_VER_MC146818  = 0,  /**< Original MC146818A                 */
     UIOX_RTC_VER_PIIX4,           /**< Intel PIIX4 integrated RTC         */
     UIOX_RTC_VER_ICH,             /**< Intel ICH/PCH RTC                  */
     UIOX_RTC_VER_AMD_FCH,         /**< AMD FCH RTC                        */
 } uiox_rtc_ver_t;
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_RTC_CAP_BCD            (1u << 0)  /**< BCD time registers    */
 #define UIOX_RTC_CAP_BINARY         (1u << 1)  /**< Binary time registers */
 #define UIOX_RTC_CAP_24H            (1u << 2)  /**< 24-hour clock         */
 #define UIOX_RTC_CAP_12H            (1u << 3)  /**< 12-hour clock         */
 #define UIOX_RTC_CAP_ALARM          (1u << 4)  /**< Alarm interrupt        */
 #define UIOX_RTC_CAP_PERIODIC       (1u << 5)  /**< Periodic interrupt     */
 #define UIOX_RTC_CAP_UPDATE_IRQ     (1u << 6)  /**< Update-ended IRQ       */
 #define UIOX_RTC_CAP_CENTURY_REG    (1u << 7)  /**< Century CMOS byte      */
 #define UIOX_RTC_CAP_NVRAM          (1u << 8)  /**< 114 bytes NVRAM        */
 #define UIOX_RTC_CAP_SQWAVE         (1u << 9)  /**< Square-wave output     */
 #define UIOX_RTC_CAP_BATTERY        (1u << 10) /**< CR2032 battery status  */
 #define UIOX_RTC_CAP_NMI_DISABLE    (1u << 11) /**< NMI masking on access  */
 #define UIOX_RTC_CAP_DSE            (1u << 12) /**< Daylight saving        */
 
 /* =========================================================================
  * CMOS register map (MC146818A)
  * ====================================================================== */
 
 /* I/O ports */
 #define RTC_PORT_INDEX              0x70u  /**< CMOS index / NMI mask port */
 #define RTC_PORT_DATA               0x71u  /**< CMOS data port             */
 #define RTC_NMI_DISABLE_BIT         0x80u  /**< Bit 7 of index = NMI off   */
 
 /* Time / date registers */
 #define RTC_REG_SECONDS             0x00u
 #define RTC_REG_SEC_ALARM           0x01u
 #define RTC_REG_MINUTES             0x02u
 #define RTC_REG_MIN_ALARM           0x03u
 #define RTC_REG_HOURS               0x04u
 #define RTC_REG_HR_ALARM            0x05u
 #define RTC_REG_WEEKDAY             0x06u
 #define RTC_REG_DAY                 0x07u
 #define RTC_REG_MONTH               0x08u
 #define RTC_REG_YEAR                0x09u
 
 /* Control registers */
 #define RTC_REG_A                   0x0Au
 #define RTC_REG_B                   0x0Bu
 #define RTC_REG_C                   0x0Cu  /**< Read-only; clears on read  */
 #define RTC_REG_D                   0x0Du  /**< Read-only; VRT bit         */
 
 /* Register A bits */
 #define RTC_REG_A_UIP               0x80u  /**< Update-in-progress         */
 #define RTC_REG_A_DV_MASK           0x70u  /**< Oscillator divider bits    */
 #define RTC_REG_A_DV_ON             0x20u  /**< 32.768 kHz crystal on      */
 #define RTC_REG_A_RS_MASK           0x0Fu  /**< Rate select (periodic IRQ) */
 #define RTC_REG_A_RS_NONE           0x00u  /**< No periodic interrupt      */
 #define RTC_REG_A_RS_1HZ            0x06u  /**< 1 Hz                       */
 #define RTC_REG_A_RS_2HZ            0x07u
 #define RTC_REG_A_RS_4HZ            0x08u
 #define RTC_REG_A_RS_8192HZ         0x03u  /**< 8192 Hz (max)              */
 
 /* Register B bits */
 #define RTC_REG_B_SET               0x80u  /**< Halt update cycle (for set)*/
 #define RTC_REG_B_PIE               0x40u  /**< Periodic interrupt enable  */
 #define RTC_REG_B_AIE               0x20u  /**< Alarm interrupt enable     */
 #define RTC_REG_B_UIE               0x10u  /**< Update-ended IRQ enable    */
 #define RTC_REG_B_SQWE              0x08u  /**< Square-wave output enable  */
 #define RTC_REG_B_DM                0x04u  /**< Data mode: 1=binary, 0=BCD */
 #define RTC_REG_B_24H               0x02u  /**< Hour: 1=24h, 0=12h         */
 #define RTC_REG_B_DSE               0x01u  /**< Daylight saving enable     */
 
 /* Register C interrupt flags (cleared on read) */
 #define RTC_REG_C_IRQF              0x80u  /**< Any interrupt pending      */
 #define RTC_REG_C_PF                0x40u  /**< Periodic flag              */
 #define RTC_REG_C_AF                0x20u  /**< Alarm flag                 */
 #define RTC_REG_C_UF                0x10u  /**< Update-ended flag          */
 
 /* Register D */
 #define RTC_REG_D_VRT               0x80u  /**< Valid RAM & Time (batt OK) */
 
 /* Extended CMOS */
 #define RTC_REG_CENTURY             0x32u  /**< Century (ACPI/BIOS ext)    */
 #define RTC_NVRAM_START             0x0Eu  /**< First NVRAM byte           */
 #define RTC_NVRAM_LEN               114u   /**< Bytes 0x0E–0x7F            */
 #define RTC_CMOS_SIZE               128u   /**< Total CMOS bytes           */
 
 /* IRQ */
 #define RTC_IRQ_LINE                8u     /**< x86 legacy IRQ 8           */
 
 /* =========================================================================
  * Battery state
  * ====================================================================== */
 
 typedef enum {
     UIOX_RTC_BAT_GOOD    = 0,  /**< VRT set; CR2032 voltage OK (~3 V)    */
     UIOX_RTC_BAT_LOW     = 1,  /**< VRT cleared; time may be lost        */
     UIOX_RTC_BAT_UNKNOWN = 2,  /**< Not yet sampled                       */
 } uiox_rtc_bat_t;
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 #define UIOX_RTC_MODEL_LEN   48u
 
 typedef struct {
     uint16_t        index_port;    /**< CMOS index port (default 0x70)    */
     uint16_t        data_port;     /**< CMOS data port  (default 0x71)    */
     uint32_t        irq;           /**< IRQ line (default 8)              */
     uint32_t        caps;          /**< Capability flags                  */
     uiox_rtc_ver_t  version;
     char            model[UIOX_RTC_MODEL_LEN];
     uint8_t         century_reg;   /**< Century CMOS offset (0 = none)    */
     bool            use_nmi_mask;  /**< Assert NMI-disable on port writes */
     bool            use_bcd;       /**< BCD mode (default true)           */
     bool            use_24h;       /**< 24-hour mode (default true)       */
     /* Battery */
     uiox_rtc_bat_t  bat_state;
     /* IRQ status */
     volatile uint32_t pending_irq; /**< Bitmask of pending IRQ flags      */
     /* Private (ops pointer) */
     void           *priv;
 } uiox_rtc_hw_t;
 
 /* Pending IRQ bits (mirrors Register C) */
 #define UIOX_RTC_IRQ_ALARM          RTC_REG_C_AF
 #define UIOX_RTC_IRQ_PERIODIC       RTC_REG_C_PF
 #define UIOX_RTC_IRQ_UPDATE         RTC_REG_C_UF
 
 /* =========================================================================
  * Hardware operations vtable  (16-op table)
  * ====================================================================== */
 
 typedef struct {
     /* Lifecycle */
     int  (*init)         (uiox_rtc_hw_t *hw);
     void (*deinit)       (uiox_rtc_hw_t *hw);
 
     /* Raw CMOS register access */
     uint8_t (*reg_read)  (uiox_rtc_hw_t *hw, uint8_t reg);
     void    (*reg_write) (uiox_rtc_hw_t *hw, uint8_t reg, uint8_t val);
 
     /* Battery */
     uiox_rtc_bat_t (*bat_check)(uiox_rtc_hw_t *hw);
 
     /* Time */
     int  (*time_read)    (uiox_rtc_hw_t *hw,
                           uint8_t *sec, uint8_t *min, uint8_t *hr,
                           uint8_t *mday, uint8_t *mon, uint16_t *year);
     int  (*time_write)   (uiox_rtc_hw_t *hw,
                           uint8_t sec, uint8_t min, uint8_t hr,
                           uint8_t mday, uint8_t mon, uint16_t year);
 
     /* Alarm */
     int  (*alarm_read)   (uiox_rtc_hw_t *hw,
                           uint8_t *sec, uint8_t *min, uint8_t *hr);
     int  (*alarm_write)  (uiox_rtc_hw_t *hw,
                           uint8_t sec, uint8_t min, uint8_t hr);
     int  (*alarm_enable) (uiox_rtc_hw_t *hw, bool enable);
 
     /* Periodic interrupt */
     int  (*periodic_set) (uiox_rtc_hw_t *hw, uint8_t rate_sel);
 
     /* NVRAM */
     int  (*nvram_read)   (uiox_rtc_hw_t *hw, uint8_t offset, uint8_t *val);
     int  (*nvram_write)  (uiox_rtc_hw_t *hw, uint8_t offset, uint8_t  val);
 
     /* GPIO (FOUT / INT lines) */
     void (*gpio_write)   (uiox_rtc_hw_t *hw, uint32_t pin, bool val);
     bool (*gpio_read)    (uiox_rtc_hw_t *hw, uint32_t pin);
 
     /* ISR */
     void (*isr)          (uiox_rtc_hw_t *hw);
 } uiox_rtc_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int            uiox_rtc_hw_init        (uiox_rtc_hw_t *hw,
                                          const uiox_rtc_hw_ops_t *ops);
 void           uiox_rtc_hw_deinit      (uiox_rtc_hw_t *hw);
 uint8_t        uiox_rtc_hw_reg_read    (uiox_rtc_hw_t *hw, uint8_t reg);
 void           uiox_rtc_hw_reg_write   (uiox_rtc_hw_t *hw,
                                          uint8_t reg, uint8_t val);
 uiox_rtc_bat_t uiox_rtc_hw_bat_check  (uiox_rtc_hw_t *hw);
 int            uiox_rtc_hw_time_read   (uiox_rtc_hw_t *hw,
                                          uint8_t *s, uint8_t *m,
                                          uint8_t *h, uint8_t *md,
                                          uint8_t *mo, uint16_t *yr);
 int            uiox_rtc_hw_time_write  (uiox_rtc_hw_t *hw,
                                          uint8_t s, uint8_t m,
                                          uint8_t h, uint8_t md,
                                          uint8_t mo, uint16_t yr);
 int            uiox_rtc_hw_alarm_read  (uiox_rtc_hw_t *hw,
                                          uint8_t *s, uint8_t *m,
                                          uint8_t *h);
 int            uiox_rtc_hw_alarm_write (uiox_rtc_hw_t *hw,
                                          uint8_t s, uint8_t m, uint8_t h);
 int            uiox_rtc_hw_alarm_enable(uiox_rtc_hw_t *hw, bool en);
 int            uiox_rtc_hw_nvram_read  (uiox_rtc_hw_t *hw,
                                          uint8_t off, uint8_t *val);
 int            uiox_rtc_hw_nvram_write (uiox_rtc_hw_t *hw,
                                          uint8_t off, uint8_t  val);
 
 static inline uint32_t uiox_rtc_caps(const uiox_rtc_hw_t *hw)
 { return hw ? hw->caps : 0u; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_RTC_HW_H */
 