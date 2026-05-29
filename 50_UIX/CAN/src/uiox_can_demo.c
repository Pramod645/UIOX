/**
 * @file    uiox_can_demo.c
 * @brief   UIOX CAN stack end-to-end demonstration.
 *
 * Demonstrates the complete flow:
 *   HAL init → bus config → node setup → filter → mailbox →
 *   NMT → PDO TX → SDO write → RX dispatch → bus health → teardown.
 *
 * Uses stub HAL ops — replace with real M_CAN / SJA1000 / FDCAN driver.
 * @date    2026-05-26
 */
//Demo Application
 #include "uiox_can_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 
 /* =========================================================================
  * Stub HAL ops
  * ====================================================================== */
 
 static uint32_t s_sim_tec = 0;
 static uint32_t s_sim_rec = 0;
 
 static int  stub_init    (uiox_can_hw_t *hw)
 { (void)hw; printf("  [hal] init  clk=%u Hz\n", hw->clk_hz); return 0; }
 
 static void stub_deinit  (uiox_can_hw_t *hw)
 { (void)hw; printf("  [hal] deinit\n"); }
 
 static int  stub_start   (uiox_can_hw_t *hw)
 { (void)hw; printf("  [hal] start\n"); return 0; }
 
 static void stub_stop    (uiox_can_hw_t *hw)
 { (void)hw; printf("  [hal] stop\n"); }
 
 static int stub_set_mode (uiox_can_hw_t *hw, uiox_can_mode_t mode)
 {
     (void)hw;
     const char *names[] = {"NORMAL","LOOPBACK","LISTEN","SLEEP"};
     printf("  [hal] mode = %s\n", names[mode]);
     return 0;
 }
 
 static int stub_set_bittiming(uiox_can_hw_t *hw,
                                const uiox_can_bittiming_t *nom,
                                const uiox_can_bittiming_t *data)
 {
     (void)hw;
     printf("  [hal] nom  BRP=%u TSEG1=%u TSEG2=%u SJW=%u\n",
            nom->brp,  nom->tseg1,  nom->tseg2,  nom->sjw);
     printf("  [hal] data BRP=%u TSEG1=%u TSEG2=%u SJW=%u\n",
            data->brp, data->tseg1, data->tseg2, data->sjw);
     return 0;
 }
 
 static uint32_t s_tx_frame_count = 0;
 
 static int stub_tx_submit(uiox_can_hw_t *hw,
                            uintptr_t phys, uint32_t length)
 {
     (void)hw; (void)length;
     uiox_can_msg_t *msg = (uiox_can_msg_t *)phys;
     uint32_t id = msg->id & UIOX_CAN_ID_EXT_MASK;
     uint8_t  len = uiox_can_dlc2len(msg->dlc);
     printf("  [hal] TX  ID=0x%08X  DLC=%u  len=%u  data=",
            id, msg->dlc, len);
     for (uint8_t i = 0; i < len && i < 8u; i++)
         printf("%02X ", msg->data[i]);
     printf("\n");
     s_tx_frame_count++;
     return 0;
 }
 
 static int stub_tx_done(uiox_can_hw_t *hw)
 { (void)hw; return (int)s_tx_frame_count; }
 
 /* Simulate one incoming RX frame per poll call */
 static uint8_t s_rx_count = 0;
 static int stub_rx_poll(uiox_can_hw_t *hw,
                          uintptr_t *phys_out, uint32_t *len_out)
 {
     (void)hw;
     if (s_rx_count >= 3) return 0;
 
     /* Simulate a PDO from node 0x02 */
     static uiox_can_msg_t sim_msg;
     memset(&sim_msg, 0, sizeof(sim_msg));
     sim_msg.id  = (uint32_t)(0x182 + s_rx_count); /* TPDO1 from node 2 */
     sim_msg.dlc = 4;
     sim_msg.data[0] = 0x10 + s_rx_count;
     sim_msg.data[1] = 0x20;
     sim_msg.data[2] = 0x30;
     sim_msg.data[3] = 0x40;
     sim_msg.ts_ns   = (uint64_t)s_rx_count * 1000000ULL;
     s_rx_count++;
 
     *phys_out = (uintptr_t)&sim_msg;
     *len_out  = sizeof(sim_msg);
     return (int)*len_out;
 }
 
 static int stub_set_filter(uiox_can_hw_t *hw,
                             uint8_t idx, uint32_t id,
                             uint32_t mask, bool ext)
 {
     (void)hw;
     printf("  [hal] filter[%u]  ID=0x%08X  mask=0x%08X  ext=%d\n",
            idx, id, mask, (int)ext);
     return 0;
 }
 
 static void stub_clr_filters(uiox_can_hw_t *hw)
 { (void)hw; printf("  [hal] clear_filters\n"); }
 
 static int stub_get_err_cnt(uiox_can_hw_t *hw, uiox_can_err_cnt_t *out)
 {
     (void)hw;
     out->tec = (uint16_t)s_sim_tec;
     out->rec = (uint16_t)s_sim_rec;
     return 0;
 }
 
 static int stub_recover(uiox_can_hw_t *hw)
 {
     (void)hw;
     printf("  [hal] bus-off recovery\n");
     hw->err_state = UIOX_CAN_ERR_ACTIVE;
     s_sim_tec = 0;
     s_sim_rec = 0;
     return 0;
 }
 
 static void stub_isr(uiox_can_hw_t *hw) { (void)hw; }
 
 static const uiox_can_hw_ops_t stub_ops = {
     .init          = stub_init,
     .deinit        = stub_deinit,
     .start         = stub_start,
     .stop          = stub_stop,
     .set_mode      = stub_set_mode,
     .set_bittiming = stub_set_bittiming,
     .tx_submit     = stub_tx_submit,
     .tx_done       = stub_tx_done,
     .rx_poll       = stub_rx_poll,
     .set_filter    = stub_set_filter,
     .clear_filters = stub_clr_filters,
     .get_err_cnt   = stub_get_err_cnt,
     .recover       = stub_recover,
     .isr           = stub_isr,
 };
 
 /* =========================================================================
  * Hardware device instances (two CAN buses)
  * ====================================================================== */
 
  static uiox_can_hw_t s_hw_bus0 = {
    .base_addr = 0x40006400uL,  /* CAN1 MMIO (STM32-style)              */
    .irq       = 20,
    .caps      = UIOX_CAN_CAP_FD        |
                 UIOX_CAN_CAP_DMA       |
                 UIOX_CAN_CAP_HW_FILTER |
                 UIOX_CAN_CAP_LOOPBACK  |
                 UIOX_CAN_CAP_TIMESTAMP |
                 UIOX_CAN_CAP_BERR_RPT,
    .clk_hz    = 80000000u,     /* 80 MHz controller clock              */
    .mode      = UIOX_CAN_MODE_NORMAL,
    .err_state = UIOX_CAN_ERR_ACTIVE,
};

