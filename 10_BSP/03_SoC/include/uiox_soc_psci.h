/**
 * @file    uiox_soc_psci.h
 * @brief   UIOX SoC — PSCI 1.1 dispatch table.
 * @version 1.0.0
 * @date    2026-07-07
 */

 #ifndef UIOX_SOC_PSCI_H
 #define UIOX_SOC_PSCI_H
 
 /*
  * uiox_soc_types.h provides:
  *   uiox_soc_cpu_state_t   (REMOVED from this file — was causing conflict)
  *   UIOX_SOC_MAX_CPUS      (REMOVED from this file — was causing conflict)
  */
 #include "uiox_soc_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* ── PSCI function IDs ──────────────────────────────────────────────── */
 #define PSCI_FN_VERSION                 0x84000000u
 #define PSCI_FN32_CPU_SUSPEND           0x84000001u
 #define PSCI_FN32_CPU_OFF               0x84000002u
 #define PSCI_FN32_CPU_ON                0x84000003u
 #define PSCI_FN32_AFFINITY_INFO         0x84000004u
 #define PSCI_FN32_MIGRATE               0x84000005u
 #define PSCI_FN32_MIGRATE_INFO_TYPE     0x84000006u
 #define PSCI_FN32_MIGRATE_INFO_UP_CPU   0x84000007u
 #define PSCI_FN_SYSTEM_OFF              0x84000008u
 #define PSCI_FN_SYSTEM_RESET            0x84000009u
 #define PSCI_FN_SYSTEM_RESET2           0x84000012u
 #define PSCI_FN64_CPU_SUSPEND           0xC4000001u
 #define PSCI_FN64_CPU_ON                0xC4000003u
 #define PSCI_FN64_AFFINITY_INFO         0xC4000004u
 #define PSCI_FN64_MIGRATE               0xC4000005u
 #define PSCI_FN64_MIGRATE_INFO_UP_CPU   0xC4000007u
 #define PSCI_FN_FEATURES                0x8400000Au
 #define PSCI_FN_CPU_FREEZE              0x8400000Bu
 #define PSCI_FN_SYSTEM_SUSPEND          0x8400000Eu
 #define PSCI_FN64_SYSTEM_SUSPEND        0xC400000Eu
 #define PSCI_FN_SET_SUSPEND_MODE        0x8400000Fu
 #define PSCI_FN_STAT_RESIDENCY          0x84000010u
 #define PSCI_FN_STAT_COUNT              0x84000011u
 
 /* ── PSCI return codes ──────────────────────────────────────────────── */
 #define PSCI_RET_SUCCESS            0
 #define PSCI_RET_NOT_SUPPORTED     (-1)
 #define PSCI_RET_INVALID_PARAMS    (-2)
 #define PSCI_RET_DENIED            (-3)
 #define PSCI_RET_ALREADY_ON        (-4)
 #define PSCI_RET_ON_PENDING        (-5)
 #define PSCI_RET_INTERNAL_FAILURE  (-6)
 #define PSCI_RET_NOT_PRESENT       (-7)
 #define PSCI_RET_DISABLED          (-8)
 #define PSCI_RET_INVALID_ADDRESS   (-9)
 
 /* ── PSCI version ───────────────────────────────────────────────────── */
 #define PSCI_VERSION_MAJOR   1u
 #define PSCI_VERSION_MINOR   1u
 #define PSCI_VERSION_VALUE   ((PSCI_VERSION_MAJOR << 16u) | \
                                PSCI_VERSION_MINOR)
 
 /* ── AFFINITY_INFO return values ────────────────────────────────────── */
 #define PSCI_AFFINITY_LEVEL_ON      0u
 #define PSCI_AFFINITY_LEVEL_OFF     1u
 #define PSCI_AFFINITY_LEVEL_ON_PEND 2u
 
 /*
  * NOTE: uiox_soc_cpu_state_t is NOT defined here.
  * It is defined once in uiox_soc_types.h with values:
  *   UIOX_SOC_CPU_OFF / ON / PENDING / SUSPEND
  *
  * NOTE: UIOX_SOC_MAX_CPUS is NOT defined here.
  * It is defined in uiox_soc_types.h as 8u.
  */
 
 /* ── Per-CPU PSCI descriptor ────────────────────────────────────────── */
 typedef struct {
     uiox_soc_cpu_state_t state;        /* uses type from uiox_soc_types.h */
     uiox_uint64_t        affinity;     /**< MPIDR_EL1 value for this CPU  */
     uiox_uintptr_t       warm_entry;   /**< Warm-boot entry address        */
     uiox_uint64_t        context_id;   /**< Passed in CPU_ON x2            */
     uiox_uint64_t        suspend_state;/**< Last CPU_SUSPEND state         */
 } uiox_soc_psci_cpu_t;
 
 /* ── PSCI handler and dispatch table ────────────────────────────────── */
 typedef uiox_int64_t (*uiox_soc_psci_handler_t)(uiox_uint64_t a1,
                                                    uiox_uint64_t a2,
                                                    uiox_uint64_t a3,
                                                    uiox_uint64_t a4);
 
 typedef struct {
     uiox_uint32_t              fn_id;
     const char                *name;
     uiox_soc_psci_handler_t    handler;
 } uiox_soc_psci_entry_t;
 
 /* ── PSCI registration context ──────────────────────────────────────── */
 typedef struct {
     uiox_soc_psci_cpu_t  cpus[UIOX_SOC_MAX_CPUS]; /* uses limit from types.h */
     uiox_uint32_t        num_cpus;
     uiox_bool_t          smc_enabled;
     uiox_bool_t          initialized;
     void (*platform_cpu_on) (uiox_uint32_t cpu_id, uiox_uintptr_t entry);
     void (*platform_cpu_off)(uiox_uint32_t cpu_id);
     void (*platform_reset)  (void) __attribute__((noreturn));
     void (*platform_off)    (void) __attribute__((noreturn));
     uiox_uint32_t cpu_on_count;
     uiox_uint32_t cpu_off_count;
     uiox_uint32_t suspend_count;
     uiox_uint32_t reset_count;
     uiox_uint32_t unknown_fn_count;
 } uiox_soc_psci_ctx_t;
 
 /* ── PSCI API ───────────────────────────────────────────────────────── */
 uiox_soc_err_t       uiox_soc_psci_init      (uiox_soc_psci_ctx_t *ctx,
                                                 uiox_uint32_t num_cpus,
                                                 uiox_bool_t   use_smc);
 void                 uiox_soc_psci_set_cpu_on(uiox_soc_psci_ctx_t *ctx,
                                                 void (*fn)(uiox_uint32_t,
                                                            uiox_uintptr_t));
 void uiox_soc_psci_set_reset(uiox_soc_psci_ctx_t *ctx,
     void __attribute__((noreturn)) (*fn)(void));
 void uiox_soc_psci_set_off  (uiox_soc_psci_ctx_t *ctx,
     void __attribute__((noreturn)) (*fn)(void));
 
 uiox_int64_t         uiox_soc_psci_dispatch  (uiox_soc_psci_ctx_t *ctx,
                                                 uiox_uint64_t fn_id,
                                                 uiox_uint64_t a1,
                                                 uiox_uint64_t a2,
                                                 uiox_uint64_t a3);
 uiox_bool_t          uiox_soc_psci_supported (uiox_uint32_t fn_id);
 uiox_soc_cpu_state_t uiox_soc_psci_cpu_state (const uiox_soc_psci_ctx_t *ctx,
                                                 uiox_uint32_t cpu_id);
 void                 uiox_soc_psci_print     (const uiox_soc_psci_ctx_t *ctx);
 
 /* Individual handlers */
 uiox_int64_t uiox_soc_psci_version       (uiox_uint64_t, uiox_uint64_t,
                                             uiox_uint64_t, uiox_uint64_t);
 uiox_int64_t uiox_soc_psci_cpu_on        (uiox_uint64_t, uiox_uint64_t,
                                             uiox_uint64_t, uiox_uint64_t);
 uiox_int64_t uiox_soc_psci_cpu_off       (uiox_uint64_t, uiox_uint64_t,
                                             uiox_uint64_t, uiox_uint64_t);
 uiox_int64_t uiox_soc_psci_cpu_suspend   (uiox_uint64_t, uiox_uint64_t,
                                             uiox_uint64_t, uiox_uint64_t);
 uiox_int64_t uiox_soc_psci_affinity_info (uiox_uint64_t, uiox_uint64_t,
                                             uiox_uint64_t, uiox_uint64_t);
 uiox_int64_t uiox_soc_psci_system_off    (uiox_uint64_t, uiox_uint64_t,
                                             uiox_uint64_t, uiox_uint64_t);
 uiox_int64_t uiox_soc_psci_system_reset  (uiox_uint64_t, uiox_uint64_t,
                                             uiox_uint64_t, uiox_uint64_t);
 uiox_int64_t uiox_soc_psci_features      (uiox_uint64_t, uiox_uint64_t,
                                             uiox_uint64_t, uiox_uint64_t);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SOC_PSCI_H */
 