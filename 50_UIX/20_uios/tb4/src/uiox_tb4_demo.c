#include "uiox_tb4_device.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

/* Simulated NHI register space */
static uint32_t s_nhi[0x1000/4];
static uint32_t s_icm_resp[8];
static uint32_t s_icm_resp_len = 0;
static uint32_t s_fake_rx_phys = 0x10000000uL;
static bool     s_hotplug_sim  = false;

static int  stub_init(uiox_tb4_hw_t *hw)
{
    (void)hw;
    printf("  [hal] init  %s  NHI=0x%lX\n",
           hw->model, (unsigned long)hw->nhi_base);
    memset(s_nhi, 0, sizeof(s_nhi));
    hw->icm_ready = true;
    return 0;
}
static void stub_deinit  (uiox_tb4_hw_t *hw) { (void)hw; }
static int  stub_power_on(uiox_tb4_hw_t *hw)
{ (void)hw; printf("  [hal] TB4 power ON\n"); return 0; }
static void stub_power_off(uiox_tb4_hw_t *hw)
{ (void)hw; printf("  [hal] TB4 power OFF\n"); }
static uint32_t stub_nhi_read(uiox_tb4_hw_t *hw, uint32_t off)
{ (void)hw; return s_nhi[(off>>2) & 0x3FFu]; }
static void stub_nhi_write(uiox_tb4_hw_t *hw, uint32_t off, uint32_t v)
{
    (void)hw;
    s_nhi[(off>>2) & 0x3FFu] = v;
    if (off == NHI_INTERRUPT_MASK_CLR)
        printf("  [hal] IRQ unmask 0x%08X\n", v);
}
static int stub_cfg_read(uiox_tb4_hw_t *hw, uint8_t rhi,
                          uint32_t rlo, uint32_t off, uint32_t *v)
{
    (void)hw;
    printf("  [hal] cfg_read  route=%02X:%08X  off=0x%X\n",rhi,rlo,off);
    *v = 0xDEADBEEFu;
    return 0;
}
static int stub_cfg_write(uiox_tb4_hw_t *hw, uint8_t rhi,
                           uint32_t rlo, uint32_t off, uint32_t v)
{
    (void)hw;
    printf("  [hal] cfg_write route=%02X:%08X  off=0x%X  val=0x%08X\n",
           rhi, rlo, off, v);
    return 0;
}
static int stub_icm_send(uiox_tb4_hw_t *hw,
                          const uint32_t *msg, uint8_t n)
{
    (void)hw;
    printf("  [hal] ICM send  opcode=0x%02X  dwords=%u\n",
           msg[0] & 0xFFu, n);
    /* Prepare canned ICM response */
    s_icm_resp[0] = ICM_RESP_OK;
    s_icm_resp[1] = 0x11223344u;
    s_icm_resp[2] = 0x55667788u;
    s_icm_resp[3] = 0x99AABBCCu;
    s_icm_resp_len = 4u;
    return 0;
}
static int stub_icm_recv(uiox_tb4_hw_t *hw,
                          uint32_t *msg, uint8_t max)
{
    (void)hw;
    uint8_t n = s_icm_resp_len < max ? s_icm_resp_len : max;
    memcpy(msg, s_icm_resp, n * 4);
    printf("  [hal] ICM recv  dwords=%u\n", n);
    return (int)n;
}
static int stub_tx_submit(uiox_tb4_hw_t *hw,
                           uintptr_t phys, uint32_t len, bool eof)
{
    (void)hw;
    printf("  [hal] TX submit  phys=0x%lX  len=%u  eof=%d\n",
           (unsigned long)phys, len, (int)eof);
    return 0;
}
static int stub_rx_poll(uiox_tb4_hw_t *hw,
                         uintptr_t *phys_out, uint32_t *len_out)
{
    (void)hw;
    if (!s_hotplug_sim) return 0;
    s_hotplug_sim = false;
    *phys_out = s_fake_rx_phys;
    *len_out  = 64u;
    printf("  [hal] RX frame  phys=0x%lX  len=64\n",
           (unsigned long)s_fake_rx_phys);
    return 1;
}
static void stub_gpio_w(uiox_tb4_hw_t *hw, uint32_t p, bool v)
{ (void)hw; printf("  [hal] GPIO pin=%u val=%d\n",p,(int)v); }
static bool stub_gpio_r(uiox_tb4_hw_t *hw, uint32_t p)
{ (void)hw; (void)p; return false; }
static void stub_isr_ring   (uiox_tb4_hw_t *hw) { (void)hw; }
static void stub_isr_hotplug(uiox_tb4_hw_t *hw)
{ if(hw) hw->pending_irq |= NHI_INT_HOTPLUG; }
static void stub_isr_icm    (uiox_tb4_hw_t *hw) { (void)hw; }
static void stub_isr_error  (uiox_tb4_hw_t *hw) { (void)hw; }

static const uiox_tb4_hw_ops_t stub_ops = {
    .init        = stub_init,    .deinit       = stub_deinit,
    .power_on    = stub_power_on,.power_off    = stub_power_off,
    .nhi_read    = stub_nhi_read,.nhi_write    = stub_nhi_write,
    .cfg_read    = stub_cfg_read,.cfg_write    = stub_cfg_write,
    .icm_send    = stub_icm_send,.icm_recv     = stub_icm_recv,
    .tx_submit   = stub_tx_submit,.rx_poll     = stub_rx_poll,
    .gpio_write  = stub_gpio_w,  .gpio_read    = stub_gpio_r,
    .isr_ring    = stub_isr_ring,.isr_hotplug  = stub_isr_hotplug,
    .isr_icm     = stub_isr_icm, .isr_error    = stub_isr_error,
};

