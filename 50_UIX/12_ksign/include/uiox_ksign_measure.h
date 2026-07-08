/**
 * @file  uiox_ksign_measure.h
 * @brief UIOX Signed Kernel — TPM-style PCR measurement log.
 * @version 1.0.0
 */

 #ifndef UIOX_KSIGN_MEASURE_H
 #define UIOX_KSIGN_MEASURE_H
 
 #include "uiox_ksign_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_KS_LOG_MAX_ENTRIES  64u
 #define UIOX_KS_EVENT_NAME_LEN   48u
 
 /* =========================================================================
  * Event types
  * ====================================================================== */
 
 typedef enum {
     UIOX_KS_EVT_FIRMWARE     = 0,
     UIOX_KS_EVT_KERNEL_CODE  = 1,
     UIOX_KS_EVT_KERNEL_DATA  = 2,
     UIOX_KS_EVT_CMDLINE      = 3,
     UIOX_KS_EVT_MODULE       = 4,
     UIOX_KS_EVT_RUNTIME_CHK  = 5,
     UIOX_KS_EVT_KEY_LOAD     = 6,
     UIOX_KS_EVT_SIG_VERIFY   = 7,
 } uiox_ks_evt_type_t;
 
 /* =========================================================================
  * Measurement log entry
  * ====================================================================== */
 
 typedef struct {
     uint32_t          pcr_index;
     uiox_ks_evt_type_t event_type;
     char              event_name[UIOX_KS_EVENT_NAME_LEN];
     uint8_t           measurement[UIOX_KS_SHA256_LEN]; /**< What was measured */
     uint8_t           pcr_after [UIOX_KS_SHA256_LEN];  /**< PCR value after  */
     uint64_t          timestamp_ms;
 } uiox_ks_log_entry_t;
 
 /* =========================================================================
  * Measurement log context
  * ====================================================================== */
 
 typedef struct {
     uint32_t            magic;
     uint8_t             pcr[UIOX_KS_PCR_MAX][UIOX_KS_SHA256_LEN];
     uiox_ks_log_entry_t entries[UIOX_KS_LOG_MAX_ENTRIES];
     uint32_t            entry_count;
     bool                locked;        /**< PCRs sealed after kernel boot*/
     uint64_t            (*get_time_ms)(void);
 } uiox_ks_measure_ctx_t;
 
 /* =========================================================================
  * Measurement API
  * ====================================================================== */
 
 uiox_ks_err_t uiox_ks_measure_init   (uiox_ks_measure_ctx_t *ctx,
                                          uint64_t (*get_time_ms)(void));
 
 /**
  * Extend PCR @index with @data:
  *   PCR_new = SHA-256(PCR_old || SHA-256(@data, @data_len))
  */
 uiox_ks_err_t uiox_ks_measure_extend (uiox_ks_measure_ctx_t *ctx,
                                          uint32_t pcr_index,
                                          const uint8_t *data, size_t data_len,
                                          const char *event_name,
                                          uiox_ks_evt_type_t evt_type);
 
 /** Extend PCR with a pre-computed @hash. */
 uiox_ks_err_t uiox_ks_measure_extend_hash(uiox_ks_measure_ctx_t *ctx,
                                              uint32_t pcr_index,
                                              const uint8_t hash[UIOX_KS_SHA256_LEN],
                                              const char *event_name,
                                              uiox_ks_evt_type_t evt_type);
 
 /** Read current PCR value. */
 void          uiox_ks_measure_read_pcr(const uiox_ks_measure_ctx_t *ctx,
                                          uint32_t pcr_index,
                                          uint8_t out[UIOX_KS_SHA256_LEN]);
 
 /** Lock all PCRs (prevents further extends after kernel is running). */
 void          uiox_ks_measure_lock   (uiox_ks_measure_ctx_t *ctx);
 
 /** Generate attestation quote (SHA-256 of all PCRs). */
 void          uiox_ks_measure_quote  (const uiox_ks_measure_ctx_t *ctx,
                                          uint8_t quote[UIOX_KS_SHA256_LEN]);
 
 void          uiox_ks_measure_print  (const uiox_ks_measure_ctx_t *ctx);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KSIGN_MEASURE_H */
 