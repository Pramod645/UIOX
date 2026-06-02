/**
 * @file    uiox_cpu_demo.c
 * @brief   UIOX CPU/SoC stack end-to-end demonstration.
 *
 * Demonstrates: detect → feature query → DVFS OPPs → SMP core-up →
 *   IPI → PMU counters → thermal read → governor switch → stats.
 *
 * @date    2026-06-02
 */

 #include "uiox_cpu_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 #include <errno.h>
 
 /* =========================================================================
  * Stub HAL ops (platform-neutral simulation)
  * ====================================================================== */
 
 static int stub_init(uiox_cpu_hw_t *hw)
 {
     printf("  [hal] init  arch=%s  cores=%u\n",
            uiox_cpu_arch_name(hw->arch), hw->num_cores);
     /* Mark core 0 as online */
     hw->cores[0].state       = UIOX_CPU_STATE_RUNNING;
     hw->cores[0].freq_mhz    = 1800u;
     hw->cores[0].max_freq_mhz= 3000u;
     hw->cores[0].min_freq_mhz= 600u;
     hw->cores[0].temp_celsius = 45;
     return 0;
 }
 
 static void stub_deinit(uiox_cpu_hw_t *hw) { (void)hw; }
 
 static int stub_detect(uiox_cpu_hw_t *hw)
 {
     hw->caps = UIOX_CPU_CAP_SMP | UIOX_CPU_CAP_MMU | UIOX_CPU_CAP_FPU |
                UIOX_CPU_CAP_NEON | UIOX_CPU_CAP_ATOMIC | UIOX_CPU_CAP_PMU |
                UIOX_CPU_CAP_CACHE_L1 | UIOX_CPU_CAP_CACHE_L2 |
                UIOX_CPU_CAP_CACHE_L3 | UIOX_CPU_CAP_CRYPTO |
                UIOX_CPU_CAP_VIRTUALIZATION | UIOX_CPU_CAP_HOTPLUG;
     hw->l1i.size_kb = 64u;  hw->l1i.line_bytes = 64u;
     hw->l1d.size_kb = 64u;  hw->l1d.line_bytes = 64u;
     hw->l2.size_kb  = 512u; hw->l2.line_bytes  = 64u;
     hw->l3.size_kb  = 8192u;hw->l3.line_bytes  = 64u;
     printf("  [hal] detect  caps=0x%08X\n", hw->caps);
     return 0;
 }
 
 static uint32_t s_ipi_count = 0;
 static int stub_core_powerup(uiox_cpu_hw_t *hw, uint8_t core_id,
                               uintptr_t entry)
 {
     (void)entry;
     printf("  [hal] core %u power UP  entry=0x%lX\n",
            core_id, (unsigned long)entry);
     hw->cores[core_id].state        = UIOX_CPU_STATE_ONLINE;
     hw->cores[core_id].freq_mhz     = 1800u;
     hw->cores[core_id].temp_celsius = 42;
     return 0;
 }
 
 static int stub_core_powerdown(uiox_cpu_hw_t *hw, uint8_t core_id)
 {
     printf("  [hal] core %u power DOWN\n", core_id);
     hw->cores[core_id].state = UIOX_CPU_STATE_OFFLINE;
     return 0;
 }
 
 static int stub_set_freq(uiox_cpu_hw_t *hw, uint8_t core_id,
    uint32_t freq_mhz)
{
(void)hw;
printf("  [hal] core %u freq → %u MHz\n", core_id, freq_mhz);
hw->cores[core_id].freq_mhz = freq_mhz;
return 0;
}

static int stub_set_voltage(uiox_cpu_hw_t *hw, uint8_t cluster_id,
       uint32_t mv)
{
(void)hw;
printf("  [hal] cluster %u voltage → %u mV\n", cluster_id, mv / 1000u);
return 0;
}

static int8_t s_fake_temp[UIOX_CPU_MAX_CORES] = {45,43,41,40,38,36,35,33};

static int stub_read_temp(uiox_cpu_hw_t *hw, uint8_t core_id,
     int8_t *temp_out)
{
(void)hw;
*temp_out = s_fake_temp[core_id % 8];
return 0;
}

