/**
 * @file    uiox_ram_hw.h
 * @brief   UIOX RAM Hardware Abstraction Layer (HAL).
 *
 * Lowest-level interface to RAM controller hardware. Supports:
 *   - LPDDR5 (up to 8533 MT/s, Gear mode, WCK)
 *   - LPDDR4X (up to 4266 MT/s)
 *   - DDR4 (up to 3200 MT/s, ECC DIMM)
 *   - DDR5 (up to 6400 MT/s, on-die ECC)
 *   - SRAM (static, zero-wait-state internal)
 *   - PSRAM / HyperRAM (SPI/HyperBus external)
 *
 * Owns:
 *   - DRAM controller MMIO register access
 *   - PHY / DFI (DDR PHY Interface) initialisation
 *   - Timing parameter programming (tRCD, tRAS, tRP, tCL, etc.)
 *   - Mode register writes (MR0..MR13 for LPDDR5)
 *   - ZQ calibration
 *   - Self-refresh / power-down entry/exit
 *   - ECC engine enable / error IRQ
 *   - Memory-mapped region configuration (base, size, interleave)
 *
 * @version 1.0.0
 * @date    2026-06-03
 */

 #ifndef UIOX_RAM_HW_H
 #define UIOX_RAM_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * RAM type
  * ====================================================================== */
 
 typedef enum {
     UIOX_RAM_TYPE_LPDDR5  = 0,
     UIOX_RAM_TYPE_LPDDR4X,
     UIOX_RAM_TYPE_DDR4,
     UIOX_RAM_TYPE_DDR5,
     UIOX_RAM_TYPE_SRAM,
     UIOX_RAM_TYPE_PSRAM,
 } uiox_ram_type_t;
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_RAM_CAP_ECC            (1u << 0)  /**< Error Correction Code */
 #define UIOX_RAM_CAP_ECC_INLINE     (1u << 1)  /**< On-die / inline ECC   */
 #define UIOX_RAM_CAP_SCRUB          (1u << 2)  /**< Background memory scrub*/
 #define UIOX_RAM_CAP_INTERLEAVE     (1u << 3)  /**< Dual/quad-ch interleave*/
 #define UIOX_RAM_CAP_SELF_REFRESH   (1u << 4)  /**< Self-refresh (SREF)   */
 #define UIOX_RAM_CAP_POWER_DOWN     (1u << 5)  /**< Power-down mode        */
 #define UIOX_RAM_CAP_ZQ_CAL         (1u << 6)  /**< ZQ impedance calibration*/
 #define UIOX_RAM_CAP_BANK_GROUP     (1u << 7)  /**< Bank group (DDR4/5)   */
 #define UIOX_RAM_CAP_WRITE_CRC      (1u << 8)  /**< Write CRC (DDR4/5)    */
 #define UIOX_RAM_CAP_CA_PARITY      (1u << 9)  /**< CA parity (LPDDR4/5)  */
 #define UIOX_RAM_CAP_GEAR_MODE      (1u << 10) /**< Gear mode (LPDDR5)    */
 #define UIOX_RAM_CAP_MPU            (1u << 11) /**< Memory Protection Unit */
 #define UIOX_RAM_CAP_HOT_TEMP       (1u << 12) /**< High-temp refresh rate */
 #define UIOX_RAM_CAP_DBI            (1u << 13) /**< Data Bus Inversion    */
 
 /* =========================================================================
  * RAM timing parameters
  * ====================================================================== */
 
 typedef struct {
     uint16_t  tRCD;     /**< RAS-to-CAS delay (ns × 10)                   */
     uint16_t  tRAS;     /**< Active-to-precharge (ns × 10)                */
     uint16_t  tRP;      /**< Precharge command period (ns × 10)           */
     uint16_t  tCL;      /**< CAS latency (cycles)                         */
     uint16_t  tCWL;     /**< CAS write latency (cycles)                   */
     uint16_t  tRFC;     /**< Refresh-to-activate (ns × 10)               */
     uint16_t  tREFI;    /**< Average periodic refresh interval (ns × 10)  */
     uint16_t  tWR;      /**< Write recovery time (cycles)                 */
     uint16_t  tRTP;     /**< Read-to-precharge (cycles)                   */
     uint16_t  tFAW;     /**< Four-activate window (ns × 10)              */
     uint8_t   tCCD;     /**< Column-to-column delay (cycles)              */
     uint8_t   tRRD;     /**< Row-to-row delay (cycles)                    */
 } uiox_ram_timing_t;
 
 /* =========================================================================
  * RAM power state
  * ====================================================================== */
 
 typedef enum {
     UIOX_RAM_PWR_ACTIVE      = 0,
     UIOX_RAM_PWR_SELF_REFRESH,
     UIOX_RAM_PWR_POWER_DOWN,
     UIOX_RAM_PWR_DEEP_DOWN,
 } uiox_ram_pwr_t;
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 #define UIOX_RAM_MAX_CHANNELS    4
 #define UIOX_RAM_MAX_RANKS       2
 #define UIOX_RAM_MODEL_LEN       32
 
 typedef struct {
     uintptr_t           ctrl_base;   /**< DRAM controller MMIO base        */
     uintptr_t           phy_base;    /**< PHY / DFI MMIO base              */
     uint32_t            irq_ecc;     /**< ECC error IRQ                    */
     uint32_t            irq_parity;  /**< CA parity IRQ                    */
     uint32_t            caps;        /**< UIOX_RAM_CAP_* bitmask          */
     uiox_ram_type_t     type;
     char                model[UIOX_RAM_MODEL_LEN]; /**< e.g. "LPDDR5-8533" */
     uint8_t             num_channels;
     uint8_t             num_ranks;
     uint8_t             bus_width;   /**< Bits: 16, 32, 64, 128            */
     uint8_t             density_gb;  /**< Per-channel capacity (GB)        */
     uint32_t            speed_mtps;  /**< MT/s (e.g. 8533)                */
     uint32_t            ref_clk_mhz; /**< Controller reference clock       */
 
     /* Physical memory layout */
     uint64_t            base_phys;   /**< Physical base address            */
     uint64_t            total_bytes; /**< Total RAM size (bytes)           */
 
     /* Timing */
     uiox_ram_timing_t   timing;
 
     /* State */
     uiox_ram_pwr_t      pwr_state;
     bool                initialised;
     volatile uint32_t   ecc_ce_count; /**< Correctable ECC events         */
     volatile uint32_t   ecc_ue_count; /**< Uncorrectable ECC events       */
 
     void               *priv;
 } uiox_ram_hw_t;
 
 /* =========================================================================
  * Hardware operations vtable
  * ====================================================================== */
 
 typedef struct {
     int  (*init)          (uiox_ram_hw_t *hw);
     void (*deinit)        (uiox_ram_hw_t *hw);
     int  (*train)         (uiox_ram_hw_t *hw);   /**< PHY training sequence */
     int  (*mode_reg_write)(uiox_ram_hw_t *hw,
                            uint8_t mr, uint16_t val);
     int  (*mode_reg_read) (uiox_ram_hw_t *hw,
                            uint8_t mr, uint16_t *val);
     int  (*set_timing)    (uiox_ram_hw_t *hw,
                            const uiox_ram_timing_t *t);
     int  (*zq_calibrate)  (uiox_ram_hw_t *hw);
     int  (*set_power)     (uiox_ram_hw_t *hw, uiox_ram_pwr_t state);
     int  (*ecc_enable)    (uiox_ram_hw_t *hw, bool enable);
     int  (*ecc_scrub)     (uiox_ram_hw_t *hw,
                            uint64_t phys_start, uint64_t size);
     int  (*ecc_status)    (uiox_ram_hw_t *hw,
                            uint32_t *ce_out, uint32_t *ue_out,
                            uint64_t *addr_out);
     int  (*read64)        (uiox_ram_hw_t *hw,
                            uint64_t phys, uint64_t *val);
     int  (*write64)       (uiox_ram_hw_t *hw,
                            uint64_t phys, uint64_t val);
     void (*isr_ecc)       (uiox_ram_hw_t *hw);
     void (*isr_parity)    (uiox_ram_hw_t *hw);
 } uiox_ram_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int  uiox_ram_hw_init       (uiox_ram_hw_t *hw,
                               const uiox_ram_hw_ops_t *ops);
 void uiox_ram_hw_deinit     (uiox_ram_hw_t *hw);
 int  uiox_ram_hw_train      (uiox_ram_hw_t *hw);
 int  uiox_ram_hw_set_power  (uiox_ram_hw_t *hw, uiox_ram_pwr_t state);
 int  uiox_ram_hw_ecc_enable (uiox_ram_hw_t *hw, bool enable);
 int  uiox_ram_hw_ecc_scrub  (uiox_ram_hw_t *hw,
                               uint64_t phys_start, uint64_t size);
 int  uiox_ram_hw_ecc_status (uiox_ram_hw_t *hw,
                               uint32_t *ce, uint32_t *ue, uint64_t *addr);
 int  uiox_ram_hw_zq_cal     (uiox_ram_hw_t *hw);
 
 static inline uint32_t uiox_ram_caps(const uiox_ram_hw_t *hw)
 { return hw ? hw->caps : 0u; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_RAM_HW_H */
 