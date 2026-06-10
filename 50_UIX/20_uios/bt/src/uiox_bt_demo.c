/* uiox_bt_demo.c */
#include "uiox_bt_device.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

/* Simulated HCI receive buffer */
static uint8_t  s_hci_rx[UIOX_BT_HCI_BUF_MAX];
static uint16_t s_hci_rx_len = 0;
static uint32_t s_sim_step   = 0;

/* Simulate different HCI responses per step */
static void prepare_sim_response(uint16_t opcode)
{
    s_hci_rx_len = 0;
    memset(s_hci_rx, 0, sizeof(s_hci_rx));

    if (opcode == HCI_OP_RESET ||
        opcode == HCI_OP_WRITE_LOCAL_NAME ||
        opcode == HCI_OP_WRITE_SCAN_ENABLE ||
        opcode == HCI_OP_WRITE_SIMPLE_PAIR_MODE ||
        opcode == HCI_OP_LE_SET_ADV_PARAMS ||
        opcode == HCI_OP_LE_SET_ADV_DATA ||
        opcode == HCI_OP_LE_SET_ADV_ENABLE ||
        opcode == HCI_OP_LE_SET_SCAN_PARAMS ||
        opcode == HCI_OP_LE_SET_SCAN_ENABLE ||
        opcode == HCI_OP_LE_CREATE_CONN) {
        /* Generic CMD_COMPLETE with status=OK */
        s_hci_rx[0] = HCI_EVENT_PKT;
        s_hci_rx[1] = HCI_EV_CMD_COMPLETE;
        s_hci_rx[2] = 4u;                       /* param len */
        s_hci_rx[3] = 1u;                        /* num cmds */
        s_hci_rx[4] = (uint8_t)(opcode & 0xFFu);
        s_hci_rx[5] = (uint8_t)(opcode >> 8u);
        s_hci_rx[6] = 0x00u;                     /* status OK */
        s_hci_rx_len = 7u;
    } else if (opcode == HCI_OP_READ_BD_ADDR) {
        /* CMD_COMPLETE with BD addr */
        s_hci_rx[0] = HCI_EVENT_PKT;
        s_hci_rx[1] = HCI_EV_CMD_COMPLETE;
        s_hci_rx[2] = 10u;
        s_hci_rx[3] = 1u;
        s_hci_rx[4] = (uint8_t)(opcode & 0xFFu);
        s_hci_rx[5] = (uint8_t)(opcode >> 8u);
        s_hci_rx[6] = 0x00u;  /* status OK */
        /* BD addr: 11:22:33:44:55:66 */
        s_hci_rx[7] = 0x11u; s_hci_rx[8] = 0x22u;
        s_hci_rx[9] = 0x33u; s_hci_rx[10]= 0x44u;
        s_hci_rx[11]= 0x55u; s_hci_rx[12]= 0x66u;
        s_hci_rx_len = 13u;
    } else if (opcode == HCI_OP_READ_LOCAL_VER) {
        /* CMD_COMPLETE with version info */
        s_hci_rx[0] = HCI_EVENT_PKT;
        s_hci_rx[1] = HCI_EV_CMD_COMPLETE;
        s_hci_rx[2] = 12u;
        s_hci_rx[3] = 1u;
        s_hci_rx[4] = (uint8_t)(opcode & 0xFFu);
        s_hci_rx[5] = (uint8_t)(opcode >> 8u);
        s_hci_rx[6] = 0x00u;  /* status OK */
        s_hci_rx[7] = 0x0Cu;  /* HCI version BT 5.3 */
        s_hci_rx_len = 9u;
    } else {
        /* Default: simple OK */
        s_hci_rx[0] = HCI_EVENT_PKT;
        s_hci_rx[1] = HCI_EV_CMD_COMPLETE;
        s_hci_rx[2] = 4u;
        s_hci_rx[3] = 1u;
        s_hci_rx[4] = (uint8_t)(opcode & 0xFFu);
        s_hci_rx[5] = (uint8_t)(opcode >> 8u);
        s_hci_rx[6] = 0x00u;
        s_hci_rx_len = 7u;
    }
}

/* =========================================================================
 * Stub HAL ops
 * ====================================================================== */

static int stub_init(uiox_bt_hw_t *hw)
{
    (void)hw;
    printf("  [hal] init  %s  UART=%u baud\n",
           hw->model, hw->uart_baud);
    return 0;
}