static uiox_can_hw_t s_hw_bus1 = {
    .base_addr = 0x40006800uL,  /* CAN2 MMIO                            */
    .irq       = 21,
    .caps      = UIOX_CAN_CAP_HW_FILTER |
                 UIOX_CAN_CAP_LOOPBACK  |
                 UIOX_CAN_CAP_LISTEN,
    .clk_hz    = 80000000u,
    .mode      = UIOX_CAN_MODE_NORMAL,
    .err_state = UIOX_CAN_ERR_ACTIVE,
};

/* =========================================================================
 * Global RX handler — called for every received frame
 * ====================================================================== */

static void on_rx(uint8_t bus_idx, const uiox_can_msg_t *msg, void *ctx)
{
    (void)ctx;
    uint32_t id  = msg->id & UIOX_CAN_ID_EXT_MASK;
    uint8_t  len = uiox_can_dlc2len(msg->dlc);

    printf("  [rx] bus=%u  ID=0x%08X  DLC=%u  len=%u  data=",
           bus_idx, id, msg->dlc, len);
    for (uint8_t i = 0; i < len && i < 8u; i++)
        printf("%02X ", msg->data[i]);
    printf("  ts=%.3f ms\n", (double)msg->ts_ns / 1e6);
}

/* RX handler for PDO1 range only (COB 0x180..0x1FF) */
static void on_pdo1_rx(uint8_t bus_idx, const uiox_can_msg_t *msg, void *ctx)
{
    (void)ctx;
    uint32_t id = msg->id & UIOX_CAN_ID_EXT_MASK;
    printf("  [pdo1_rx] bus=%u  COB=0x%03X  data=%02X %02X %02X %02X\n",
           bus_idx, id,
           msg->data[0], msg->data[1],
           msg->data[2], msg->data[3]);
}

