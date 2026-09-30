/**
 * @file    uiox_tpwd_buf.h
 * @brief   UIOX Touch-Password buffer pools.
 *
 * Two pools:
 *   EVENT pool  — raw touch events (ring buffer, lock-free SPSC)
 *   CRED pool   — credential entries (hash + salt, zeroed on free)
 *
 * All credential buffers are zeroed before returning to the pool to
 * prevent password material remaining in memory.
 *
 * @date    2026-06-01
 */

 #ifndef UIOX_TPWD_BUF_H
 #define UIOX_TPWD_BUF_H
 
 #include "uiox_tpwd_hw.h"
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Event ring buffer
  * ====================================================================== */
 
 #define UIOX_TPWD_EVT_BUF_SIZE   64    /**< Must be power of 2           */
 #define UIOX_TPWD_EVT_BUF_MASK   (UIOX_TPWD_EVT_BUF_SIZE - 1)
 
 typedef struct {
     uiox_tpwd_raw_evt_t  buf[UIOX_TPWD_EVT_BUF_SIZE];
     volatile uint32_t    head;
     volatile uint32_t    tail;
     uint32_t             overflow;
 } uiox_tpwd_evtbuf_t;
 
 void uiox_tpwd_evtbuf_init (uiox_tpwd_evtbuf_t *rb);
 bool uiox_tpwd_evtbuf_push (uiox_tpwd_evtbuf_t *rb,
                               const uiox_tpwd_raw_evt_t *ev);
 bool uiox_tpwd_evtbuf_pop  (uiox_tpwd_evtbuf_t *rb,
                               uiox_tpwd_raw_evt_t *ev);
 bool uiox_tpwd_evtbuf_empty(const uiox_tpwd_evtbuf_t *rb);
 uint32_t uiox_tpwd_evtbuf_count(const uiox_tpwd_evtbuf_t *rb);
 
 /* =========================================================================
  * Credential buffer pool
  * ====================================================================== */
 
 #define UIOX_TPWD_CRED_POOL_SIZE  4
 #define UIOX_TPWD_HASH_LEN        32   /**< SHA-256 digest length         */
 #define UIOX_TPWD_SALT_LEN        16
 #define UIOX_TPWD_MAX_PIN_LEN     16   /**< Max digits for PIN entry      */
 #define UIOX_TPWD_MAX_PATTERN_PTS 9    /**< Max nodes in pattern (3×3)    */
 
 typedef struct uiox_tpwd_cred {
     uint8_t  hash[UIOX_TPWD_HASH_LEN];  /**< HMAC-SHA256 of credential   */
     uint8_t  salt[UIOX_TPWD_SALT_LEN];  /**< Random salt                 */
     uint8_t  attempts;                   /**< Failed attempts counter      */
     uint8_t  in_use;                     /**< Reference count             */
     struct uiox_tpwd_cred *next;
 } uiox_tpwd_cred_t;
 
 void             uiox_tpwd_cred_pool_init(void);
 uiox_tpwd_cred_t *uiox_tpwd_cred_alloc  (void);
 void             uiox_tpwd_cred_free     (uiox_tpwd_cred_t *c);
 uint8_t          uiox_tpwd_cred_free_count(void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_TPWD_BUF_H */
 