static void stub_deinit(uiox_bt_hw_t *hw) { (void)hw; }

static int stub_power(uiox_bt_hw_t *hw, bool on)
{
    (void)hw;
    printf("  [hal] BT power %s\n", on ? "ON" : "OFF");
    return 0;
}

static int stub_fw_download(uiox_bt_hw_t *hw,
                             const uint8_t *fw, uint32_t size)
{
    (void)hw; (void)fw;
    printf("  [hal] FW download  %u bytes\n", size);
    return 0;
}

static int stub_hci_write(uiox_bt_hw_t *hw,
                           const uint8_t *buf, uint16_t len)
{
    (void)hw;
    printf("  [hal] HCI TX  type=0x%02X  len=%u",
           buf[0], len);
    if (buf[0] == HCI_CMD_PKT && len >= 3) {
        uint16_t op = (uint16_t)(buf[1] | (buf[2] << 8u));
        printf("  opcode=0x%04X", op);
        prepare_sim_response(op);
    }
    printf("\n");
    return len;
}

static int stub_hci_read(uiox_bt_hw_t *hw,
                          uint8_t *buf, uint16_t max_len)
{
    (void)hw;
    if (s_hci_rx_len == 0) {
        /* Simulate an async BLE ADV report on step 6 */
        if (s_sim_step == 6) {
            s_sim_step++;
            /* HCI LE Meta ADV report: 1 device, addr AA:BB:CC:DD:EE:FF */
            static const uint8_t adv_ev[] = {
                HCI_EVENT_PKT,
                HCI_EV_LE_META,
                0x0Fu,           /* param len */
                HCI_LE_EV_ADV_REPORT,
                0x01u,           /* num reports */
                0x00u,           /* event type: ADV_IND */
                0x00u,           /* addr type: public */
                0xFFu,0xEEu,0xDDu,0xCCu,0xBBu,0xAAu, /* addr */
                0x03u,           /* data len */
                0x02u,0x01u,0x06u, /* AD: Flags */
                (uint8_t)(-65),  /* RSSI */
            };
            uint16_t copy = sizeof(adv_ev) < max_len ?
                             sizeof(adv_ev) : max_len;
            memcpy(buf, adv_ev, copy);
            return (int)copy;
        }
        return 0;
    }
    uint16_t copy = s_hci_rx_len < max_len ? s_hci_rx_len : max_len;
    memcpy(buf, s_hci_rx, copy);
    s_hci_rx_len = 0;
    s_sim_step++;
    return (int)copy;
}

static void stub_set_baud(uiox_bt_hw_t *hw, uint32_t baud)
{ (void)hw; printf("  [hal] UART baud → %u\n", baud); }

static void stub_gpio_w(uiox_bt_hw_t *hw, uint32_t p, bool v)
{ (void)hw; printf("  [hal] GPIO pin=%u val=%d\n", p, (int)v); }

static bool stub_gpio_r(uiox_bt_hw_t *hw, uint32_t p)
{ (void)hw; (void)p; return false; }

static void stub_delay(uiox_bt_hw_t *hw, uint32_t ms)
{ (void)hw; (void)ms; }

static void stub_isr_wake(uiox_bt_hw_t *hw)
{ if (hw) hw->host_wake_pending = true; }

static void stub_isr_rx(uiox_bt_hw_t *hw)
{ if (hw) hw->rx_pending = true; }

static const uiox_bt_hw_ops_t stub_ops = {
    .init          = stub_init,
    .deinit        = stub_deinit,
    .power         = stub_power,
    .fw_download   = stub_fw_download,
    .hci_write     = stub_hci_write,
    .hci_read      = stub_hci_read,
    .set_baud      = stub_set_baud,
    .gpio_write    = stub_gpio_w,
    .gpio_read     = stub_gpio_r,
    .delay_ms      = stub_delay,
    .isr_host_wake = stub_isr_wake,
    .isr_rx        = stub_isr_rx,
};

/* =========================================================================
 * Hardware device instance
 * ====================================================================== */

