/**
 * @file    uiox_bt_hw.h
 * @brief   UIOX Bluetooth Hardware Abstraction Layer (HAL).
 *
 * Supports:
 *   - Intel AX210 / AX211 (Wi-Fi 6E + BT 5.3, PCIe/CNVi)
 *   - Qualcomm WCN6855 (BT 5.2, USB/UART)
 *   - Broadcom BCM4389 (BT 5.3, UART/SDIO)
 *   - Nordic nRF52840 (BT 5.4, USB dongle)
 *   - Cambridge Silicon Radio CSR8510 (classic BT, USB)
 *
 * Owns:
 *   - UART/USB serial HCI transport read/write
 *   - BT_EN / BT_WAKE / HOST_WAKE GPIO
 *   - Controller firmware download
 *   - Power management (sleep/wake)
 *   - IRQ: HOST_WAKE (async wake from controller)
 *
 * @version 1.0.0
 * @date    2026-06-09
 */

 #ifndef UIOX_BT_HW_H
 #define UIOX_BT_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Bluetooth version
  * ====================================================================== */
 
 typedef enum {
     UIOX_BT_VER_BT40 = 0,  /**< Bluetooth 4.0 (LE Basic)               */
     UIOX_BT_VER_BT41,       /**< Bluetooth 4.1                          */
     UIOX_BT_VER_BT42,       /**< Bluetooth 4.2 (LE Secure Connections)  */
     UIOX_BT_VER_BT50,       /**< Bluetooth 5.0 (2 Mbit/s LE PHY)       */
     UIOX_BT_VER_BT51,       /**< Bluetooth 5.1 (AoA/AoD)               */
     UIOX_BT_VER_BT52,       /**< Bluetooth 5.2 (LE Audio/Isochronous)   */
     UIOX_BT_VER_BT53,       /**< Bluetooth 5.3                          */
     UIOX_BT_VER_BT54,       /**< Bluetooth 5.4 (PAwR, EATT)            */
 } uiox_bt_ver_t;
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_BT_CAP_CLASSIC       (1u << 0)  /**< BR/EDR (Classic BT)   */
 #define UIOX_BT_CAP_BLE           (1u << 1)  /**< Bluetooth Low Energy  */
 #define UIOX_BT_CAP_BLE_AUDIO     (1u << 2)  /**< BT 5.2 LE Audio/ISO   */
 #define UIOX_BT_CAP_A2DP          (1u << 3)  /**< A2DP audio streaming  */
 #define UIOX_BT_CAP_HFP           (1u << 4)  /**< Hands-Free Profile    */
 #define UIOX_BT_CAP_AVRCP         (1u << 5)  /**< AV Remote Control     */
 #define UIOX_BT_CAP_HID           (1u << 6)  /**< Human Interface Dev   */
 #define UIOX_BT_CAP_SPP           (1u << 7)  /**< Serial Port Profile   */
 #define UIOX_BT_CAP_2M_PHY        (1u << 8)  /**< BLE 2 Mbit/s PHY      */
 #define UIOX_BT_CAP_CODED_PHY     (1u << 9)  /**< BLE Coded PHY (LR)    */
 #define UIOX_BT_CAP_AOA           (1u << 10) /**< Angle of Arrival      */
 #define UIOX_BT_CAP_AOD           (1u << 11) /**< Angle of Departure    */
 #define UIOX_BT_CAP_PERIODIC_ADV  (1u << 12) /**< Periodic advertising  */
 #define UIOX_BT_CAP_EXT_ADV       (1u << 13) /**< Extended advertising  */
 #define UIOX_BT_CAP_COEX          (1u << 14) /**< Wi-Fi coexistence     */
 #define UIOX_BT_CAP_FW_DOWNLOAD   (1u << 15) /**< Runtime FW download   */
 
 /* =========================================================================
  * Transport interface type
  * ====================================================================== */
 
 typedef enum {
     UIOX_BT_IF_UART = 0,
     UIOX_BT_IF_USB,
     UIOX_BT_IF_SDIO,
     UIOX_BT_IF_SPI,
     UIOX_BT_IF_CNVI,   /**< Intel CNVi (PCIe integrated)                */
 } uiox_bt_if_type_t;
 
 /* =========================================================================
  * HCI packet types
  * ====================================================================== */
 
 #define HCI_CMD_PKT         0x01u
 #define HCI_ACL_PKT         0x02u
 #define HCI_SCO_PKT         0x03u
 #define HCI_EVENT_PKT       0x04u
 #define HCI_ISO_PKT         0x05u
 
 /* HCI event codes */
 #define HCI_EV_INQUIRY_COMPLETE         0x01u
 #define HCI_EV_INQUIRY_RESULT           0x02u
 #define HCI_EV_CONN_COMPLETE            0x03u
 #define HCI_EV_DISCONN_COMPLETE         0x05u
 #define HCI_EV_AUTH_COMPLETE            0x06u
 #define HCI_EV_REMOTE_NAME              0x07u
 #define HCI_EV_CMD_COMPLETE             0x0Eu
 #define HCI_EV_CMD_STATUS               0x0Fu
 #define HCI_EV_ROLE_CHANGE              0x12u
 #define HCI_EV_LE_META                  0x3Eu
 #define HCI_EV_VENDOR                   0xFFu
 
 /* HCI LE meta subevent codes */
 #define HCI_LE_EV_CONN_COMPLETE         0x01u
 #define HCI_LE_EV_ADV_REPORT            0x02u
 #define HCI_LE_EV_CONN_UPDATE           0x03u
 #define HCI_LE_EV_LONG_TERM_KEY_REQ     0x05u
 #define HCI_LE_EV_EXT_ADV_REPORT        0x0Du
 #define HCI_LE_EV_PERIODIC_ADV_SYNC     0x0Eu
 
 /* HCI opcodes (OGF << 10 | OCF) */
 #define HCI_OP_RESET                    0x0C03u
 #define HCI_OP_READ_BD_ADDR             0x1009u
 #define HCI_OP_READ_LOCAL_VER           0x1001u
 #define HCI_OP_READ_LOCAL_FEATURES      0x1003u
 #define HCI_OP_SET_EVENT_MASK           0x0C01u
 #define HCI_OP_WRITE_LOCAL_NAME         0x0C13u
 #define HCI_OP_INQUIRY                  0x0401u
 #define HCI_OP_INQUIRY_CANCEL           0x0402u
 #define HCI_OP_ACCEPT_CONN              0x0409u
 #define HCI_OP_REJECT_CONN              0x040Au
 #define HCI_OP_LE_SET_ADV_PARAMS        0x2006u
 #define HCI_OP_LE_SET_ADV_DATA          0x2008u
 #define HCI_OP_LE_SET_ADV_ENABLE        0x200Au
 #define HCI_OP_LE_SET_SCAN_PARAMS       0x200Bu
 #define HCI_OP_LE_SET_SCAN_ENABLE       0x200Cu
 #define HCI_OP_LE_CREATE_CONN           0x200Du
 #define HCI_OP_LE_READ_BD_ADDR          0x2009u
 #define HCI_OP_LE_SET_EXT_ADV_PARAMS   0x2036u
 #define HCI_OP_LE_SET_EXT_ADV_ENABLE   0x2039u
 #define HCI_OP_LE_SET_EXT_SCAN_PARAMS  0x2041u
 #define HCI_OP_LE_SET_EXT_SCAN_ENABLE  0x2042u
 #define HCI_OP_LE_EXT_CREATE_CONN      0x2043u
 #define HCI_OP_LE_READ_ISO_TX_SYNC     0x2061u
 #define HCI_OP_WRITE_SCAN_ENABLE        0x0C1Au
 #define HCI_OP_WRITE_AUTH_ENABLE        0x0C20u
 #define HCI_OP_WRITE_CLASS_OF_DEV       0x0C24u
 #define HCI_OP_WRITE_SIMPLE_PAIR_MODE   0x0C56u
 
 /* =========================================================================
  * Bluetooth device address
  * ====================================================================== */
 
 #define UIOX_BT_ADDR_LEN    6
 
 typedef uint8_t uiox_bt_addr_t[UIOX_BT_ADDR_LEN];
 
 /* =========================================================================
  * HCI packet header
  * ====================================================================== */
 
 typedef struct __attribute__((packed)) {
     uint8_t  type;          /**< HCI_*_PKT                                */
     uint16_t opcode;        /**< For CMD packets                          */
     uint8_t  param_len;     /**< For CMD/EVENT                            */
 } uiox_hci_cmd_hdr_t;
 
 typedef struct __attribute__((packed)) {
     uint8_t  type;
     uint8_t  event;
     uint8_t  param_len;
 } uiox_hci_ev_hdr_t;
 
 typedef struct __attribute__((packed)) {
     uint8_t  type;
     uint16_t handle_flags;  /**< Connection handle + PB/BC flags          */
     uint16_t data_len;
 } uiox_hci_acl_hdr_t;
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 #define UIOX_BT_MODEL_LEN       48
 #define UIOX_BT_FW_VER_LEN      32
 #define UIOX_BT_HCI_BUF_MAX     512
 
 typedef struct {
     uintptr_t           uart_base;    /**< UART MMIO base                  */
     uint32_t            uart_baud;    /**< e.g. 115200 / 3000000           */
     uint32_t            irq;          /**< HOST_WAKE IRQ                   */
     uint32_t            caps;
     uiox_bt_if_type_t   if_type;
     uiox_bt_ver_t       version;
     char                model[UIOX_BT_MODEL_LEN];
     char                fw_version[UIOX_BT_FW_VER_LEN];
     uiox_bt_addr_t      bd_addr;
 
     /* GPIO */
     uint32_t            bt_en_pin;    /**< BT enable GPIO                  */
     uint32_t            bt_wake_pin;  /**< BT wake GPIO (host→ctrl)       */
     uint32_t            host_wake_pin;/**< Host wake GPIO (ctrl→host)     */
 
     /* HCI receive accumulation */
     uint8_t             rx_buf[UIOX_BT_HCI_BUF_MAX];
     uint16_t            rx_len;
     volatile bool       rx_pending;
     volatile bool       host_wake_pending;
 
     bool                powered;
     bool                initialised;
 
     void               *priv;
 } uiox_bt_hw_t;
 
 /* =========================================================================
  * Hardware operations vtable
  * ====================================================================== */
 
 typedef struct {
     int  (*init)          (uiox_bt_hw_t *hw);
     void (*deinit)        (uiox_bt_hw_t *hw);
     int  (*power)         (uiox_bt_hw_t *hw, bool on);
     int  (*fw_download)   (uiox_bt_hw_t *hw,
                            const uint8_t *fw, uint32_t fw_size);
     int  (*hci_write)     (uiox_bt_hw_t *hw,
                            const uint8_t *buf, uint16_t len);
     int  (*hci_read)      (uiox_bt_hw_t *hw,
                            uint8_t *buf, uint16_t max_len);
     void (*set_baud)      (uiox_bt_hw_t *hw, uint32_t baud);
     void (*gpio_write)    (uiox_bt_hw_t *hw, uint32_t pin, bool val);
     bool (*gpio_read)     (uiox_bt_hw_t *hw, uint32_t pin);
     void (*delay_ms)      (uiox_bt_hw_t *hw, uint32_t ms);
     void (*isr_host_wake) (uiox_bt_hw_t *hw);
     void (*isr_rx)        (uiox_bt_hw_t *hw);
 } uiox_bt_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int  uiox_bt_hw_init      (uiox_bt_hw_t *hw, const uiox_bt_hw_ops_t *ops);
 void uiox_bt_hw_deinit    (uiox_bt_hw_t *hw);
 int  uiox_bt_hw_power     (uiox_bt_hw_t *hw, bool on);
 int  uiox_bt_hw_hci_write (uiox_bt_hw_t *hw,
                             const uint8_t *buf, uint16_t len);
 int  uiox_bt_hw_hci_read  (uiox_bt_hw_t *hw,
                             uint8_t *buf, uint16_t max_len);
 int  uiox_bt_hw_fw_download(uiox_bt_hw_t *hw,
                              const uint8_t *fw, uint32_t size);
 
 static inline uint32_t uiox_bt_caps(const uiox_bt_hw_t *hw)
 { return hw ? hw->caps : 0u; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BT_HW_H */
 