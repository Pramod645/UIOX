/**
 * @file    uiox_cpu_device.c
 * @brief   UIOX CPU device API implementation.
 * @date    2026-06-02
 */

#include "uiox_cpu_device.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>

int uiox_cpu_open(uiox_cpu_device_t *dev, const uiox_cpu_open_params_t *p)
{
    if (!dev || !p || !p->hw || !p->hw_ops) return -EINVAL;
    memset(dev, 0, sizeof(*dev));
    dev->hw = p->hw;

    int rc = uiox_cpu_hw_init(p->hw, p->hw_ops);
    if (rc < 0) return rc;

    rc = uiox_cpu_hw_detect(p->hw);
    if (rc < 0) return rc;

    rc = uiox_cpu_subsys_init(&dev->subsys, p->hw,
                               p->timer_interval_ns ?
                               p->timer_interval_ns : 1000000u);
    if (rc < 0) return rc;

    uiox_cpu_subsys_set_gov(&dev->subsys, p->governor);

    if (p->evt_cb)
        uiox_cpu_subsys_set_cb(&dev->subsys, p->evt_cb, p->evt_ctx);

    dev->open = true;
    return 0;
}

int uiox_cpu_start(uiox_cpu_device_t *dev)
{
    if (!dev || !dev->open) return -EINVAL;
    return uiox_cpu_subsys_start(&dev->subsys);
}

void uiox_cpu_stop(uiox_cpu_device_t *dev)
{
    if (!dev || !dev->open) return;
    uiox_cpu_subsys_stop(&dev->subsys);
}

void uiox_cpu_close(uiox_cpu_device_t *dev)
{
    if (!dev || !dev->open) return;
    uiox_cpu_stop(dev);
    uiox_cpu_hw_deinit(dev->hw);
    dev->open = false;
}

void uiox_cpu_tick(uiox_cpu_device_t *dev, uint32_t now_ms)
{
    if (!dev || !dev->open) return;
    uiox_cpu_subsys_tick(&dev->subsys, now_ms);
}

int uiox_cpu_core_up(uiox_cpu_device_t *dev,
                      uint8_t core_id, uintptr_t entry)
{
    if (!dev || !dev->open) return -EINVAL;
    return uiox_cpu_subsys_core_up(&dev->subsys, core_id, entry);
}

int uiox_cpu_core_down(uiox_cpu_device_t *dev, uint8_t core_id)
{
    if (!dev || !dev->open) return -EINVAL;
    return uiox_cpu_subsys_core_down(&dev->subsys, core_id);
}

int uiox_cpu_send_ipi(uiox_cpu_device_t *dev,
                       uint8_t target_core,
                       uiox_ipi_type_t type,
                       uint64_t arg)
{
    if (!dev || !dev->open) return -EINVAL;
    return uiox_cpu_subsys_ipi(&dev->subsys, target_core, type, arg);
}

void uiox_cpu_set_governor(uiox_cpu_device_t *dev, uiox_cpu_governor_t gov)
{
    if (!dev || !dev->open) return;
    uiox_cpu_subsys_set_gov(&dev->subsys, gov);
}

int uiox_cpu_set_opp(uiox_cpu_device_t *dev,
                      uint8_t core_id, uint8_t opp_idx)
{
    if (!dev || !dev->open) return -EINVAL;
    return uiox_cpu_pm_set_opp(&dev->subsys.pm, core_id, opp_idx);
}

int uiox_cpu_read_temp(uiox_cpu_device_t *dev,
                        uint8_t core_id, int8_t *temp_out)
{
    if (!dev || !dev->open) return -EINVAL;
    return uiox_cpu_hw_read_temp(dev->hw, core_id, temp_out);
}

void uiox_cpu_update_load(uiox_cpu_device_t *dev,
                           uint8_t core_id, uint32_t load_pct)
{
    if (!dev || !dev->open) return;
    uiox_cpu_pm_update_load(&dev->subsys.pm, core_id, load_pct);
}

void uiox_cpu_cache_flush(uiox_cpu_device_t *dev,
                           uintptr_t addr, size_t size)
{
    if (!dev || !dev->open) return;
    uiox_cpu_hw_cache_flush(dev->hw, addr, size);
}

int uiox_cpu_pmu_start(uiox_cpu_device_t *dev,
                        uint8_t counter_id, uint32_t event_id)
{
    if (!dev || !dev->open) return -EINVAL;
    return uiox_cpu_feat_pmu_start(&dev->subsys.feat, counter_id, event_id);
}

void uiox_cpu_pmu_stop(uiox_cpu_device_t *dev, uint8_t counter_id)
{
    if (!dev || !dev->open) return;
    uiox_cpu_feat_pmu_stop(&dev->subsys.feat, counter_id);
}

uint64_t uiox_cpu_pmu_read(uiox_cpu_device_t *dev, uint8_t counter_id)
{
    if (!dev || !dev->open) return 0;
    return uiox_cpu_feat_pmu_read(&dev->subsys.feat, counter_id);
}

uint32_t uiox_cpu_freq(const uiox_cpu_device_t *dev, uint8_t core_id)
{
    if (!dev || core_id >= UIOX_CPU_MAX_CORES) return 0;
    return dev->hw->cores[core_id].freq_mhz;
}