static uint64_t s_pmu_vals[UIOX_PMU_MAX_COUNTERS] = {
1234567890ULL, 987654321ULL, 11223344ULL, 55667788ULL,
99887766ULL,   44332211ULL,  12121212ULL, 34343434ULL
};

static uint64_t stub_perf_read(uiox_cpu_hw_t *hw,
          uint8_t core_id, uint8_t counter_id)
{
(void)hw; (void)core_id;
return s_pmu_vals[counter_id % UIOX_PMU_MAX_COUNTERS];
}

static void stub_perf_reset(uiox_cpu_hw_t *hw,
       uint8_t core_id, uint8_t counter_id)
{
(void)hw; (void)core_id;
s_pmu_vals[counter_id % UIOX_PMU_MAX_COUNTERS] = 0;
}

static void stub_cache_flush(uiox_cpu_hw_t *hw,
        uintptr_t addr, size_t size)
{
(void)hw;
printf("  [hal] cache flush  addr=0x%lX  size=%zu\n",
(unsigned long)addr, size);
}

static void stub_tlb_invalidate(uiox_cpu_hw_t *hw, uintptr_t vaddr)
{
(void)hw;
printf("  [hal] TLB invalidate  vaddr=0x%lX\n", (unsigned long)vaddr);
}

static int stub_ipi_send(uiox_cpu_hw_t *hw,
    uint8_t target, uint8_t vector)
{
(void)hw;
s_ipi_count++;
printf("  [hal] IPI → core %u  vector=0x%02X  (#%u)\n",
target, vector, s_ipi_count);
return 0;
}

static void stub_isr_timer (uiox_cpu_hw_t *hw) { (void)hw; }
static void stub_isr_ipi   (uiox_cpu_hw_t *hw, uint8_t v)
{ (void)hw; (void)v; }
static void stub_isr_fault (uiox_cpu_hw_t *hw, uint64_t a, uint32_t t)
{ (void)hw; (void)a; (void)t; }

static const uiox_cpu_hw_ops_t stub_ops = {
.init            = stub_init,
.deinit          = stub_deinit,
.detect          = stub_detect,
.core_powerup    = stub_core_powerup,
.core_powerdown  = stub_core_powerdown,
.set_freq        = stub_set_freq,
.set_voltage     = stub_set_voltage,
.read_temp       = stub_read_temp,
.perf_read       = stub_perf_read,
.perf_reset      = stub_perf_reset,
.cache_flush     = stub_cache_flush,
.tlb_invalidate  = stub_tlb_invalidate,
.ipi_send        = stub_ipi_send,
.isr_timer       = stub_isr_timer,
.isr_ipi         = stub_isr_ipi,
.isr_fault       = stub_isr_fault,
};

/* =========================================================================
* Hardware device instance
* ====================================================================== */

static uiox_cpu_hw_t s_hw = {
#if defined(UIOX_ARCH_ARM64)
.arch         = UIOX_CPU_ARCH_ARM64,
.model_str    = "ARM Cortex-A76",
.vendor_str   = "ARM Ltd.",
#elif defined(UIOX_ARCH_X86_64)
.arch         = UIOX_CPU_ARCH_X86_64,
.model_str    = "Intel Core i9-13900K",
.vendor_str   = "Intel Corporation",
#else
.arch         = UIOX_CPU_ARCH_RV64,
.model_str    = "SiFive U74-MC",
.vendor_str   = "SiFive Inc.",
#endif
.num_cores    = 8,
.num_clusters = 2,
.num_threads  = 1,
.gic_base     = 0x08000000uL,
.timer_base   = 0x09000000uL,
.clint_base   = 0x02000000uL,
.timer_freq_hz= 1000000u,   /* 1 MHz reference */
};

/* =========================================================================
* Event callback
* ====================================================================== */

static void on_cpu_event(uiox_cpu_evt_t evt, uint8_t core_id, void *ctx)
{
(void)ctx;
if (evt == UIOX_CPU_EVT_TIMER_TICK) return;  /* too verbose */
printf("  [event] %-22s  core=%u\n",
uiox_cpu_evt_name(evt), core_id);
}

/* =========================================================================
* IPI function callback (runs on remote core in simulation)
* ====================================================================== */

static void ipi_tlb_flush_fn(void *ctx)
{
(void)ctx;
printf("  [ipi_fn] TLB flush executed on remote core\n");
}

