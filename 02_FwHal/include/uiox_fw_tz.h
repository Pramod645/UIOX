/**
 * @file  uiox_fw_tz.h
 * @brief UIOX Firmware — ARM TrustZone / EL3 secure world setup.
 *
 * Executed at reset vector before dropping to EL1 (firmware) or EL2.
 * Responsibilities:
 *   - Configure SCR_EL3 (Secure Configuration Register)
 *   - Set up GIC secure / non-secure group assignments
 *   - Configure secure / non-secure memory regions (TZC-400)
 *   - Install the EL3 exception vector table (synchronous / IRQ / FIQ / SError)
 *   - Set CPTR_EL3 to allow FPU/SVE from EL1/EL2
 *   - Configure ACTLR_EL3 platform-specific bits
 *   - Hand off: ERET to EL2/EL1 to continue firmware init
 *
 * On ARM32:
 *   - Configure the Monitor mode exception vectors
 *   - Set the NS bit in SCR
 *   - Write NSACR to allow VFP/NEON from non-secure world
 *
 * On x86-64:
 *   - TrustZone does not apply; this module is a stub that sets up
 *     SMM (System Management Mode) protection regions instead.
 *
 * @version 1.0.0
 * @date    2026-07-07
 */

 #ifndef UIOX_FW_TZ_H
 #define UIOX_FW_TZ_H
 
 #include "uiox_fw_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * TZ result codes
  * ====================================================================== */
 
 typedef enum {
     UIOX_TZ_OK             =  0,
     UIOX_TZ_ERR_EL         = -1,  /**< Not running at EL3              */
     UIOX_TZ_ERR_GIC        = -2,  /**< GIC secure config failed        */
     UIOX_TZ_ERR_TZC        = -3,  /**< TZC-400 region config failed    */
     UIOX_TZ_ERR_VECTORS    = -4,  /**< EL3 vector table install failed */
     UIOX_TZ_ERR_INVAL      = -5,
 } uiox_tz_result_t;
 
 /* =========================================================================
  * SCR_EL3 (Secure Configuration Register) bit definitions
  * ====================================================================== */
 
 #define SCR_EL3_NS          (1ULL << 0)   /**< Non-secure state bit     */
 #define SCR_EL3_IRQ         (1ULL << 1)   /**< IRQ routed to EL3        */
 #define SCR_EL3_FIQ         (1ULL << 2)   /**< FIQ routed to EL3        */
 #define SCR_EL3_EA          (1ULL << 3)   /**< Ext aborts to EL3        */
 #define SCR_EL3_SMD         (1ULL << 7)   /**< SMC disable              */
 #define SCR_EL3_HCE         (1ULL << 8)   /**< HVC enable               */
 #define SCR_EL3_SIF         (1ULL << 9)   /**< Secure instr fetch       */
 #define SCR_EL3_RW          (1ULL << 10)  /**< EL2/EL1 in AArch64       */
 #define SCR_EL3_ST          (1ULL << 11)  /**< Trap secure timer        */
 #define SCR_EL3_TWI         (1ULL << 12)  /**< Trap WFI to EL3          */
 #define SCR_EL3_TWE         (1ULL << 13)  /**< Trap WFE to EL3          */
 #define SCR_EL3_TLOR        (1ULL << 14)
 #define SCR_EL3_TERR        (1ULL << 15)
 #define SCR_EL3_APK         (1ULL << 16)  /**< Trap PAC key access      */
 #define SCR_EL3_API         (1ULL << 17)
 
 /* Recommended SCR_EL3 for firmware (EL1 = AArch64, SMC enabled) */
 #define SCR_EL3_FW_VALUE    (SCR_EL3_NS | SCR_EL3_RW | SCR_EL3_HCE)
 
 /* =========================================================================
  * CPTR_EL3 — Coprocessor trap register
  * ====================================================================== */
 
 #define CPTR_EL3_TCPAC      (1u << 31)
 #define CPTR_EL3_TTA        (1u << 20)
 #define CPTR_EL3_TFP        (1u << 10)  /**< Trap FP/SIMD              */
 #define CPTR_EL3_EZ         (1u <<  8)  /**< SVE enable                */
 /* Allow FP/SVE from EL1/EL2: clear TFP, set EZ */
 #define CPTR_EL3_FW_VALUE   (CPTR_EL3_EZ)
 
 /* =========================================================================
  * SPSR_EL3 values for ERET to EL2/EL1
  * ====================================================================== */
 
 #define SPSR_EL3_TO_EL2H    0x3C9ULL  /**< EL2h, DAIF masked, AArch64  */
 #define SPSR_EL3_TO_EL1H    0x3C5ULL  /**< EL1h, DAIF masked, AArch64  */
 
 /* =========================================================================
  * GIC secure group assignments
  * ====================================================================== */
 
 typedef enum {
     UIOX_TZ_GIC_GROUP0   = 0,  /**< Secure Group 0 (FIQ)              */
     UIOX_TZ_GIC_GROUP1NS = 1,  /**< Non-secure Group 1 (IRQ)          */
     UIOX_TZ_GIC_GROUP1S  = 2,  /**< Secure Group 1 (IRQ)              */
 } uiox_tz_gic_group_t;
 
 /* =========================================================================
  * TZC-400 (TrustZone Address Space Controller) region descriptor
  * ====================================================================== */
 
 #define UIOX_TZ_MAX_REGIONS     8u
 
 typedef struct {
     uint64_t base;
     uint64_t top;            /**< Inclusive end address                 */
     uint8_t  nsaid_rd_en;   /**< Bitmask of NS RAID allowed to read    */
     uint8_t  nsaid_wr_en;   /**< Bitmask of NS WAID allowed to write   */
     bool     secure_only;   /**< true = no NS access permitted         */
 } uiox_tzc_region_t;
 
 /* =========================================================================
  * TrustZone configuration
  * ====================================================================== */
 
 typedef struct {
     /* GIC base addresses */
     uintptr_t gic_dist_base;
     uintptr_t gic_rdist_base;  /**< GICv3 redistributor (0 = GICv2)   */
 
     /* EL3 vector table physical address */
     uintptr_t el3_vbar;
 
     /* TZC-400 (optional; set tzc_base=0 to skip) */
     uintptr_t tzc_base;
     uiox_tzc_region_t  tzc_regions[UIOX_TZ_MAX_REGIONS];
     uint32_t           tzc_region_count;
 
     /* Secure DRAM region (locked from non-secure world) */
     uint64_t secure_dram_base;
     uint64_t secure_dram_size;
 
     /* Where to ERET after EL3 setup */
     uintptr_t   ns_entry_addr;   /**< Non-secure world entry (EL2/EL1) */
     uint64_t    ns_spsr;         /**< SPSR_EL3 for ERET                 */
 
     /* Platform flags */
     bool  enable_fiq_routing;   /**< Route FIQs to EL3                 */
     bool  enable_gic_secure;    /**< Configure GIC secure groups        */
     bool  enable_tzc;           /**< Configure TZC-400                  */
 } uiox_tz_cfg_t;
 
 /* =========================================================================
  * TrustZone setup report
  * ====================================================================== */
 
 typedef struct {
     uiox_tz_result_t  result;
     uint64_t          scr_el3_value;
     uint64_t          cptr_el3_value;
     uint32_t          current_el;    /**< EL at time of call            */
     uint32_t          gic_groups_set;
     uint32_t          tzc_regions_set;
     bool              fpu_enabled;
     bool              smc_enabled;
     char              fail_msg[128];
 } uiox_tz_report_t;
 
 /* =========================================================================
  * TrustZone API
  * ====================================================================== */
 
 /**
  * One-time EL3 secure world setup.
  * Must be called from EL3 (reset vector context).
  * After return the caller should ERET to the non-secure world.
  *
  * @param cfg     TrustZone configuration.
  * @param report  Optional output report (pass NULL to skip).
  */
 uiox_tz_result_t  uiox_fw_tz_init         (const uiox_tz_cfg_t *cfg,
                                              uiox_tz_report_t *report);
 
 /** Configure GIC secure / non-secure group assignments. */
 uiox_tz_result_t  uiox_fw_tz_gic_secure   (uintptr_t gicd_base,
                                              uint32_t num_irqs,
                                              uint32_t secure_irq_mask);
 
 /** Configure a single TZC-400 region. */
 uiox_tz_result_t  uiox_fw_tzc_set_region  (uintptr_t tzc_base,
                                              uint32_t region_id,
                                              const uiox_tzc_region_t *r);
 
 /** Install the EL3 exception vector table at @vbar_pa. */
 uiox_tz_result_t  uiox_fw_tz_set_vbar     (uintptr_t vbar_pa);
 
 /**
  * Perform an ERET to the non-secure world entry point.
  * Never returns.
  */
 void __attribute__((noreturn))
                   uiox_fw_tz_eret_to_ns    (uintptr_t entry,
                                              uint64_t spsr,
                                              uint64_t x0_arg);
 
 /** Read current exception level (0–3). */
 uint32_t          uiox_fw_tz_current_el    (void);
 
 /** Print TZ report to debug UART. */
 void              uiox_fw_tz_print         (const uiox_tz_report_t *r);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_TZ_H */
 