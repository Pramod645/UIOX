/**
 * @file    uiox_wifi_if.h
 * @brief   UIOX WiFi interface driver (TX/RX queue, rate control).
 *
 * Sits between HAL and protocol layer. Manages:
 *   - WMM AC TX queues (VO/VI/BE/BK — 4 priority queues)
 *   - RX frame demultiplexing (data/mgmt/ctrl)
 *   - Rate control (ARF / MINSTREL stub)
 *   - A-MPDU aggregation scheduling
 *   - Interface statistics
 *
 * @date    2026-05-28
 */
//Layer 2 — Interface Driver
 #ifndef UIOX_WIFI_IF_H
 #define UIOX_WIFI_IF_H
 
 #include "uiox_wifi_hw.h"
 #include "uiox_wifi_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * WMM Access Categories
  * ====================================================================== */
 
 #define UIOX_WIFI_AC_BK     0   /**< Background                           */
 #define UIOX_WIFI_AC_BE     1   /**< Best effort                          */
 #define UIOX_WIFI_AC_VI     2   /**< Video                                */
 #define UIOX_WIFI_AC_VO     3   /**< Voice                                */
 #define UIOX_WIFI_AC_COUNT  4
 
 /* =========================================================================
  * TX queue per access category
  * ====================================================================== */
 
 #define UIOX_WIFI_TXQUEUE_DEPTH  16
 
 typedef struct {
     uiox_wifi_frame_t *frames[UIOX_WIFI_TXQUEUE_DEPTH];
     uint16_t           head;
     uint16_t           tail;
     uint16_t           count;
     uint32_t           dropped;
 } uiox_wifi_txq_t;
 
 /* =========================================================================
  * Interface statistics
  * ====================================================================== */
 
 typedef struct {
     uint64_t  tx_frames;
     uint64_t  tx_bytes;
     uint64_t  tx_errors;
     uint64_t  tx_dropped;
     uint64_t  tx_retries;
     uint64_t  rx_frames;
     uint64_t  rx_bytes;
     uint64_t  rx_errors;
     uint64_t  rx_dropped;
     uint64_t  rx_mgmt;
     int8_t    last_rssi_dbm;
     uint8_t   tx_rate_idx;
 } uiox_wifi_if_stats_t;
 
 /* =========================================================================
  * Rate control state (ARF-style: Auto Rate Fallback)
  * ====================================================================== */
 
 #define UIOX_WIFI_RATE_TABLE_MAX  32
 
 typedef struct {
     uint8_t   rate_idx;         /**< Current TX rate index                 */
     uint8_t   max_rate_idx;     /**< Highest supported rate                */
     uint16_t  success_count;    /**< Consecutive TX successes              */
     uint16_t  fail_count;       /**< Consecutive TX failures               */
     uint16_t  probe_interval;   /**< Probe higher rate every N successes   */
 } uiox_wifi_rate_ctrl_t;
 
 /* =========================================================================
  * Interface descriptor
  * ====================================================================== */
 
 typedef struct {
     uiox_wifi_hw_t        *hw;
     uiox_wifi_txq_t        txq[UIOX_WIFI_AC_COUNT];
     uiox_wifi_rate_ctrl_t  rate;
     uiox_wifi_if_stats_t   stats;
     bool                   primed;
 } uiox_wifi_if_t;
 
 /* =========================================================================
  * Interface API
  * ====================================================================== */
 
 int  uiox_wifi_if_config  (uiox_wifi_if_t *wif, uiox_wifi_hw_t *hw);
 
 /** Enqueue frame for TX on given AC. */
 int  uiox_wifi_if_tx      (uiox_wifi_if_t *wif,
                             uiox_wifi_frame_t *frame, uint8_t ac);
 
 /** Flush pending TX frames from SW queues to HW. */
 void uiox_wifi_if_tx_flush(uiox_wifi_if_t *wif);
 
 /** Poll for received frame. Returns NULL if none. */
 uiox_wifi_frame_t *uiox_wifi_if_rx(uiox_wifi_if_t *wif);
 
 /** Update rate control after TX completion. */
 void uiox_wifi_if_rate_update(uiox_wifi_if_t *wif, bool success);
 
 void uiox_wifi_if_stats_get  (const uiox_wifi_if_t *wif,
                                uiox_wifi_if_stats_t *out);
 void uiox_wifi_if_stats_reset(uiox_wifi_if_t *wif);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_WIFI_IF_H */
 