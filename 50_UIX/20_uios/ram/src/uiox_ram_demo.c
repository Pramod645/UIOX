/**
 * @file    uiox_ram_demo.c
 * @brief   UIOX RAM stack end-to-end demonstration.
 * @date    2026-06-03
 */

 #include "uiox_ram_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 #include <errno.h>
 
 /* =========================================================================
  * Static backing memory (simulates RAM available after MMU init)
  * ====================================================================== */
 
 #define HEAP_SIZE    (512u * 1024u)   /* 512 KB heap      */
 #define BUDDY_SIZE   (256u * 1024u)   /* 256 KB buddy     */
 
 static uint8_t s_heap_mem  [HEAP_SIZE]  __attribute__((aligned(64)));
 static uint8_t s_buddy_mem [BUDDY_SIZE] __attribute__((aligned(64)));
 
 /* Slab backing: 32 objects of 64 bytes each */
 static uint8_t s_slab_mem[32 * 64] __attribute__((aligned(64)));
 
 /* =========================================================================
  * Stub HAL ops
  * ====================================================================== */
 
 static int stub_init(uiox_ram_hw_t *hw)
 {
     printf("  [hal] init  %s  %llu MB  %u MT/s\n",
            uiox_ram_type_name(hw->type),
            (unsigned long long)(hw->total_bytes / (1024*1024)),
            hw->speed_mtps);
     hw->initialised = true;
     return 0;
 }
 
 static void stub_deinit(uiox_ram_hw_t *hw) { (void)hw; }
 
 static int stub_train(uiox_ram_hw_t *hw)
 {
     (void)hw;
     printf("  [hal] PHY training complete\n");
     return 0;
 }
 
 static int stub_mode_reg_write(uiox_ram_hw_t *hw, uint8_t mr, uint16_t val)
 { (void)hw; printf("  [hal] MR%u ← 0x%04X\n", mr, val); return 0; }
 
 static int stub_mode_reg_read(uiox_ram_hw_t *hw, uint8_t mr, uint16_t *val)
 { (void)hw; *val = 0; printf("  [hal] MR%u read\n", mr); return 0; }
 
 static int stub_set_timing(uiox_ram_hw_t *hw, const uiox_ram_timing_t *t)
 { (void)hw; printf("  [hal] timing  tCL=%u  tRCD=%u  tRP=%u\n",
                    t->tCL, t->tRCD, t->tRP); return 0; }
 
 static int stub_zq_cal(uiox_ram_hw_t *hw)
 { (void)hw; printf("  [hal] ZQ calibration\n"); return 0; }
 
 static int stub_set_power(uiox_ram_hw_t *hw, uiox_ram_pwr_t state)
 {
     (void)hw;
     static const char *names[] = {"ACTIVE","SELF_REFRESH","POWER_DOWN","DEEP_DOWN"};
     printf("  [hal] power → %s\n", names[state < 4 ? state : 0]);
     return 0;
 }
 
 static int stub_ecc_enable(uiox_ram_hw_t *hw, bool en)
 { (void)hw; printf("  [hal] ECC %s\n", en?"ON":"OFF"); return 0; }
 
 static uint32_t s_sim_ce = 0;
 static int stub_ecc_scrub(uiox_ram_hw_t *hw,
                            uint64_t phys, uint64_t size)
 {
     (void)hw; (void)phys; (void)size;
     s_sim_ce++;   /* simulate finding one CE per scrub chunk */
     return 0;
 }
 
 static int stub_ecc_status(uiox_ram_hw_t *hw,
                             uint32_t *ce, uint32_t *ue, uint64_t *addr)
 {
     (void)hw;
     *ce   = s_sim_ce;
     *ue   = 0;
     *addr = 0x80001000ULL;
     return 0;
 }
 
 static int stub_read64(uiox_ram_hw_t *hw, uint64_t phys, uint64_t *val)
 { (void)hw; *val = phys ^ 0xDEADBEEFCAFEBABEULL; return 0; }
 
 static int stub_write64(uiox_ram_hw_t *hw, uint64_t phys, uint64_t val)
 { (void)hw; (void)phys; (void)val; return 0; }
 
 static void stub_isr_ecc    (uiox_ram_hw_t *hw) { (void)hw; }
 static void stub_isr_parity (uiox_ram_hw_t *hw) { (void)hw; }
 
 static const uiox_ram_hw_ops_t stub_ops = {
     .init           = stub_init,
     .deinit         = stub_deinit,
     .train          = stub_train,
     .mode_reg_write = stub_mode_reg_write,
     .mode_reg_read  = stub_mode_reg_read,
     .set_timing     = stub_set_timing,
     .zq_calibrate   = stub_zq_cal,
     .set_power      = stub_set_power,
     .ecc_enable     = stub_ecc_enable,
     .ecc_scrub      = stub_ecc_scrub,
     .ecc_status     = stub_ecc_status,
     .read64         = stub_read64,
     .write64        = stub_write64,
     .isr_ecc        = stub_isr_ecc,
     .isr_parity     = stub_isr_parity,
 };
 
 static uiox_ram_hw_t s_hw = {
     .ctrl_base   = 0xF8000000uL,
     .phy_base    = 0xF8010000uL,
     .irq_ecc     = 60,
     .irq_parity  = 61,
     .caps        = UIOX_RAM_CAP_ECC | UIOX_RAM_CAP_SCRUB |
                    UIOX_RAM_CAP_ZQ_CAL | UIOX_RAM_CAP_INTERLEAVE |
                    UIOX_RAM_CAP_SELF_REFRESH | UIOX_RAM_CAP_GEAR_MODE,
     .type        = UIOX_RAM_TYPE_LPDDR5,
     .model       = "LPDDR5-8533",
     .num_channels= 2,
     .num_ranks   = 1,
     .bus_width   = 64,
     .density_gb  = 4,
     .speed_mtps  = 8533u,
     .ref_clk_mhz = 1066u,
     .base_phys   = 0x80000000ULL,
     .total_bytes = 8ULL * 1024ULL * 1024ULL * 1024ULL,  /* 8 GB */
     .timing      = { .tRCD=13, .tRAS=33, .tRP=13,
                      .tCL=10,  .tCWL=8,  .tRFC=260,
                      .tREFI=39, .tWR=15, .tRTP=8,
                      .tFAW=25, .tCCD=4,  .tRRD=4 },
 };
 
 static void on_ram_event(uiox_ram_evt_t evt, void *ctx)
 {
     (void)ctx;
     printf("  [event] %s\n", uiox_ram_evt_name(evt));
 }
 
 int main(void)
 {
     printf("=== UIOX RAM Stack Demo ===\n\n");
 
     printf("--- Open ---\n");
     uiox_ram_device_t dev;
     uiox_ram_open_params_t p = {
         .hw         = &s_hw,
         .hw_ops     = &stub_ops,
         .heap_base  = s_heap_mem,
         .heap_size  = HEAP_SIZE,
         .buddy_base = s_buddy_mem,
         .buddy_size = BUDDY_SIZE,
         .evt_cb     = on_ram_event,
     };
     int rc = uiox_ram_open(&dev, &p);
     if (rc < 0) { printf("[error] open: %d\n", rc); return 1; }
 
     printf("\n--- RAM info ---\n");
     uiox_ram_print_info(&dev);
 
     printf("\n--- Start ---\n");
     rc = uiox_ram_start(&dev);
     printf("  State: %s  rc=%d\n",
            uiox_ram_state_name(dev.subsys.state), rc);
 
     printf("\n--- Heap allocations ---\n");
     void *a = uiox_ram_alloc(&dev, 4096);
     void *b = uiox_ram_calloc(&dev, 16, 64);
     void *c = uiox_ram_alloc(&dev, 1024);
     printf("  alloc(4096) = %p\n", a);
     printf("  calloc(16×64)= %p\n", b);
     printf("  alloc(1024) = %p\n", c);
     if (a) memset(a, 0xAA, 4096);
     if (b) memset(b, 0xBB, 16*64);
     uiox_ram_free(&dev, b);
     b = uiox_ram_realloc(&dev, c, 2048);
     printf("  realloc(1024→2048) = %p\n", b);
 
     printf("\n--- Buddy allocations ---\n");
     void *x = uiox_ram_buddy_alloc(&dev, 8192);
     void *y = uiox_ram_buddy_alloc(&dev, 4096);
     printf("  buddy_alloc(8KB) = %p\n", x);
     printf("  buddy_alloc(4KB) = %p\n", y);
     uiox_ram_buddy_free(&dev, y, 4096);
     printf("  buddy_free(4KB)\n");
 
     printf("\n--- Slab cache ---\n");
     rc = uiox_ram_slab_create(&dev, "task_struct",
                                64u, 32u, s_slab_mem);
     printf("  slab 'task_struct' 64B×32  rc=%d\n", rc);
     void *t1 = uiox_ram_slab_alloc(&dev, 64u);
     void *t2 = uiox_ram_slab_alloc(&dev, 64u);
     printf("  slab_alloc×2: %p  %p\n", t1, t2);
     uiox_ram_slab_free(&dev, t1, 64u);
     printf("  slab_free t1\n");
 
     printf("\n--- ECC scrub (5 ticks) ---\n");
     uiox_ram_ecc_scrub(&dev, s_hw.base_phys, 320u * 1024u);
     for (uint32_t t = 10; t <= 50; t += 10) {
         uiox_ram_tick(&dev, t);
         printf("  [t=%2u ms]  scrub_pos=0x%llX  CE=%u\n", t,
                (unsigned long long)dev.subsys.ecc.scrub_pos,
                dev.subsys.ecc.total_ce);
     }
 
     printf("\n--- Power management ---\n");
     uiox_ram_set_power(&dev, UIOX_RAM_PWR_SELF_REFRESH);
     uiox_ram_set_power(&dev, UIOX_RAM_PWR_ACTIVE);
 
     printf("\n--- Memory info ---\n");
     size_t hu=0, hf=0, bu=0, bf=0;
     uiox_ram_get_info(&dev, &hu, &hf, &bu, &bf);
     printf("  Heap   used=%zu KB  free=%zu KB\n", hu/1024, hf/1024);
     printf("  Buddy  used=%zu KB  free=%zu KB\n", bu/1024, bf/1024);
 
     printf("\n--- Statistics ---\n");
     uiox_ram_print_stats(&dev);
 
     printf("\n--- Stop and close ---\n");
     if (a) uiox_ram_free(&dev, a);
     if (b) uiox_ram_free(&dev, b);
     if (x) uiox_ram_buddy_free(&dev, x, 8192);
     uiox_ram_stop(&dev);
     uiox_ram_close(&dev);
     printf("  Device: CLOSED\n");
     printf("\n=== UIOX RAM Demo complete ===\n");
     return 0;
 }
 