static uiox_bt_hw_t s_hw = {
    .uart_base    = 0x40011000uL,
    .uart_baud    = 3000000u,
    .irq          = 37,
    .caps         = UIOX_BT_CAP_CLASSIC    |
                    UIOX_BT_CAP_BLE        |
                    UIOX_BT_CAP_BLE_AUDIO  |
                    UIOX_BT_CAP_A2DP       |
                    UIOX_BT_CAP_HFP        |
                    UIOX_BT_CAP_HID        |
                    UIOX_BT_CAP_2M_PHY     |
                    UIOX_BT_CAP_CODED_PHY  |
                    UIOX_BT_CAP_EXT_ADV    |
                    UIOX_BT_CAP_PERIODIC_ADV |
                    UIOX_BT_CAP_COEX       |
                    UIOX_BT_CAP_FW_DOWNLOAD,
    .if_type      = UIOX_BT_IF_UART,
    .version      = UIOX_BT_VER_BT53,
    .model        = "Intel AX211 BT 5.3",
    .fw_version   = "22.120.2.3",
    .bt_en_pin    = 5u,
    .bt_wake_pin  = 6u,
    .host_wake_pin= 7u,
};

/* =========================================================================
 * Event callback
 * ====================================================================== */

static void on_bt_event(uiox_bt_ev_t ev,
                         uiox_bt_remote_dev_t *dev, void *ctx)
{
    (void)ctx;
    printf("  [event] %-18s", uiox_bt_ev_name(ev));
    if (dev)
        printf("  addr=%02X:%02X:%02X:%02X:%02X:%02X",
               dev->addr[5],dev->addr[4],dev->addr[3],
               dev->addr[2],dev->addr[1],dev->addr[0]);
    printf("\n");
}

/* =========================================================================
 * main
 * ====================================================================== */

