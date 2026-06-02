/**
 * @file    uiox_cpu_hw.c
 * @brief   UIOX CPU/SoC HAL — generic hardware lifecycle management.
 * @date    2026-06-02
 */

 #include "uiox_cpu_hw.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_cpu_hw_init(uiox_cpu_hw_t *hw, const uiox_cpu_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv = (void *)ops;
     for (uint8_t i = 0; i < UIOX_CPU_MAX_CORES; i++) {
         hw->cores[i].core_id = i;
         hw->cores[i].state   = UIOX_CPU_STATE_OFFLINE;
     }
     return ops->init(hw);
 }
 
 void uiox_cpu_hw_deinit(uiox_cpu_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_cpu_hw_ops_t *ops = (const uiox_cpu_hw_ops_t *)hw->priv;
     if (ops->deinit) ops->deinit(hw);
     hw->priv = NULL;
 }
 
 int uiox_cpu_hw_detect(uiox_cpu_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_cpu_hw_ops_t *ops = (const uiox_cpu_hw_ops_t *)hw->priv;
     if (!ops->detect) return -ENOSYS;
     return ops->detect(hw);
 }
 
 int uiox_cpu_hw_core_powerup(uiox_cpu_hw_t *hw,
    uint8_t core_id, uintptr_t entry)
{
if (!hw || !hw->priv) return -EINVAL;
const uiox_cpu_hw_ops_t *ops = (const uiox_cpu_hw_ops_t *)hw->priv;
if (!ops->core_powerup) return -ENOSYS;
int rc = ops->core_powerup(hw, core_id, entry);
if (rc == 0 && core_id < UIOX_CPU_MAX_CORES)
hw->cores[core_id].state = UIOX_CPU_STATE_ONLINE;
return rc;
}

int uiox_cpu_hw_set_freq(uiox_cpu_hw_t *hw,
uint8_t core_id, uint32_t freq_mhz)
{
if (!hw || !hw->priv) return -EINVAL;
const uiox_cpu_hw_ops_t *ops = (const uiox_cpu_hw_ops_t *)hw->priv;
if (!ops->set_freq) return -ENOSYS;
int rc = ops->set_freq(hw, core_id, freq_mhz);
if (rc == 0 && core_id < UIOX_CPU_MAX_CORES)
hw->cores[core_id].freq_mhz = freq_mhz;
return rc;
}

int uiox_cpu_hw_read_temp(uiox_cpu_hw_t *hw,
 uint8_t core_id, int8_t *temp)
{
if (!hw || !hw->priv || !temp) return -EINVAL;
const uiox_cpu_hw_ops_t *ops = (const uiox_cpu_hw_ops_t *)hw->priv;
if (!ops->read_temp) return -ENOSYS;
int rc = ops->read_temp(hw, core_id, temp);
if (rc == 0 && core_id < UIOX_CPU_MAX_CORES)
hw->cores[core_id].temp_celsius = *temp;
return rc;
}

void uiox_cpu_hw_cache_flush(uiox_cpu_hw_t *hw,
    uintptr_t addr, size_t size)
{
if (!hw || !hw->priv) return;
const uiox_cpu_hw_ops_t *ops = (const uiox_cpu_hw_ops_t *)hw->priv;
if (ops->cache_flush) ops->cache_flush(hw, addr, size);
}

int uiox_cpu_hw_ipi_send(uiox_cpu_hw_t *hw,
uint8_t target, uint8_t vector)
{
if (!hw || !hw->priv) return -EINVAL;
const uiox_cpu_hw_ops_t *ops = (const uiox_cpu_hw_ops_t *)hw->priv;
if (!ops->ipi_send) return -ENOSYS;
return ops->ipi_send(hw, target, vector);
}

uint64_t uiox_cpu_hw_timestamp(const uiox_cpu_hw_t *hw)
{
(void)hw;
#if defined(UIOX_ARCH_ARM64)
uint64_t t;
__asm__ volatile("mrs %0, cntvct_el0" : "=r"(t));
return t;
#elif defined(UIOX_ARCH_X86_64)
return uiox_cpu_rdtsc();
#elif defined(UIOX_ARCH_RV64)
return uiox_cpu_csr_read(time);
#else
return 0;
#endif
}
