/**
 * @file    uiox_can_if.h
 * @brief   UIOX CAN interface driver (TX/RX FIFO, acceptance filters).
 *
 * Sits between HAL and protocol layer. Manages:
 *   - TX message submission to hardware FIFO
 *   - RX message retrieval from hardware FIFO
 *   - Acceptance filter programming (up to 32 filters)
 *   - TX/RX statistics counters
 *   - Bit-timing setup
 *
 * @date    2026-05-26
 */
//Layer 2 — Interface Driver

 #ifndef UIOX_CAN_IF_H
 #define UIOX_CAN_IF_H
 
 #include "uiox_can_hw.h"
 #include "uiox_can_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_CAN_MAX_FILTERS    32   /**< Max hardware acceptance filters   */
 #define UIOX_CAN_IF_TX_DEPTH    16   /**< Software TX queue depth           */
 #define UIOX_CAN_IF_RX_DEPTH    32   /**< Software RX queue depth           */
 
 /* =========================================================================
  * Acceptance filter entry
  * ====================================================================== */
 
 typedef struct {
     uint32_t  id;        /**< Filter ID (11-bit or 29-bit)                  */
     uint32_t  mask;      /**< Filter mask (0=don't care, 1=must match)      */
     bool      ext;       /**< true = extended 29-bit ID, false = standard   */
     bool      enabled;
 } uiox_can_filter_t;
 
 /* =========================================================================
  * Interface statistics
  * ====================================================================== */
 
 typedef struct {
     uint64_t  tx_frames;
     uint64_t  tx_bytes;
     uint64_t  tx_errors;
     uint64_t  tx_dropped;
     uint64_t  rx_frames;
     uint64_t  rx_bytes;
     uint64_t  rx_errors;
     uint64_t  rx_dropped;
     uint64_t  rx_overflows;
 } uiox_can_if_stats_t;
 
 /* =========================================================================
  * Interface descriptor
  * ====================================================================== */
 
 typedef struct {
     uiox_can_hw_t        *hw;
     uint8_t               channel;       /**< CAN bus channel index          */
     bool                  fd_enabled;
     uiox_can_filter_t     filters[UIOX_CAN_MAX_FILTERS];
     uint8_t               filter_count;
     uiox_can_if_stats_t   stats;
 
     /* Software TX queue (ring buffer) */
     uiox_can_msg_t       *tx_queue[UIOX_CAN_IF_TX_DEPTH];
     uint16_t              tx_q_head;
     uint16_t              tx_q_tail;
     uint16_t              tx_q_count;
 
     /* Software RX queue (ring buffer) */
     uiox_can_msg_t       *rx_queue[UIOX_CAN_IF_RX_DEPTH];
     uint16_t              rx_q_head;
     uint16_t              rx_q_tail;
     uint16_t              rx_q_count;
    } uiox_can_if_t;

    /* =========================================================================
     * Interface API
     * ====================================================================== */
    
    int  uiox_can_if_config    (uiox_can_if_t *cif,
                                 uiox_can_hw_t *hw,
                                 uint8_t        channel,
                                 bool           fd_enabled,
                                 uint32_t       nom_bitrate,
                                 uint32_t       data_bitrate);
    
    int  uiox_can_if_add_filter(uiox_can_if_t *cif,
                                 uint32_t id, uint32_t mask, bool ext);
    void uiox_can_if_clr_filters(uiox_can_if_t *cif);
    
    int  uiox_can_if_tx        (uiox_can_if_t *cif, uiox_can_msg_t *msg);
    uiox_can_msg_t *uiox_can_if_rx(uiox_can_if_t *cif);
    
    void uiox_can_if_tx_flush  (uiox_can_if_t *cif);
    void uiox_can_if_stats_get (const uiox_can_if_t *cif,
                                 uiox_can_if_stats_t *out);
    void uiox_can_if_stats_reset(uiox_can_if_t *cif);
    
    #ifdef __cplusplus
    }
    #endif
    #endif /* UIOX_CAN_IF_H */
    
 