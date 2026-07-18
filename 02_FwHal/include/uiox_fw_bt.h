/**
 * @file  uiox_fw_bt.h
 * @brief UIOX FwHal — Bluetooth HCI transport (UART / USB HCI).
 *        Firmware role: power-on chip, load firmware patch, open HCI.
 */

 #ifndef UIOX_FW_BT_H
 #define UIOX_FW_BT_H
 
 #include "uiox_fw_types.h"
 #include "uiox_fw_uart.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef enum {
     UIOX_BT_TRANSPORT_UART = 0,
     UIOX_BT_TRANSPORT_USB  = 1,
     UIOX_BT_TRANSPORT_SDIO = 2,
 } uiox_bt_transport_t;
 
 /* ── HCI packet types ────────────────────────────────────────── */
 #define HCI_CMD_PKT     0x01u
 #define HCI_ACL_PKT     0x02u
 #define HCI_SCO_PKT     0x03u
 #define HCI_EVT_PKT     0x04u
 
 /* ── HCI command opcodes (OGF:OCF packed) ────────────────────── */
 #define HCI_OP_RESET                0x0C03u
 #define HCI_OP_READ_LOCAL_VER       0x1001u
 #define HCI_OP_READ_BD_ADDR         0x1009u
 #define HCI_OP_SET_EVENT_MASK       0x0C01u
 #define HCI_OP_WRITE_SCAN_EN        0x0C1Au
 #define HCI_OP_LE_SET_ADV_PARAMS    0x2006u
 #define HCI_OP_LE_SET_ADV_DATA      0x2008u
 #define HCI_OP_LE_SET_ADV_EN        0x200Au
 
 /* ── HCI UART 3-wire (H4) packet ─────────────────────────────── */
 #define UIOX_BT_MAX_PACKET  260u
 
 typedef struct {
     //uiox_uart_hw_t      *uart;       /**< For UART transport           */
     uiox_bt_transport_t  transport;
     uint8_t              bd_addr[6]; /**< Local Bluetooth address      */
     uint32_t             gpio_reset; /**< Reset GPIO pin               */
     uint32_t             gpio_pwren; /**< Power-enable GPIO pin        */
     bool                 fw_loaded;  /**< Vendor firmware patch loaded */
     bool                 initialized;
     /* Rx state machine */
     uint8_t              rx_buf[UIOX_BT_MAX_PACKET];
     uint32_t             rx_len;
     void                *priv;
 } uiox_bt_dev_t;
 
 typedef struct {
     uiox_fw_err_t (*init)       (uiox_bt_dev_t *dev);
     void          (*deinit)     (uiox_bt_dev_t *dev);
     uiox_fw_err_t (*reset)      (uiox_bt_dev_t *dev);
     uiox_fw_err_t (*send_hci)   (uiox_bt_dev_t *dev,
                                    const uint8_t *pkt, uint32_t len);
     int32_t       (*recv_hci)   (uiox_bt_dev_t *dev,
                                    uint8_t *buf, uint32_t max_len);
     uiox_fw_err_t (*load_fw)    (uiox_bt_dev_t *dev,
                                    const uint8_t *fw, uint32_t len);
     void          (*isr)        (uiox_bt_dev_t *dev);
 } uiox_bt_ops_t;
 
 uiox_fw_err_t uiox_fw_bt_init     (uiox_bt_dev_t *dev,
                                      const uiox_bt_ops_t *ops);
 void          uiox_fw_bt_deinit   (uiox_bt_dev_t *dev);
 uiox_fw_err_t uiox_fw_bt_reset    (uiox_bt_dev_t *dev);
 uiox_fw_err_t uiox_fw_bt_send_hci (uiox_bt_dev_t *dev,
                                      const uint8_t *pkt, uint32_t len);
 int32_t       uiox_fw_bt_recv_hci (uiox_bt_dev_t *dev,
                                      uint8_t *buf, uint32_t max_len);
 uiox_fw_err_t uiox_fw_bt_init_uart(uiox_bt_dev_t *dev,
                                      /*uiox_uart_hw_t *uart, */
                                      uint32_t gpio_reset,
                                      uint32_t gpio_pwren);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_BT_H */
 