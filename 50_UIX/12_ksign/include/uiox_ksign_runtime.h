/**
 * @file  uiox_ksign_runtime.h
 * @brief UIOX Signed Kernel — runtime integrity monitoring.
 *
 * Periodically re-hashes critical kernel .text and .rodata regions
 * and compares against the verified boot-time hashes. Detects:
 *   - Live patching without authorization (unregistered kpatch)
 *   - Memory corruption of kernel code
 *   - Rootkit trampolines injected after boot
 *
 * @version 1.0.0
 */

 #ifndef UIOX_KSIGN_RUNTIME_H
 #define UIOX_KSIGN_RUNTIME_H
 
 #include "uiox_ksign_measure.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_KS_RT_MAX_REGIONS     8u
 #define UIOX_KS_RT_CHECK_INTERVAL  60000u  /**< Recheck every 60 s     */
 #define UIOX_KS_RT_REGION_NAME_LEN 24u
 
 /* =========================================================================
  * Monitored region descriptor
  * ====================================================================== */
 
 typedef struct {
     char     name[UIOX_KS_RT_REGION_NAME_LEN];
     uintptr_t base;
     size_t   size;
     uint8_t  expected_hash[UIOX_KS_SHA256_LEN]; /**< Hash at boot time  */
     uint64_t last_check_ms;
     uint32_t violation_count;
     bool     active;
 } uiox_ks_rt_region_t;
 
 /* =========================================================================
  * Runtime monitor context
  * ====================================================================== */
 
 typedef enum {
     UIOX_KS_RT_OK        = 0,
     UIOX_KS_RT_TAMPERED  = 1,
     UIOX_KS_RT_DISABLED  = 2,
 } uiox_ks_rt_state_t;
 
 typedef void (*uiox_ks_rt_violation_cb_t)(const uiox_ks_rt_region_t *region,
                                             const uint8_t actual[UIOX_KS_SHA256_LEN],
                                             void *priv);
 
 typedef struct {
     uiox_ks_rt_region_t   regions[UIOX_KS_RT_MAX_REGIONS];
     uint32_t               region_count;
     uiox_ks_rt_state_t     state;
     uiox_ks_measure_ctx_t *measure;    /**< For PCR extends on violation */
     uiox_ks_rt_violation_cb_t cb;
     void                  *cb_priv;
     uint32_t               check_count;
     uint32_t               violation_count;
     uint64_t               (*get_time_ms)(void);
     bool                   panic_on_violation;  /**< Halt if tampered?  */
 } uiox_ks_rt_ctx_t;
 
 /* =========================================================================
  * Runtime monitor API
  * ====================================================================== */
 
 uiox_ks_err_t uiox_ks_rt_init       (uiox_ks_rt_ctx_t *ctx,
                                         uiox_ks_measure_ctx_t *measure,
                                         uint64_t (*get_time_ms)(void));
 
 /**
  * Register a kernel region to monitor.
  * Call this during Stage 7 (scheduler init) for each .text / .rodata region.
  * The hash is computed NOW and stored as the expected value.
  */
 uiox_ks_err_t uiox_ks_rt_register   (uiox_ks_rt_ctx_t *ctx,
                                         const char *name,
                                         uintptr_t base, size_t size);
 
 /**
  * Register a region with a known-good hash (from the verified image header).
  */
 uiox_ks_err_t uiox_ks_rt_register_hash(uiox_ks_rt_ctx_t *ctx,
                                           const char *name,
                                           uintptr_t base, size_t size,
                                           const uint8_t expected[UIOX_KS_SHA256_LEN]);
 
 /**
  * Tick: call from the scheduler tick (every 60 s by default).
  * Rehashes each registered region and compares.
  */
 void          uiox_ks_rt_tick        (uiox_ks_rt_ctx_t *ctx, uint64_t now_ms);
 
 /** Set violation callback. */
 void          uiox_ks_rt_set_cb      (uiox_ks_rt_ctx_t *ctx,
                                         uiox_ks_rt_violation_cb_t cb,
                                         void *priv);
 
 /** Force an immediate full check of all regions. */
 uiox_ks_rt_state_t uiox_ks_rt_check_all(uiox_ks_rt_ctx_t *ctx);
 
 void          uiox_ks_rt_print       (const uiox_ks_rt_ctx_t *ctx);
 
 /* =========================================================================
  * Syscall interface
  * ====================================================================== */
 
 #define SYS_KERNEL_VERIFY    220u
 #define SYS_KSIGN_STATUS     221u
 #define SYS_KSIGN_QUOTE      222u
 
 long sys_kernel_verify(long image_addr, long image_size, long flags, long a3);
 long sys_ksign_status (long buf,        long buf_size,   long a2,    long a3);
 long sys_ksign_quote  (long buf,        long buf_size,   long a2,    long a3);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KSIGN_RUNTIME_H */
 