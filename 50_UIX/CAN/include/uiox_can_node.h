/**
 * @file    uiox_can_node.h
 * @brief   UIOX CAN node abstraction (mailbox, heartbeat, NMT).
 *
 * Represents a logical CAN node (ECU, sensor, actuator) on the bus.
 * Manages:
 *   - Node ID and network address
 *   - Mailbox registration (TX/RX message objects)
 *   - Periodic heartbeat / keep-alive transmission
 *   - NMT-lite state machine (INIT → PRE-OP → OPERATIONAL → STOPPED)
 *
 * @date    2026-05-26
 */
//Layer 2b — Node Abstraction
 #ifndef UIOX_CAN_NODE_H
 #define UIOX_CAN_NODE_H
 
 #include "uiox_can_if.h"
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_CAN_MAX_MAILBOXES   16    /**< Max mailboxes per node          */
 #define UIOX_CAN_NODE_ID_MAX     127   /**< Max CANopen node ID             */
 #define UIOX_CAN_HEARTBEAT_COB   0x700 /**< CANopen heartbeat COB-ID base   */
 
 /* =========================================================================
  * NMT state
  * ====================================================================== */
 
 typedef enum {
     UIOX_CAN_NMT_INIT        = 0x00,
     UIOX_CAN_NMT_STOPPED     = 0x04,
     UIOX_CAN_NMT_OPERATIONAL = 0x05,
     UIOX_CAN_NMT_PRE_OP      = 0x7F,
 } uiox_can_nmt_state_t;
 
 /* =========================================================================
  * Mailbox direction
  * ====================================================================== */
 
 typedef enum {
     UIOX_CAN_MB_TX = 0,
     UIOX_CAN_MB_RX,
 } uiox_can_mb_dir_t;
 
 /* =========================================================================
  * Mailbox (message object)
  * ====================================================================== */
 
 typedef struct {
     uint32_t           cob_id;    /**< Communication object identifier      */
     uint32_t           mask;      /**< ID mask for RX matching              */
     uiox_can_mb_dir_t  dir;
     uint32_t           period_ms; /**< TX period (0 = event-driven)         */
     uint32_t           last_tx_ms;/**< Timestamp of last TX (ms)            */
     bool               ext;       /**< Extended (29-bit) frame              */
     bool               enabled;
     uint8_t            dlc;       /**< Fixed DLC (0 = dynamic)              */
 
     /** RX callback — called when a matching message is received. */
     void (*rx_cb)(uint32_t cob_id, const uint8_t *data,
                   uint8_t len, void *ctx);
     void *cb_ctx;
 } uiox_can_mailbox_t;
 
 /* =========================================================================
  * Node descriptor
  * ====================================================================== */
 
 typedef struct {
     uint8_t               node_id;
     const char           *name;          /**< e.g. "BMS", "MOTOR_CTRL"     */
     uiox_can_nmt_state_t  nmt_state;
     uiox_can_if_t        *cif;
     uiox_can_mailbox_t    mailboxes[UIOX_CAN_MAX_MAILBOXES];
     uint8_t               mb_count;
     uint32_t              heartbeat_ms;  /**< Heartbeat interval (0=off)    */
     uint32_t              last_hb_ms;    /**< Last heartbeat TX time        */
 } uiox_can_node_t;
 
 /* =========================================================================
  * Node API
  * ====================================================================== */
 
 int  uiox_can_node_init    (uiox_can_node_t *node,
                              uiox_can_if_t   *cif,
                              uint8_t          node_id,
                              const char      *name,
                              uint32_t         heartbeat_ms);
 
 int  uiox_can_node_add_mb  (uiox_can_node_t     *node,
                              const uiox_can_mailbox_t *mb);
 
 int  uiox_can_node_tx      (uiox_can_node_t *node,
                              uint32_t         cob_id,
                              const uint8_t   *data,
                              uint8_t          len,
                              bool             ext);
 
 void uiox_can_node_tick    (uiox_can_node_t *node, uint32_t now_ms);
 void uiox_can_node_dispatch(uiox_can_node_t *node, uiox_can_msg_t *msg);
 int  uiox_can_node_nmt     (uiox_can_node_t *node,
                              uiox_can_nmt_state_t state);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_CAN_NODE_H */
 