static uiox_tb4_hw_t s_hw = {
    .nhi_base    = 0xB0000000uL,
    .cfg_base    = 0xB0100000uL,
    .irq         = 24,
    .caps        = UIOX_TB4_CAP_40GBPS | UIOX_TB4_CAP_PCIE_TUNNEL |
                   UIOX_TB4_CAP_DP_TUNNEL | UIOX_TB4_CAP_USB3_TUNNEL |
                   UIOX_TB4_CAP_USB2_TUNNEL | UIOX_TB4_CAP_XDOMAIN |
                   UIOX_TB4_CAP_DMA_TUNNEL | UIOX_TB4_CAP_DUAL_PORT |
                   UIOX_TB4_CAP_DAISY_CHAIN | UIOX_TB4_CAP_SECURITY |
                   UIOX_TB4_CAP_ICM | UIOX_TB4_CAP_IOMMU,
    .version     = UIOX_TB4_VER_TB4,
    .security    = UIOX_TB4_SEC_USER_AUTH,
    .model       = "Intel JHL8540 Maple Ridge",
    .num_ports   = 2,
    .num_tx_rings= 6,
    .num_rx_rings= 6,
    .frc_pwr_pin = 10,
    .plug_det_pin= 11,
};

static void on_tb4_event(uiox_tb4_ev_t ev,
                          uiox_tb4_router_t *r, void *ctx)
{
    (void)ctx;
    printf("  [event] %-28s  router=%02X:%08X\n",
           uiox_tb4_ev_name(ev),
           r ? r->route_hi : 0u,
           r ? r->route_lo : 0u);
}

int main(void)
{
    printf("=== UIOX Thunderbolt 4 Stack Demo ===\n\n");

    printf("--- Open ---\n");
    uiox_tb4_device_t dev;
    uiox_tb4_open_params_t p = {
        .hw       = &s_hw,
        .hw_ops   = &stub_ops,
        .security = UIOX_TB4_SEC_USER_AUTH,
        .evt_cb   = on_tb4_event,
    };
    int rc = uiox_tb4_open(&dev, &p);
    if (rc < 0) { printf("[error] open: %d\n", rc); return 1; }

    printf("\n--- Start (power on + ICM driver_ready) ---\n");
    rc = uiox_tb4_start(&dev);
    printf("  State: %s  rc=%d\n",
           uiox_tb4_state_name(dev.subsys.state), rc);

    printf("\n--- Device info ---\n");
    uiox_tb4_print_info(&dev);

    printf("\n--- Simulate hotplug event ---\n");
    /* Trigger simulated hotplug */
    stub_isr_hotplug(&s_hw);
    for (uint32_t t = 10; t <= 30; t += 10)
        uiox_tb4_tick(&dev, t);

    printf("\n--- Topology ---\n");
    uiox_tb4_print_topo(&dev);

    printf("\n--- Manual scan ---\n");
    rc = uiox_tb4_scan(&dev);
    printf("  Routers found: %d\n", rc);

    printf("\n--- Approve device ---\n");
    uiox_tb4_router_t *r = uiox_tb4_get_router(&dev, 0u, 0x01u);
    if (r) {
        rc = uiox_tb4_approve(&dev, r);
        printf("  Approve rc=%d  authorised=%s\n",
               rc, r->authorised ? "YES" : "NO");
    } else {
        printf("  Router 0x01 not found\n");
    }

    printf("\n--- Send data frame ---\n");
    static const uint8_t payload[] = {
        0xAA,0xBB,0xCC,0xDD, 0x01,0x02,0x03,0x04,
        0x05,0x06,0x07,0x08, 0x09,0x0A,0x0B,0x0C,
    };
    rc = uiox_tb4_send(&dev, payload, sizeof(payload));
    printf("  Send rc=%d  len=%zu\n", rc, sizeof(payload));

    printf("\n--- Simulate RX frame ---\n");
    s_hotplug_sim = true;
    uiox_tb4_frame_t *rxf = uiox_tb4_if_rx(&dev.subsys.tif);
    if (rxf) {
        printf("  Received frame  len=%u  paddr=0x%lX\n",
               rxf->len, (unsigned long)rxf->paddr);
        uiox_tb4_buf_free(rxf);
    }

    printf("\n--- NHI register read ---\n");
    uint32_t pwr = uiox_tb4_hw_nhi_read(&s_hw, NHI_POWER_STATE);
    printf("  NHI_POWER_STATE = 0x%08X\n", pwr);

    printf("\n--- Tick loop (3 × 10ms) ---\n");
    for (uint32_t t = 100; t <= 120; t += 10)
        uiox_tb4_tick(&dev, t);

    printf("\n--- Statistics ---\n");
    uiox_tb4_print_stats(&dev);

    printf("\n--- Final topology ---\n");
    uiox_tb4_print_topo(&dev);

    printf("\n--- Stop and close ---\n");
    uiox_tb4_stop(&dev);
    printf("  State: %s\n", uiox_tb4_state_name(dev.subsys.state));
    uiox_tb4_close(&dev);
    printf("  Device: CLOSED\n");

    printf("\n=== UIOX Thunderbolt 4 Demo complete ===\n");
    return 0;
}