#if 1
uint64_t uiox_cpu_cycles(void)
{
#if defined(UIOX_ARCH_ARM64)
    uint64_t v; __asm__ volatile("mrs %0, pmccntr_el0":"=r"(v)); return v;
#elif defined(UIOX_ARCH_X86_64)
    return uiox_cpu_rdtsc();
#elif defined(UIOX_ARCH_RV64)
    return uiox_cpu_csr_read(mcycle);
#else
    return 0;
#endif
}
#endif
uint64_t uiox_cpu_timestamp(const uiox_cpu_device_t *dev)
{
    return uiox_cpu_hw_timestamp(dev ? dev->hw : NULL);
}

uint64_t uiox_cpu_uptime_ms(const uiox_cpu_device_t *dev)
{
    return dev ? dev->subsys.uptime_ms : 0;
}

uiox_cpu_core_state_t uiox_cpu_core_state(const uiox_cpu_device_t *dev,
                                           uint8_t core_id)
{
    if (!dev || core_id >= UIOX_CPU_MAX_CORES) return UIOX_CPU_STATE_OFFLINE;
    return dev->hw->cores[core_id].state;
}

void uiox_cpu_print_info(const uiox_cpu_device_t *dev)
{
    if (!dev) return;
    const uiox_cpu_hw_t *hw = dev->hw;
    printf("  Architecture   : %s\n", uiox_cpu_arch_name(hw->arch));
    printf("  Model          : %s\n", hw->model_str);
    printf("  Vendor         : %s\n", hw->vendor_str);
    printf("  Cores          : %u  clusters=%u  threads=%u\n",
           hw->num_cores, hw->num_clusters, hw->num_threads);
    printf("  Capabilities   : 0x%08X\n", hw->caps);
    uiox_cpu_feat_print(&dev->subsys.feat);
    printf("  Governor       : %s\n",
           uiox_cpu_gov_name(dev->subsys.pm.governor));
    printf("  OPPs available : %u\n", dev->subsys.pm.num_opps);
    for (uint8_t i = 0; i < dev->subsys.pm.num_opps; i++)
        printf("    OPP[%u]  %4u MHz  %7u µV  %5u mW\n",
               i,
               dev->subsys.pm.opps[i].freq_mhz,
               dev->subsys.pm.opps[i].voltage_uv,
               dev->subsys.pm.opps[i].power_mw);
}

void uiox_cpu_print_stats(const uiox_cpu_device_t *dev)
{
    if (!dev) return;
    const uiox_cpu_subsys_t *s = &dev->subsys;
    printf("  Subsys state   : %s\n", uiox_cpu_state_name(s->state));
    printf("  Uptime         : %llu ms\n", (unsigned long long)s->uptime_ms);
    printf("  Timer ticks    : %u\n",   s->timer_ticks);
    printf("  IPI count      : %u\n",   s->ipi_count);
    printf("  Fault count    : %u\n",   s->fault_count);
    printf("  Throttle events: %u\n",   s->pm.throttle_count);
    for (uint8_t i = 0; i < dev->hw->num_cores; i++) {
        const uiox_cpu_core_t *c = &dev->hw->cores[i];
        static const char *cstate[] = {"OFFLINE","ONLINE","IDLE","RUNNING","HOTPLUG"};
        printf("  Core[%u]  %4u MHz  %3d°C  load=%3u%%  state=%s\n",
               i, c->freq_mhz, c->temp_celsius,
               s->pm.load_pct[i],
               cstate[c->state < 5 ? c->state : 0]);
    }
}

const char *uiox_cpu_arch_name(uiox_cpu_arch_t arch)
{
    switch (arch) {
    case UIOX_CPU_ARCH_ARM64:  return "ARM64 (Cortex-A76 / ARMv8.2-A)";
    case UIOX_CPU_ARCH_X86_64: return "x86-64 (AMD64 / Intel 64)";
    case UIOX_CPU_ARCH_RV64:   return "RISC-V RV64GC";
    default:                    return "Unknown";
    }
}

const char *uiox_cpu_state_name(uiox_cpu_subsys_state_t s)
{
    switch (s) {
    case UIOX_CPU_SUBSYS_STOPPED:   return "STOPPED";
    case UIOX_CPU_SUBSYS_BOOTING:   return "BOOTING";
    case UIOX_CPU_SUBSYS_RUNNING:   return "RUNNING";
    case UIOX_CPU_SUBSYS_SUSPENDED: return "SUSPENDED";
    default:                         return "UNKNOWN";
    }
}

const char *uiox_cpu_evt_name(uiox_cpu_evt_t evt)
{
    switch (evt) {
    case UIOX_CPU_EVT_CORE_ONLINE:       return "CORE_ONLINE";
    case UIOX_CPU_EVT_CORE_OFFLINE:      return "CORE_OFFLINE";
    case UIOX_CPU_EVT_FREQ_CHANGE:       return "FREQ_CHANGE";
    case UIOX_CPU_EVT_THERMAL_THROTTLE:  return "THERMAL_THROTTLE";
    case UIOX_CPU_EVT_FAULT:             return "FAULT";
    case UIOX_CPU_EVT_IPI_RECEIVED:      return "IPI_RECEIVED";
    case UIOX_CPU_EVT_TIMER_TICK:        return "TIMER_TICK";
    default:                              return "UNKNOWN";
    }
}

const char *uiox_cpu_gov_name(uiox_cpu_governor_t gov)
{
    switch (gov) {
    case UIOX_CPU_GOV_PERFORMANCE:   return "performance";
    case UIOX_CPU_GOV_POWERSAVE:     return "powersave";
    case UIOX_CPU_GOV_ONDEMAND:      return "ondemand";
    case UIOX_CPU_GOV_SCHEDUTIL:     return "schedutil";
    case UIOX_CPU_GOV_CONSERVATIVE:  return "conservative";
    default:                          return "unknown";
    }
}
