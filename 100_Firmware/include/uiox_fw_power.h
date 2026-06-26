/**
 * @file  uiox_fw_power.h
 * @brief UIOX Firmware — Power management (PSCI for ARM, ACPI for x86).
 * @version 1.0.0
 * @date    2026-06-21
 */

 #ifndef UIOX_FW_POWER_H
 #define UIOX_FW_POWER_H
 
 #include "uiox_fw_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* PSCI function IDs (ARM PSCI 1.0) */
 #define PSCI_VERSION            0x84000000u
 #define PSCI_CPU_SUSPEND        0x84000001u
 #define PSCI_CPU_OFF            0x84000002u
 #define PSCI_CPU_ON             0x84000003u
 #define PSCI_AFFINITY_INFO      0x84000004u
 #define PSCI_MIGRATE            0x84000005u
 #define PSCI_SYSTEM_OFF         0x84000008u
 #define PSCI_SYSTEM_RESET       0x84000009u
 
 /* ACPI PM1 (x86 q35) */
 #define ACPI_PM1A_CNT_BLOCK     0xB004u
 #define ACPI_S5_SLEEP_TYPE      (0x07u << 10u)
 #define ACPI_SLP_EN             UIOX_FW_BIT(13)
 
 /* Power states */
 typedef enum {
     UIOX_FW_PWR_ACTIVE   = 0,
     UIOX_FW_PWR_SLEEP    = 1,   /**< WFI / HLT                         */
     UIOX_FW_PWR_DEEP     = 2,   /**< PSCI CPU_SUSPEND / ACPI S3        */
     UIOX_FW_PWR_OFF      = 3,   /**< PSCI SYSTEM_OFF / ACPI S5         */
 } uiox_fw_pwr_state_t;
 
 /* CPU hot-plug state */
 typedef enum {
     UIOX_FW_CPU_OFF     = 0,
     UIOX_FW_CPU_ON      = 1,
     UIOX_FW_CPU_PENDING = 2,
 } uiox_fw_cpu_state_t;
 
 #define UIOX_FW_MAX_CPUS        8u
 
 typedef struct {
     uiox_fw_pwr_state_t  system_state;
     uiox_fw_cpu_state_t  cpu_state[UIOX_FW_MAX_CPUS];
     uint32_t             num_cpus;
     bool                 psci_available;
     bool                 acpi_available;
 } uiox_fw_power_ctx_t;
 
 /* API */
 uiox_fw_err_t uiox_fw_power_init      (uiox_fw_power_ctx_t *ctx);
 void          uiox_fw_power_idle      (void);   /**< WFI / HLT (low-power wait) */
 uiox_fw_err_t uiox_fw_power_cpu_on    (uiox_fw_power_ctx_t *ctx,
                                          uint32_t cpu_id,
                                          uintptr_t entry_pa);
 uiox_fw_err_t uiox_fw_power_cpu_off   (uiox_fw_power_ctx_t *ctx,
                                          uint32_t cpu_id);
 void __attribute__((noreturn))
               uiox_fw_power_reset     (void);
 void __attribute__((noreturn))
               uiox_fw_power_shutdown  (void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_POWER_H */
 