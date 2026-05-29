/**
 * @file    uiox_can_proto.h
 * @brief   UIOX CAN protocol layer.
 *
 * Implements:
 *   - CANopen SDO (Service Data Object) — expedited read/write
 *   - CANopen PDO (Process Data Object) — TX/RX mapping
 *   - NMT master commands (start/stop/reset all nodes)
 *   - EMCY (Emergency) message handling
 *   - Bus-off recovery state machine
 *   - CRC validation for CAN-FD
 *
 * @date    2026-05-26
 */
//Layer 3 — Protocol
 #ifndef UIOX_CAN_PROTO_H
 #define UIOX_CAN_PROTO_H
 
 #include "uiox_can_node.h"
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * CANopen COB-ID base addresses
  * ====================================================================== */
 
 #define UIOX_CAN_COB_NMT        0x000u  /**< NMT master command            */
 #define UIOX_CAN_COB_SYNC       0x080u  /**< SYNC object                   */
 #define UIOX_CAN_COB_EMCY_BASE  0x080u  /**< Emergency (+ node_id)         */
 #define UIOX_CAN_COB_TPDO1_BASE 0x180u  /**< TPDO1 (+ node_id)            */
 #define UIOX_CAN_COB_RPDO1_BASE 0x200u  /**< RPDO1 (+ node_id)            */
 #define UIOX_CAN_COB_TPDO2_BASE 0x280u  /**< TPDO2 (+ node_id)            */
 #define UIOX_CAN_COB_RPDO2_BASE 0x300u  /**< RPDO2 (+ node_id)            */
 #define UIOX_CAN_COB_TSDO_BASE  0x580u  /**< TSDO  (+ node_id)            */
 #define UIOX_CAN_COB_RSDO_BASE  0x600u  /**< RSDO  (+ node_id)            */
 
 /* =========================================================================
  * SDO command specifiers (cs field, bits [7:5])
  * ====================================================================== */
 
 #define UIOX_SDO_CS_WR_EXP      0x20u   /**< Expedited download (write)    */
 #define UIOX_SDO_CS_WR_ACK      0x60u   /**< Download response             */
 #define UIOX_SDO_CS_RD_REQ      0x40u   /**< Upload request (read)         */
 #define UIOX_SDO_CS_RD_RSP      0x40u   /**< Upload response               */
 #define UIOX_SDO_CS_ABORT       0x80u   /**< Abort transfer                */
 
 /* =========================================================================
  * EMCY error codes
  * ====================================================================== */
 
 #define UIOX_CAN_EMCY_NO_ERR    0x0000u /**< Error reset / no error        */
 #define UIOX_CAN_EMCY_GENERIC   0x1000u /**< Generic error                 */
 #define UIOX_CAN_EMCY_COMM      0x8100u /**< CAN overrun                   */
 #define UIOX_CAN_EMCY_BUS_OFF   0x8140u /**< CAN bus off                   */
 #define UIOX_CAN_EMCY_HEARTBEAT 0x8130u /**< Heartbeat timeout             */
 
 /* =========================================================================
  * PDO mapping entry
  * ====================================================================== */
 
 #define UIOX_CAN_PDO_MAX_MAPS   8
 
 typedef struct {
     uint16_t  index;       /**< Object dictionary index                     */
     uint8_t   subindex;    /**< Object dictionary sub-index                 */
     uint8_t   bits;        /**< Mapped bits (8/16/32)                       */
     uint8_t   offset;      /**< Byte offset in PDO frame                   */
 } uiox_can_pdo_map_t;
 
 typedef struct {
     uint32_t          cob_id;
     uint32_t          period_ms;     /**< Transmission period (0=event)    */
     bool              ext;
     uint8_t           n_maps;
     uiox_can_pdo_map_t maps[UIOX_CAN_PDO_MAX_MAPS];
 } uiox_can_pdo_t;
 
 /* =========================================================================
  * Bus-off recovery config
  * ====================================================================== */
 
 typedef struct {
     uint32_t  retry_delay_ms;    /**< Delay before recovery attempt        */
     uint8_t   max_retries;       /**< 0 = retry forever                    */
 } uiox_can_busoff_cfg_t;
 
 /* =========================================================================
  * Protocol context
  * ====================================================================== */
 
 typedef struct {
     uiox_can_node_t       *node;
     uiox_can_busoff_cfg_t  busoff_cfg;
     uint8_t                busoff_retries;
     uint32_t               busoff_ts_ms;
     bool                   busoff_pending;
     uint32_t               sync_period_ms;
     uint32_t               last_sync_ms;
 } uiox_can_proto_t;
 
 /* =========================================================================
  * Protocol API
  * ====================================================================== */
 
 int  uiox_can_proto_init   (uiox_can_proto_t         *proto,
                              uiox_can_node_t          *node,
                              const uiox_can_busoff_cfg_t *bo_cfg,
                              uint32_t                  sync_period_ms);
 
 /** Send NMT command to a specific node (0 = broadcast). */
 int  uiox_can_proto_nmt_cmd(uiox_can_proto_t *proto,
                              uint8_t           node_id,
                              uint8_t           cmd);
 
/** SDO expedited write to remote node. */
int  uiox_can_proto_sdo_write(uiox_can_proto_t *proto,
    uint8_t  node_id,
    uint16_t index,
    uint8_t  subindex,
    const uint8_t *data,
    uint8_t  len);

/** SDO expedited read from remote node. */
int  uiox_can_proto_sdo_read (uiox_can_proto_t *proto,
    uint8_t  node_id,
    uint16_t index,
    uint8_t  subindex,
    uint8_t *data_out,
    uint8_t *len_out);

/** Send EMCY message from this node. */
int  uiox_can_proto_emcy     (uiox_can_proto_t *proto,
    uint16_t err_code,
    uint8_t  err_reg,
    const uint8_t *mspec,
    uint8_t  mspec_len);

/** Send SYNC message (master only). */
int  uiox_can_proto_sync     (uiox_can_proto_t *proto);

/** Send a PDO frame. */
int  uiox_can_proto_pdo_tx   (uiox_can_proto_t     *proto,
    const uiox_can_pdo_t *pdo,
    const uint8_t        *data,
    uint8_t               len);

/**
* @brief  Periodic tick — drives bus-off recovery, SYNC, heartbeat.
* @param  now_ms  Monotonic time in milliseconds.
*/
void uiox_can_proto_tick     (uiox_can_proto_t *proto, uint32_t now_ms);

/**
* @brief  Handle incoming message — dispatch to SDO/PDO/NMT/EMCY handlers.
*/
void uiox_can_proto_rx       (uiox_can_proto_t *proto,
    uiox_can_msg_t   *msg);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_CAN_PROTO_H */

 