int main(void)
{
    printf("=== UIOX Bluetooth Stack Demo ===\n\n");

    /* ------------------------------------------------------------------ */
    /* 1. Open device                                                      */
    /* ------------------------------------------------------------------ */

    printf("--- Open ---\n");
    uiox_bt_device_t dev;
    uiox_bt_open_params_t p = {
        .hw     = &s_hw,
        .hw_ops = &stub_ops,
        .evt_cb = on_bt_event,
    };
    int rc = uiox_bt_open(&dev, &p);
    if (rc < 0) { printf("[error] open: %d\n", rc); return 1; }

    /* ------------------------------------------------------------------ */
    /* 2. Start (power on + HCI reset + read BD addr)                      */
    /* ------------------------------------------------------------------ */

    printf("\n--- Start ---\n");
    rc = uiox_bt_start(&dev);
    printf("  State: %s  rc=%d\n",
           uiox_bt_state_name(dev.subsys.state), rc);

    printf("\n--- Device info ---\n");
    uiox_bt_print_info(&dev);

    /* ------------------------------------------------------------------ */
    /* 3. Set local name                                                   */
    /* ------------------------------------------------------------------ */

    printf("\n--- Set local name ---\n");
    rc = uiox_bt_set_name(&dev, "UIOX Laptop BT");
    printf("  Set name 'UIOX Laptop BT'  rc=%d\n", rc);

    /* ------------------------------------------------------------------ */
    /* 4. Classic BT inquiry scan                                          */
    /* ------------------------------------------------------------------ */

    printf("\n--- Classic BT inquiry scan (3 s) ---\n");
    rc = uiox_bt_scan_start(&dev, 3u);
    printf("  Scan start  rc=%d  state=%s\n",
           rc, uiox_bt_state_name(dev.subsys.state));
    for (uint32_t t = 10u; t <= 40u; t += 10u)
        uiox_bt_tick(&dev, t);
    uiox_bt_scan_stop(&dev);
    printf("  Scan stopped\n");

    /* ------------------------------------------------------------------ */
    /* 5. BLE advertising                                                  */
    /* ------------------------------------------------------------------ */

    printf("\n--- BLE advertising ---\n");
    static const uint8_t adv_data[] = {
        0x02u, 0x01u, 0x06u,        /* Flags: LE General Discoverable */
        0x09u, 0x09u,               /* Complete Local Name */
        'U','I','O','X',' ','B','T','\0'
    };
    rc = uiox_bt_adv_start(&dev, adv_data, sizeof(adv_data));
    printf("  ADV start  rc=%d  state=%s\n",
           rc, uiox_bt_state_name(dev.subsys.state));
    for (uint32_t t = 50u; t <= 70u; t += 10u)
        uiox_bt_tick(&dev, t);
    rc = uiox_bt_adv_stop(&dev);
    printf("  ADV stop   rc=%d\n", rc);

    /* ------------------------------------------------------------------ */
    /* 6. BLE scan — simulate receiving an ADV report                     */
    /* ------------------------------------------------------------------ */

    printf("\n--- BLE scan + simulated ADV report ---\n");
    s_sim_step = 5u;  /* Next read will trigger ADV report at step 6 */
    rc = uiox_bt_le_scan_start(&dev, 160u, 80u, false);
    printf("  LE scan start  rc=%d\n", rc);
    for (uint32_t t = 80u; t <= 130u; t += 10u)
        uiox_bt_tick(&dev, t);
    uiox_bt_le_scan_stop(&dev);
    printf("  LE scan stopped\n");

    /* ------------------------------------------------------------------ */
    /* 7. Print discovered devices                                         */
    /* ------------------------------------------------------------------ */

    printf("\n--- Discovered devices ---\n");
    uiox_bt_print_devices(&dev);

    /* ------------------------------------------------------------------ */
    /* 8. Raw HCI command                                                  */
    /* ------------------------------------------------------------------ */

    printf("\n--- Raw HCI command (Read Local Features) ---\n");
    uint8_t resp[16];
    rc = uiox_bt_hci_cmd(&dev, HCI_OP_READ_LOCAL_FEATURES,
                          NULL, 0, resp, sizeof(resp), 3000u);
    printf("  HCI_READ_LOCAL_FEATURES  rc=%d\n", rc);

    /* ------------------------------------------------------------------ */
    /* 9. ACL data send (simulated connected handle 0x0040)               */
    /* ------------------------------------------------------------------ */

    printf("\n--- ACL data send (handle=0x0040) ---\n");
    static const uint8_t acl_payload[] = {
        0x00u,0x00u,  /* L2CAP len = 4 */
        0x04u,0x00u,  /* L2CAP: len=4 */
        0x04u,0x00u,  /* CID = ATT */
        0x0Bu,        /* ATT_FIND_INFO_REQ */
        0x01u,0x00u,  /* start handle */
        0xFFu,0xFFu,  /* end handle */
    };
    rc = uiox_bt_acl_send(&dev, 0x0040u,
                           acl_payload, sizeof(acl_payload));
    printf("  ACL send  rc=%d  len=%zu\n", rc, sizeof(acl_payload));

    /* ------------------------------------------------------------------ */
    /* 10. GATT discover (simulated)                                       */
    /* ------------------------------------------------------------------ */

    printf("\n--- GATT primary service discovery ---\n");
    rc = uiox_bt_proto_gatt_discover(&dev.subsys.proto, 0x0040u);
    printf("  GATT discover  rc=%d\n", rc);

    /* ------------------------------------------------------------------ */
    /* 11. GATT write                                                      */
    /* ------------------------------------------------------------------ */

    printf("\n--- GATT write (battery level notify enable) ---\n");
    uint8_t cccd[2] = { 0x01u, 0x00u };  /* Notifications enabled */
    rc = uiox_bt_gatt_write(&dev, 0x0040u, 0x000Fu, cccd, 2u);
    printf("  GATT write  attr=0x000F  rc=%d\n", rc);

    /* ------------------------------------------------------------------ */
    /* 12. Tick loop                                                       */
    /* ------------------------------------------------------------------ */

    printf("\n--- Tick loop (5 × 10ms) ---\n");
    for (uint32_t t = 140u; t <= 180u; t += 10u)
        uiox_bt_tick(&dev, t);

    /* ------------------------------------------------------------------ */
    /* 13. Statistics                                                      */
    /* ------------------------------------------------------------------ */

    printf("\n--- Statistics ---\n");
    uiox_bt_print_stats(&dev);

    /* ------------------------------------------------------------------ */
    /* 14. Stop and close                                                  */
    /* ------------------------------------------------------------------ */

    printf("\n--- Stop and close ---\n");
    uiox_bt_stop(&dev);
    printf("  State: %s\n", uiox_bt_state_name(dev.subsys.state));
    uiox_bt_close(&dev);
    printf("  Device: CLOSED\n");

    printf("\n=== UIOX Bluetooth Demo complete ===\n");
    return 0;
}