/* Mailbox RX callback for node-specific mailbox */
static void on_mb_rx(uint32_t cob_id, const uint8_t *data,
                      uint8_t len, void *ctx)
{
    (void)ctx;
    printf("  [mb_rx] COB=0x%03X  len=%u  data=", cob_id, len);
    for (uint8_t i = 0; i < len; i++) printf("%02X ", data[i]);
    printf("\n");
}

/* =========================================================================
 * main
 * ====================================================================== */

int main(void)
{
    printf("=== UIOX CAN Communication Stack Demo ===\n\n");

    /* ------------------------------------------------------------------ */
    /* 1. Open device                                                      */
    /* ------------------------------------------------------------------ */

    uiox_can_device_t dev;
    printf("--- Open device ---\n");
    int rc = uiox_can_open(&dev);
    if (rc < 0) {
        printf("[error] uiox_can_open failed: %d\n", rc);
        return 1;
    }
    printf("  Device opened\n");

    /* ------------------------------------------------------------------ */
    /* 2. Add CAN bus 0 (CAN-FD, 500 kbit/s nominal, 2 Mbit/s data)      */
    /* ------------------------------------------------------------------ */

    printf("\n--- Add bus 0 (CAN-FD) ---\n");
    uiox_can_bus_params_t p0;
    memset(&p0, 0, sizeof(p0));
    p0.hw            = &s_hw_bus0;
    p0.hw_ops        = &stub_ops;
    p0.node_id       = 0x01;
    p0.name          = "ECU_MAIN";
    p0.fd_enabled    = true;
    p0.nom_bitrate   = 500000u;
    p0.data_bitrate  = 2000000u;
    p0.heartbeat_ms  = 1000u;
    p0.busoff.retry_delay_ms = 100u;
    p0.busoff.max_retries    = 5u;

    int bus0 = uiox_can_add_bus(&dev, &p0);
    if (bus0 < 0) {
        printf("[error] add_bus 0 failed: %d\n", bus0);
        uiox_can_close(&dev);
        return 1;
    }
    printf("  Bus 0 added  node_id=0x%02X  name=%s  FD=%d\n",
           p0.node_id, p0.name, (int)p0.fd_enabled);

    /* ------------------------------------------------------------------ */
    /* 3. Add CAN bus 1 (Classic CAN, 250 kbit/s)                        */
    /* ------------------------------------------------------------------ */

    printf("\n--- Add bus 1 (Classic CAN) ---\n");
    uiox_can_bus_params_t p1;
    memset(&p1, 0, sizeof(p1));
    p1.hw            = &s_hw_bus1;
    p1.hw_ops        = &stub_ops;
    p1.node_id       = 0x02;
    p1.name          = "SENSOR_BUS";
    p1.fd_enabled    = false;
    p1.nom_bitrate   = 250000u;
    p1.data_bitrate  = 250000u;
    p1.heartbeat_ms  = 2000u;
    p1.busoff.retry_delay_ms = 200u;
    p1.busoff.max_retries    = 3u;

    int bus1 = uiox_can_add_bus(&dev, &p1);
    if (bus1 < 0) {
        printf("[error] add_bus 1 failed: %d\n", bus1);
        uiox_can_close(&dev);
        return 1;
    }
    printf("  Bus 1 added  node_id=0x%02X  name=%s\n",
           p1.node_id, p1.name);

    /* ------------------------------------------------------------------ */
    /* 4. Register acceptance filters                                      */
    /* ------------------------------------------------------------------ */

    printf("\n--- Acceptance filters ---\n");
    /* Bus 0: accept all CANopen standard IDs (11-bit) */
    uiox_can_add_filter(&dev, (uint8_t)bus0, 0x000u, 0x000u, false);
    /* Bus 1: accept only PDO1 range (0x180..0x1FF) */
    uiox_can_add_filter(&dev, (uint8_t)bus1, 0x180u, 0x780u, false);

    /* ------------------------------------------------------------------ */
    /* 5. Register mailbox on bus 0 (RX COB 0x201 — RPDO1 from node 1)   */
    /* ------------------------------------------------------------------ */

    printf("\n--- Mailbox registration ---\n");
    uiox_can_mailbox_t mb = {
        .cob_id    = 0x201u,
        .mask      = 0x7FFu,
        .dir       = UIOX_CAN_MB_RX,
        .period_ms = 0u,
        .ext       = false,
        .enabled   = true,
        .dlc       = 0u,
        .rx_cb     = on_mb_rx,
        .cb_ctx    = NULL,
    };
    uiox_can_add_mailbox(&dev, (uint8_t)bus0, &mb);
    printf("  Mailbox RX  COB=0x%03X  bus=%d\n", mb.cob_id, bus0);

    /* ------------------------------------------------------------------ */
    /* 6. Register global RX handlers                                      */
    /* ------------------------------------------------------------------ */

    printf("\n--- RX handlers ---\n");
    /* All frames on all buses */
    uiox_can_register_rx(&dev, on_rx, NULL, 0x000u, 0x000u);
    /* PDO1 range only */
    uiox_can_register_rx(&dev, on_pdo1_rx, NULL, 0x180u, 0x780u);
    printf("  Registered 2 global RX handlers\n");

    /* ------------------------------------------------------------------ */
    /* 7. Start both buses                                                 */
    /* ------------------------------------------------------------------ */

    printf("\n--- Start buses ---\n");
    rc = uiox_can_start_bus(&dev, (uint8_t)bus0);
    printf("  Bus 0 start rc=%d\n", rc);
    rc = uiox_can_start_bus(&dev, (uint8_t)bus1);
    printf("  Bus 1 start rc=%d\n", rc);

    /* ------------------------------------------------------------------ */
    /* 8. NMT — start all nodes on bus 0                                  */
    /* ------------------------------------------------------------------ */

    printf("\n--- NMT start broadcast ---\n");
    rc = uiox_can_nmt_cmd(&dev, (uint8_t)bus0, 0x00u, 0x01u);
    printf("  NMT START(all)  bus=0  rc=%d\n", rc);

    /* ------------------------------------------------------------------ */
    /* 9. SDO write — set heartbeat producer time on remote node 0x05     */
    /* ------------------------------------------------------------------ */

    printf("\n--- SDO write ---\n");
    uint8_t hb_time[2] = { 0xE8u, 0x03u };  /* 1000 ms little-endian */
    rc = uiox_can_sdo_write(&dev, (uint8_t)bus0,
                             0x05u,          /* target node          */
                             0x1017u,        /* heartbeat producer   */
                             0x00u,
                             hb_time, 2u);
    printf("  SDO write  node=0x05  idx=0x1017  val=0x03E8  rc=%d\n", rc);

    /* ------------------------------------------------------------------ */
    /* 10. PDO transmissions (TPDO1 — process data)                       */
    /* ------------------------------------------------------------------ */

    printf("\n--- PDO TX ---\n");
    uint8_t pdo_data[8] = { 0x11, 0x22, 0x33, 0x44,
                             0x55, 0x66, 0x77, 0x88 };

    /* TPDO1 = COB 0x180 + node_id(0x01) = 0x181 */
    rc = uiox_can_tx(&dev, (uint8_t)bus0,
                     0x181u, pdo_data, 8u, false);
    printf("  TPDO1  COB=0x181  bus=0  rc=%d\n", rc);

    /* TPDO2 = COB 0x280 + node_id(0x01) = 0x281 */
    pdo_data[0] = 0xAA; pdo_data[1] = 0xBB;
    rc = uiox_can_tx(&dev, (uint8_t)bus0,
                     0x281u, pdo_data, 4u, false);
    printf("  TPDO2  COB=0x281  bus=0  rc=%d\n", rc);

    /* CAN-FD frame (12 bytes, DLC=9) */
    uint8_t fd_data[12];
    for (uint8_t i = 0; i < 12u; i++) fd_data[i] = i;
    rc = uiox_can_tx(&dev, (uint8_t)bus0,
                     0x301u, fd_data, 12u, false);
    printf("  FD PDO  COB=0x301  len=12  bus=0  rc=%d\n", rc);

    /* ------------------------------------------------------------------ */
    /* 11. EMCY — announce a generic error on bus 0                       */
    /* ------------------------------------------------------------------ */

    printf("\n--- EMCY ---\n");
    rc = uiox_can_emcy(&dev, (uint8_t)bus0,
                        UIOX_CAN_EMCY_GENERIC, 0x01u);
    printf("  EMCY  code=0x1000  err_reg=0x01  rc=%d\n", rc);

    /* ------------------------------------------------------------------ */
    /* 12. Process RX — drain hardware FIFOs and dispatch messages        */
    /* ------------------------------------------------------------------ */

    printf("\n--- RX processing ---\n");
    for (int i = 0; i < 3; i++) {
        s_rx_count = 0;  /* reset stub sim counter for each iteration */
        uiox_can_process(&dev);
    }

    /* ------------------------------------------------------------------ */
    /* 13. Periodic tick (simulate 5 × 100 ms ticks)                     */
    /* ------------------------------------------------------------------ */

    printf("\n--- Periodic tick (5 × 100 ms) ---\n");
    for (uint32_t t = 100u; t <= 500u; t += 100u) {
        uiox_can_tick(&dev, t);
        printf("  tick  t=%u ms\n", t);
    }

    /* ------------------------------------------------------------------ */
    /* 14. Enable gateway (bus 0 ↔ bus 1)                                */
    /* ------------------------------------------------------------------ */

    printf("\n--- Gateway mode ---\n");
    uiox_can_gateway(&dev, true);
    printf("  Gateway enabled (bus0 ↔ bus1)\n");
    s_rx_count = 0;
    uiox_can_process(&dev);  /* routed frames will appear on both buses */
    uiox_can_gateway(&dev, false);
    printf("  Gateway disabled\n");

    /* ------------------------------------------------------------------ */
    /* 15. Bus health and statistics                                       */
    /* ------------------------------------------------------------------ */

    printf("\n--- Bus health & statistics ---\n");
    for (uint8_t bi = 0; bi < 2u; bi++) {
        uiox_can_bus_health_t h = uiox_can_health(&dev, bi);
        uiox_can_if_stats_t   stats;
        uiox_can_stats(&dev, bi, &stats);
        printf("  Bus %u  health=%-14s"
               "  TX frames=%llu  TX bytes=%llu"
               "  RX frames=%llu  RX bytes=%llu\n",
               bi,
               uiox_can_health_name(h),
               (unsigned long long)stats.tx_frames,
               (unsigned long long)stats.tx_bytes,
               (unsigned long long)stats.rx_frames,
               (unsigned long long)stats.rx_bytes);
    }

    /* ------------------------------------------------------------------ */
    /* 16. Buffer pool status                                              */
    /* ------------------------------------------------------------------ */

    printf("\n--- Buffer pool ---\n");
    printf("  TX free : %u / %u\n",
           uiox_can_buf_tx_free(), UIOX_CAN_TX_POOL_SIZE);
    printf("  RX free : %u / %u\n",
           uiox_can_buf_rx_free(), UIOX_CAN_RX_POOL_SIZE);

    /* ------------------------------------------------------------------ */
    /* 17. NMT stop all + close                                           */
    /* ------------------------------------------------------------------ */

    printf("\n--- NMT stop + close ---\n");
    uiox_can_nmt_cmd(&dev, (uint8_t)bus0, 0x00u, 0x02u); /* STOP all */
    uiox_can_close(&dev);
    printf("  Device closed\n");

    printf("\n=== UIOX CAN Demo complete ===\n");
    return 0;
}
