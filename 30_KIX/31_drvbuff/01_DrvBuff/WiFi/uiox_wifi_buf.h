/**
 * @file    uiox_wifi_buf.h
 * @brief   UIOX WiFi MPDU/MSDU frame buffer pool.
 *
 * Two pools:
 *   TX pool — frames to be transmitted (application → MAC)
 *   RX pool — frames received from air (MAC → application)
 *
 * Frame layout:
 *   [ 802.11 MAC header | LLC/SNAP | payload | FCS (HW) ]
 *   Headroom reserved for MAC header prepend (zero-copy).
 *
 * @date    2026-05-28
 */
//Layer 1.5 — Buffer Manager
 #ifndef UIOX_WIFI_BUF_H
 #define UIOX_WIFI_BUF_H
 
#include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Pool sizing
  * ====================================================================== */
 
 #define UIOX_WIFI_TX_POOL_SIZE    32
 #define UIOX_WIFI_RX_POOL_SIZE    64
 #define UIOX_WIFI_FRAME_MAX       2346   /**< Max 802.11 MSDU + header     */
 #define UIOX_WIFI_HEADROOM        64     /**< Reserved for MAC header       */
 #define UIOX_WIFI_BUF_ALIGN       64
 
 /* =========================================================================
  * Frame type
  * ====================================================================== */
 
 typedef enum {
     UIOX_WIFI_FRAME_DATA = 0,
     UIOX_WIFI_FRAME_MGMT,
     UIOX_WIFI_FRAME_CTRL,
 } uiox_wifi_frame_type_t;
 
 /* =========================================================================
  * Frame buffer descriptor
  * ====================================================================== */
 
 typedef struct uiox_wifi_frame {
     uint8_t    *buf_start;      /**< Start of allocated storage            */
     uint8_t    *buf_end;        /**< One past end                          */
     uint8_t    *data;           /**< Current frame start                   */
     uint16_t    len;            /**< Current data length                   */
 
     uintptr_t   paddr;          /**< Physical address of data (for DMA)    */
     uint8_t     type;           /**< uiox_wifi_frame_type_t                */
     uint8_t     ac;             /**< WMM access category (0..3)            */
     int8_t      rssi_dbm;       /**< RX RSSI (filled by driver)            */
     uint8_t     rate_idx;       /**< TX rate index                         */
     bool        ampdu;          /**< Part of A-MPDU aggregation            */
     bool        encrypted;      /**< Payload is encrypted                  */
     uint64_t    tsf;            /**< TSF timestamp (µs)                    */
     uint8_t     in_use;         /**< Reference count                       */
 
     struct uiox_wifi_frame *next;
 } uiox_wifi_frame_t;
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 void               uiox_wifi_buf_init    (void);
 uiox_wifi_frame_t *uiox_wifi_buf_alloc_tx(void);
 uiox_wifi_frame_t *uiox_wifi_buf_alloc_rx(void);
 void               uiox_wifi_buf_ref     (uiox_wifi_frame_t *f);
 void               uiox_wifi_buf_free    (uiox_wifi_frame_t *f);
 
 /** Prepend `len` bytes (zero-copy header push). */
 void *uiox_wifi_buf_push(uiox_wifi_frame_t *f, uint16_t len);
 
 /** Strip `len` bytes from front (header pull). */
 void *uiox_wifi_buf_pull(uiox_wifi_frame_t *f, uint16_t len);
 
 /** Append `len` bytes at tail. */
 void *uiox_wifi_buf_put (uiox_wifi_frame_t *f, uint16_t len);
 
 uint16_t uiox_wifi_buf_tx_free(void);
 uint16_t uiox_wifi_buf_rx_free(void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_WIFI_BUF_H */
 