/**
 * @file    uiox_soc_tz.h
 * @brief   UIOX SoC — ARM TrustZone / EL3 secure world setup.
 *
 * Executed at reset vector before dropping to EL1 (SoC HAL) or EL2.
 *
 * @version 1.0.0
 * @date    2026-07-07
 */

 #ifndef UIOX_SOC_TZ_H
 #define UIOX_SOC_TZ_H
 
 #include "uiox_soc_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* ── TZ result codes ────────────────────────────────────── */
 typedef enum {
     UIOX_SOC_TZ_OK          =  0,
     UIOX_SOC_TZ_ERR_EL      = -1,
     UIOX_SOC_TZ_ERR_GIC     = -2,
     UIOX_SOC_TZ_ERR_TZC     = -3,
     UIOX_SOC_TZ_ERR_VECTORS = -4,
     UIOX_SOC_TZ_ERR_INVAL   = -5,
 } uiox_soc_tz_result_t;
 
 /* ── SCR_EL3 bits ───────────────────────────────────────── */
 #define SCR_EL3_NS    (1ULL <<  0)
 #define SCR_EL3_IRQ   (1ULL <<  1)
 #define SCR_EL3_FIQ   (1ULL <<  2)
 #define SCR_EL3_EA    (1ULL <<  3)
 #define SCR_EL3_SMD   (1ULL <<  7)
 #define SCR_EL3_HCE   (1ULL <<  8)
 #define SCR_EL3_SIF   (1ULL <<  9)
 #define SCR_EL3_RW    (1ULL << 10)
 #define SCR_EL3_ST    (1ULL << 11)
 #define SCR_EL3_TWI   (1ULL << 12)
 #define SCR_EL3_TWE   (1ULL << 13)
 #define SCR_EL3_APK   (1ULL << 16)
 #define SCR_EL3_API   (1ULL << 17)
 
 /* Recommended SCR_EL3 for SoC HAL */
 #define SCR_EL3_SOC_VALUE   (SCR_EL3_NS | SCR_EL3_RW | SCR_EL3_HCE)
 
 /* ── CPTR_EL3 bits ──────────────────────────────────────── */
 #define CPTR_EL3_TCPAC   (1u << 31)
 #define CPTR_EL3_TTA     (1u << 20)
 #define CPTR_EL3_TFP     (1u << 10)
 #define CPTR_EL3_EZ      (1u <<  8)
 #define CPTR_EL3_SOC_VALUE  (CPTR_EL3_EZ)
 
 /* ── SPSR_EL3 values for ERET ───────────────────────────── */
 #define SPSR_EL3_TO_EL2H  0x3C9ULL
 #define SPSR_EL3_TO_EL1H  0x3C5ULL
 
 /* ── GIC secure group assignments ──────────────────────── */
 typedef enum {
     UIOX_SOC_TZ_GIC_GROUP0   = 0,
     UIOX_SOC_TZ_GIC_GROUP1NS = 1,
     UIOX_SOC_TZ_GIC_GROUP1S  = 2,
 } uiox_soc_tz_gic_group_t;
 
 /* ── TZC-400 region descriptor ──────────────────────────── */
 #define UIOX_SOC_TZ_MAX_REGIONS  8u
 
 typedef struct {
     uiox_uint64_t base;
     uiox_uint64_t top;
     uiox_uint8_t  nsaid_rd_en;
     uiox_uint8_t  nsaid_wr_en;
     uiox_bool_t     secure_only;
 } uiox_soc_tzc_region_t;
 
 /* ── TrustZone configuration ────────────────────────────── */
 typedef struct {
     uiox_uintptr_t               gic_dist_base;
     uiox_uintptr_t               gic_rdist_base;
     uiox_uintptr_t               el3_vbar;
     uiox_uintptr_t               tzc_base;
     uiox_soc_tzc_region_t   tzc_regions[UIOX_SOC_TZ_MAX_REGIONS];
     uiox_uint32_t                tzc_region_count;
     uiox_uint64_t                secure_dram_base;
     uiox_uint64_t                secure_dram_size;
     uiox_uintptr_t               ns_entry_addr;
     uiox_uint64_t                ns_spsr;
     uiox_bool_t                    enable_fiq_routing;
     uiox_bool_t                    enable_gic_secure;
     uiox_bool_t                    enable_tzc;
 } uiox_soc_tz_cfg_t;
 
 /* ── TrustZone report ───────────────────────────────────── */
 typedef struct {
     uiox_soc_tz_result_t result;
     uiox_uint64_t             scr_el3_value;
     uiox_uint64_t             cptr_el3_value;
     uiox_uint32_t             current_el;
     uiox_uint32_t             gic_groups_set;
     uiox_uint32_t             tzc_regions_set;
     uiox_bool_t                 fpu_enabled;
     uiox_bool_t                 smc_enabled;
     char                 fail_msg[128];
 } uiox_soc_tz_report_t;
 
 /* ── TrustZone API ──────────────────────────────────────── */
 uiox_soc_tz_result_t uiox_soc_tz_init       (const uiox_soc_tz_cfg_t *cfg,
                                                uiox_soc_tz_report_t *report);
 uiox_soc_tz_result_t uiox_soc_tz_gic_secure (uiox_uintptr_t gicd_base,
                                                uiox_uint32_t  num_irqs,
                                                uiox_uint32_t  secure_irq_mask);
 uiox_soc_tz_result_t uiox_soc_tzc_set_region(uiox_uintptr_t tzc_base,
                                                uiox_uint32_t  region_id,
                                                const uiox_soc_tzc_region_t *r);
 uiox_soc_tz_result_t uiox_soc_tz_set_vbar   (uiox_uintptr_t vbar_pa);
 
 void __attribute__((noreturn))
                      uiox_soc_tz_eret_to_ns  (uiox_uintptr_t entry,
                                                uiox_uint64_t  spsr,
                                                uiox_uint64_t  x0_arg);
 
 uiox_uint32_t             uiox_soc_tz_current_el  (void);
 void                 uiox_soc_tz_print       (const uiox_soc_tz_report_t *r);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SOC_TZ_H */
 