/**
 * @file    uiox_netbuf.h
 * @brief   UIOX network buffer (netbuf) — zero-copy packet buffer manager.
 *
 * Every packet in flight is described by one uiox_netbuf_t. The design
 * follows a headroom/data/tailroom layout so that headers can be
 * prepended and trailers appended without copying:
 *
 *   [ headroom | <--- data (len bytes) ---> | tailroom ]
 *   ^           ^                           ^           ^
 *   buf_start   data                        data+len    buf_end
 *
 * Buffers are drawn from a fixed pool to avoid heap fragmentation on
 * embedded/RTOS targets.
 *
 * @date    2026-05-25
 */
//Layer 2.5 — Network Buffer Manager
 #ifndef UIOX_NETBUF_H
 #define UIOX_NETBUF_H
 
#include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Pool configuration
  * ====================================================================== */
 
 #define UIOX_NETBUF_POOL_SIZE       256     /**< Total buffers in pool        */
 #define UIOX_NETBUF_DATA_SIZE       1600    /**< Per-buffer payload capacity  */
 #define UIOX_NETBUF_HEADROOM        128     /**< Reserved bytes before data   */
 
 /* =========================================================================
  * Buffer descriptor
  * ====================================================================== */
 
 typedef struct uiox_netbuf {
     uint8_t            *buf_start;  /**< Start of allocated storage           */
     uint8_t            *buf_end;    /**< One past end of allocated storage     */
     uint8_t            *data;       /**< Current start of packet data          */
     uint16_t            len;        /**< Current packet data length (bytes)    */
     uint16_t            total_len;  /**< Total length (sum over chain)         */
 
     struct uiox_netbuf *next;       /**< Next buffer in chain (fragmentation)  */
     struct uiox_netbuf *next_free;  /**< Free-list linkage (internal)          */
 
     uint8_t             ref;        /**< Reference count                       */
     uint8_t             flags;      /**< UIOX_NBUF_F_* bitmask                */
     uint16_t            proto;      /**< Detected EtherType (filled by netif)  */
     void               *priv;       /**< Layer-private scratch pointer         */
 } uiox_netbuf_t;
 
 /* Buffer flags */
 #define UIOX_NBUF_F_CLONED      (1u << 0)  /**< Buffer is a clone (shared)   */
 #define UIOX_NBUF_F_CSUM_VALID  (1u << 1)  /**< RX checksum verified by HW   */
 #define UIOX_NBUF_F_BROADCAST   (1u << 2)  /**< Received as broadcast        */
 #define UIOX_NBUF_F_MULTICAST   (1u << 3)  /**< Received as multicast        */
 
 /* =========================================================================
  * Pool management
  * ====================================================================== */
 
 /** Initialise the buffer pool — call once at boot before any netbuf use. */
 void uiox_netbuf_pool_init(void);
 
 /** Return number of free buffers remaining in the pool. */
 uint16_t uiox_netbuf_pool_free(void);
 
 /* =========================================================================
  * Buffer lifecycle
  * ====================================================================== */
 
 /**
  * @brief  Allocate a buffer from the pool.
  * @return Pointer to initialised buffer, or NULL if pool exhausted.
  */
 uiox_netbuf_t *uiox_netbuf_alloc(void);
 
 /**
  * @brief  Decrement reference count; return to pool when it reaches 0.
  */
 void uiox_netbuf_free(uiox_netbuf_t *buf);
 
 /**
  * @brief  Increment reference count (for shared/cloned buffers).
  */
 void uiox_netbuf_ref(uiox_netbuf_t *buf);
 
 /* =========================================================================
  * Data manipulation (zero-copy)
  * ====================================================================== */
 
 /**
  * @brief  Prepend `len` bytes of headroom — moves data pointer back.
  * @return Pointer to the new data start, or NULL if no headroom left.
  */
 void *uiox_netbuf_push(uiox_netbuf_t *buf, uint16_t len);
 
 /**
  * @brief  Remove `len` bytes from the front — moves data pointer forward.
  * @return Pointer to new data start, or NULL if len > buf->len.
  */
 void *uiox_netbuf_pull(uiox_netbuf_t *buf, uint16_t len);
 
 /**
  * @brief  Append `len` bytes of space at the tail.
  * @return Pointer to the appended region, or NULL if no tailroom left.
  */
 void *uiox_netbuf_put(uiox_netbuf_t *buf, uint16_t len);
 
 /**
  * @brief  Trim `len` bytes from the tail.
  */
 void  uiox_netbuf_trim(uiox_netbuf_t *buf, uint16_t len);
 
 /** Available headroom (bytes free before data). */
 static inline uint16_t uiox_netbuf_headroom(const uiox_netbuf_t *b)
 { return (uint16_t)(b->data - b->buf_start); }
 
 /** Available tailroom (bytes free after data). */
 static inline uint16_t uiox_netbuf_tailroom(const uiox_netbuf_t *b)
 { return (uint16_t)(b->buf_end - (b->data + b->len)); }
 
 /* =========================================================================
  * Chain helpers
  * ====================================================================== */
 
 /** Append `tail` to the end of the buffer chain rooted at `head`. */
 void uiox_netbuf_chain(uiox_netbuf_t *head, uiox_netbuf_t *tail);
 
 /** Return total byte count across all chained buffers. */
 uint16_t uiox_netbuf_total_len(const uiox_netbuf_t *head);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* UIOX_NETBUF_H */
 