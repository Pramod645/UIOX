/**
 * @file    uiox_soc_power.h
 * @brief   UIOX SoC — Power management (PSCI for ARM, ACPI for x86).
 * @version 1.0.0
 * @date    2026-06-21
 */

 #ifndef UIOX_SOC_POWER_H
 #define UIOX_SOC_POWER_H
 
 /*
  * uiox_soc_types.h is included first — it provides:
  *   uiox_soc_cpu_state_t   (REMOVED from this file — was causing conflict)
  *   UIOX_SOC_MAX_CPUS      (REMOVED from this file — was causing conflict)
  */
 #include "uiox_soc_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* ── PSCI function IDs (ARM PSCI 1.0) ──────────────────────────────── */
 #define PSCI_VERSION          0x84000000u
 #define PSCI_CPU_SUSPEND      0x84000001u
 #define PSCI_CPU_OFF          0x84000002u
 #define PSCI_CPU_ON           0x84000003u
 #define PSCI_AFFINITY_INFO    0x84000004u
 #define PSCI_MIGRATE          0x84000005u
 #define PSCI_SYSTEM_OFF       0x84000008u
 #define PSCI_SYSTEM_RESET     0x84000009u
 
 /* ── ACPI PM1 (x86 q35) ─────────────────────────────────────────────── */
 #define ACPI_PM1A_CNT_BLOCK   0xB004u
 #define ACPI_S5_SLEEP_TYPE    (0x07u << 10u)
 #define ACPI_SLP_EN           UIOX_SOC_BIT(13)
 
 /* ── Power states ───────────────────────────────────────────────────── */
 typedef enum {
     UIOX_SOC_PWR_ACTIVE = 0,
     UIOX_SOC_PWR_SLEEP  = 1,  /**< WFI / HLT                           */
     UIOX_SOC_PWR_DEEP   = 2,  /**< PSCI CPU_SUSPEND / ACPI S3          */
     UIOX_SOC_PWR_OFF    = 3,  /**< PSCI SYSTEM_OFF  / ACPI S5          */
 } uiox_soc_pwr_state_t;
 
 /*
  * NOTE: uiox_soc_cpu_state_t is NOT defined here.
  * It is defined once in uiox_soc_types.h and shared with uiox_soc_psci.h.
  * Values available: UIOX_SOC_CPU_OFF / ON / PENDING / SUSPEND
  *
  * NOTE: UIOX_SOC_MAX_CPUS is NOT defined here.
  * It is defined in uiox_soc_types.h as 8u.
  */
 
 /* ── Power management context ───────────────────────────────────────── */
 typedef struct {
     uiox_soc_pwr_state_t  system_state;
     uiox_soc_cpu_state_t  cpu_state[UIOX_SOC_MAX_CPUS]; /* from types.h */
     uiox_uint32_t         num_cpus;
     uiox_bool_t           psci_available;
     uiox_bool_t           acpi_available;
 } uiox_soc_power_ctx_t;
 
 /* ── Power API ──────────────────────────────────────────────────────── */
 uiox_soc_err_t uiox_soc_power_init     (uiox_soc_power_ctx_t *ctx);
 void           uiox_soc_power_idle     (void);  /**< WFI / HLT          */
 uiox_soc_err_t uiox_soc_power_cpu_on   (uiox_soc_power_ctx_t *ctx,
                                          uiox_uint32_t  cpu_id,
                                          uiox_uintptr_t entry_pa);
 uiox_soc_err_t uiox_soc_power_cpu_off  (uiox_soc_power_ctx_t *ctx,
                                          uiox_uint32_t  cpu_id);
 void __attribute__((noreturn))
                uiox_soc_power_reset    (void);
 void __attribute__((noreturn))
                uiox_soc_power_shutdown (void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SOC_POWER_H */
 