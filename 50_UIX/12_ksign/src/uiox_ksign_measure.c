/**
 * @file  uiox_ksign_measure.c
 * @brief UIOX Signed Kernel — PCR measurement log.
 * @date  2026-07-07
 */

 #include "../include/uiox_ksign_measure.h"

 extern void uiox_fw_printf(const char *fmt, ...);
 
 static void ms_memset(void *d,int v,size_t n)
 { uint8_t *p=(uint8_t*)d;while(n--)*p++=(uint8_t)v; }
 static void ms_memcpy(void *d,const void *s,size_t n)
 { uint8_t *dp=(uint8_t*)d;const uint8_t *sp=(const uint8_t*)s;while(n--)*dp++=*sp++; }
 static void ms_strncpy(char *d,const char *s,size_t n)
 { size_t i=0;while(i<n-1&&s[i]){d[i]=s[i];i++;}d[i]='\0'; }
 
 uiox_ks_err_t uiox_ks_measure_init(uiox_ks_measure_ctx_t *ctx,
                                       uint64_t (*get_time_ms)(void))
 {
     if (!ctx) return UIOX_KS_ERR_INVAL;
     ms_memset(ctx, 0, sizeof(*ctx));
     ctx->magic        = UIOX_KS_LOG_MAGIC;
     ctx->get_time_ms  = get_time_ms;
     ctx->locked       = false;
     return UIOX_KS_OK;
 }
 
 uiox_ks_err_t uiox_ks_measure_extend(uiox_ks_measure_ctx_t *ctx,
                                         uint32_t pcr_index,
                                         const uint8_t *data, size_t data_len,
                                         const char *event_name,
                                         uiox_ks_evt_type_t evt_type)
 {
     if (!ctx || !data || pcr_index >= UIOX_KS_PCR_MAX)
         return UIOX_KS_ERR_INVAL;
     uint8_t measurement[UIOX_KS_SHA256_LEN];
     uiox_ks_sha256(data, data_len, measurement);
     return uiox_ks_measure_extend_hash(ctx, pcr_index, measurement,
                                          event_name, evt_type);
 }
 
 uiox_ks_err_t uiox_ks_measure_extend_hash(uiox_ks_measure_ctx_t *ctx,
                                              uint32_t pcr_index,
                                              const uint8_t hash[UIOX_KS_SHA256_LEN],
                                              const char *event_name,
                                              uiox_ks_evt_type_t evt_type)
 {
     if (!ctx || !hash || pcr_index >= UIOX_KS_PCR_MAX)
         return UIOX_KS_ERR_INVAL;
     if (ctx->locked) return UIOX_KS_ERR_INVAL;
 
     /* PCR_new = SHA-256(PCR_old || hash) */
     uint8_t combined[UIOX_KS_SHA256_LEN * 2u];
     ms_memcpy(combined,                      ctx->pcr[pcr_index], 32u);
     ms_memcpy(combined + UIOX_KS_SHA256_LEN, hash,               32u);
     uiox_ks_sha256(combined, 64u, ctx->pcr[pcr_index]);
 
     /* Record in log */
     if (ctx->entry_count < UIOX_KS_LOG_MAX_ENTRIES) {
         uiox_ks_log_entry_t *e = &ctx->entries[ctx->entry_count++];
         e->pcr_index  = pcr_index;
         e->event_type = evt_type;
         if (event_name)
             ms_strncpy(e->event_name, event_name, UIOX_KS_EVENT_NAME_LEN);
         ms_memcpy(e->measurement, hash, 32u);
         ms_memcpy(e->pcr_after, ctx->pcr[pcr_index], 32u);
         e->timestamp_ms = ctx->get_time_ms ? ctx->get_time_ms() : 0u;
     }
     return UIOX_KS_OK;
 }
 
 void uiox_ks_measure_read_pcr(const uiox_ks_measure_ctx_t *ctx,
                                  uint32_t pcr_index,
                                  uint8_t out[UIOX_KS_SHA256_LEN])
 {
     if (!ctx || pcr_index >= UIOX_KS_PCR_MAX) return;
     ms_memcpy(out, ctx->pcr[pcr_index], UIOX_KS_SHA256_LEN);
 }
 
 void uiox_ks_measure_lock(uiox_ks_measure_ctx_t *ctx)
 { if (ctx) ctx->locked = true; }
 
 void uiox_ks_measure_quote(const uiox_ks_measure_ctx_t *ctx,
                               uint8_t quote[UIOX_KS_SHA256_LEN])
 {
     if (!ctx || !quote) return;
     uiox_ks_sha256_ctx_t hctx;
     uiox_ks_sha256_init(&hctx);
     for (uint32_t i=0u;i<UIOX_KS_PCR_MAX;i++)
         uiox_ks_sha256_update(&hctx, ctx->pcr[i], UIOX_KS_SHA256_LEN);
     uiox_ks_sha256_final(&hctx, quote);
 }
 
 void uiox_ks_measure_print(const uiox_ks_measure_ctx_t *ctx)
 {
     if (!ctx) return;
     static const char *evt_names[] = {
         "FIRMWARE","KERNEL
 