/* =========================================================================
* main
* ====================================================================== */

int main(void)
{
printf("=== UIOX CPU/SoC Stack Demo ===\n");
printf("    Architecture: %s\n\n",
uiox_cpu_arch_name(s_hw.arch));

/* ------------------------------------------------------------------ */
/* 1. Open device                                                      */
/* ------------------------------------------------------------------ */

printf("--- Open ---\n");
uiox_cpu_device_t dev;
uiox_cpu_open_params_t p;
memset(&p, 0, sizeof(p));

p.hw                  = &s_hw;
p.hw_ops              = &stub_ops;
p.timer_interval_ns   = 1000000u;   /* 1 ms tick                      */
p.governor            = UIOX_CPU_GOV_ONDEMAND;
p.evt_cb              = on_cpu_event;

int rc = uiox_cpu_open(&dev, &p);
if (rc < 0) {
printf("[error] uiox_cpu_open failed: %d\n", rc);
return 1;
}

/* ------------------------------------------------------------------ */
/* 2. Print CPU info                                                   */
/* ------------------------------------------------------------------ */

printf("\n--- CPU Information ---\n");
uiox_cpu_print_info(&dev);

/* ------------------------------------------------------------------ */
/* 3. Start subsystem (core 0 online)                                  */
/* ------------------------------------------------------------------ */

printf("\n--- Start ---\n");
rc = uiox_cpu_start(&dev);
printf("  Subsystem state: %s  rc=%d\n",
uiox_cpu_state_name(dev.subsys.state), rc);

/* ------------------------------------------------------------------ */
/* 4. SMP — bring secondary cores online                               */
/* ------------------------------------------------------------------ */

printf("\n--- SMP: bring cores 1..3 online ---\n");
for (uint8_t c = 1; c <= 3; c++) {
rc = uiox_cpu_core_up(&dev, c, 0xFFFF0000uL + c * 0x100u);
printf("  Core %u up  rc=%d  state=%d\n",
c, rc, (int)uiox_cpu_core_state(&dev, c));
}

/* ------------------------------------------------------------------ */
/* 5. Set initial OPP and frequency                                    */
/* ------------------------------------------------------------------ */

printf("\n--- DVFS: set OPP ---\n");
for (uint8_t c = 0; c < 4; c++) {
rc = uiox_cpu_set_opp(&dev, c, 2u);  /* 1800 MHz */
printf("  Core %u  OPP[2] = %u MHz  rc=%d\n",
c, uiox_cpu_freq(&dev, c), rc);
}

/* ------------------------------------------------------------------ */
/* 6. Temperature readings                                             */
/* ------------------------------------------------------------------ */

printf("\n--- Temperature ---\n");
for (uint8_t c = 0; c < 4; c++) {
int8_t temp = 0;
uiox_cpu_read_temp(&dev, c, &temp);
printf("  Core %u  temp=%d°C\n", c, temp);
}

/* ------------------------------------------------------------------ */
/* 7. IPI — send inter-processor interrupts                            */
/* ------------------------------------------------------------------ */

printf("\n--- IPI ---\n");
rc = uiox_cpu_send_ipi(&dev, 1u, UIOX_IPI_RESCHEDULE, 0u);
printf("  RESCHEDULE → core 1  rc=%d\n", rc);

rc = uiox_cpu_send_ipi(&dev, 2u, UIOX_IPI_TLB_FLUSH, 0xBEEF0000u);
printf("  TLB_FLUSH  → core 2  rc=%d\n", rc);

/* Function call IPI with callback */
uiox_cpu_work_t *w = uiox_work_alloc();
if (w) {
w->fn  = ipi_tlb_flush_fn;
w->ctx = NULL;
uiox_work_enqueue(0u, w);
}
rc = uiox_cpu_send_ipi(&dev, 3u, UIOX_IPI_WAKEUP, 0u);
printf("  WAKEUP     → core 3  rc=%d\n", rc);

/* ------------------------------------------------------------------ */
/* 8. PMU hardware counters                                            */
/* ------------------------------------------------------------------ */

printf("\n--- PMU counters ---\n");
/* Counter 0: instructions retired (ARM64: 0x08, x86: 0xC0) */
uiox_cpu_pmu_start(&dev, 0u, 0x08u);
/* Counter 1: cache misses (ARM64: 0x17, x86: 0x412E) */
uiox_cpu_pmu_start(&dev, 1u, 0x17u);
/* Counter 2: branch mispredictions (ARM64: 0x10, x86: 0x00C5) */
uiox_cpu_pmu_start(&dev, 2u, 0x10u);

printf("  Counter 0 (instructions)      : %llu\n",
(unsigned long long)uiox_cpu_pmu_read(&dev, 0u));
printf("  Counter 1 (cache misses)      : %llu\n",
(unsigned long long)uiox_cpu_pmu_read(&dev, 1u));
printf("  Counter 2 (branch mispred.)   : %llu\n",
(unsigned long long)uiox_cpu_pmu_read(&dev, 2u));

uiox_cpu_pmu_stop(&dev, 0u);
uiox_cpu_pmu_stop(&dev, 1u);
uiox_cpu_pmu_stop(&dev, 2u);

/* ------------------------------------------------------------------ */
/* 9. Load-based DVFS simulation                                       */
/* ------------------------------------------------------------------ */

printf("\n--- DVFS load simulation (10 ticks) ---\n");
uint32_t sim_loads[4] = { 90u, 10u, 50u, 75u };

for (uint32_t t = 1u; t <= 10u; t++) {
/* Simulate varying load */
for (uint8_t c = 0; c < 4; c++) {
uint32_t load = (sim_loads[c] + t * 3u) % 100u;
uiox_cpu_update_load(&dev, c, load);
}
uiox_cpu_tick(&dev, t * 10u);
if (t % 5u == 0u) {
printf("  tick %2u  ", t);
for (uint8_t c = 0; c < 4; c++)
printf("core%u=%4uMHz(%3u%%)  ",
 c, uiox_cpu_freq(&dev, c),
 dev.subsys.pm.load_pct[c]);
printf("\n");
}
}

/* ------------------------------------------------------------------ */
/* 10. Governor switch                                                 */
/* ------------------------------------------------------------------ */

printf("\n--- Governor switch to performance ---\n");
uiox_cpu_set_governor(&dev, UIOX_CPU_GOV_PERFORMANCE);
uiox_cpu_tick(&dev, 110u);
printf("  Core 0 freq: %u MHz\n", uiox_cpu_freq(&dev, 0u));

printf("\n--- Governor switch to powersave ---\n");
uiox_cpu_set_governor(&dev, UIOX_CPU_GOV_POWERSAVE);
uiox_cpu_tick(&dev, 120u);
printf("  Core 0 freq: %u MHz\n", uiox_cpu_freq(&dev, 0u));

/* ------------------------------------------------------------------ */
/* 11. Cache flush                                                     */
/* ------------------------------------------------------------------ */

printf("\n--- Cache flush ---\n");
uiox_cpu_cache_flush(&dev, 0x80000000uL, 4096u);

/* ------------------------------------------------------------------ */
/* 12. Hardware timestamp and cycle count                              */
/* ------------------------------------------------------------------ */

printf("\n--- Timing ---\n");
printf("  Uptime    : %llu ms\n",
(unsigned long long)uiox_cpu_uptime_ms(&dev));
printf("  Timestamp : %llu ticks\n",
(unsigned long long)uiox_cpu_timestamp(&dev));

/* ------------------------------------------------------------------ */
/* 13. Take cores offline                                              */
/* ------------------------------------------------------------------ */

printf("\n--- SMP: take cores 1..3 offline ---\n");
for (uint8_t c = 3u; c >= 1u; c--) {
rc = uiox_cpu_core_down(&dev, c);
printf("  Core %u down  rc=%d\n", c, rc);
}

/* ------------------------------------------------------------------ */
/* 14. Statistics                                                      */
/* ------------------------------------------------------------------ */

printf("\n--- Statistics ---\n");
uiox_cpu_print_stats(&dev);

/* ------------------------------------------------------------------ */
/* 15. Stop and close                                                  */
/* ------------------------------------------------------------------ */

printf("\n--- Stop and close ---\n");
uiox_cpu_stop(&dev);
printf("  State: %s\n", uiox_cpu_state_name(dev.subsys.state));
uiox_cpu_close(&dev);
printf("  Device: CLOSED\n");

printf("\n=== UIOX CPU/SoC Demo complete ===\n");
return 0;
}
