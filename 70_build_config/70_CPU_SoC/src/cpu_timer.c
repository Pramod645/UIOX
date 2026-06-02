/*
 * cpu_timer.c - System timer abstraction
 */
#include "../include/cpu_timer.h"
#include "../include/cpu_regs.h"
#include <stdio.h>

int cpu_timer_init(cpu_timer_t *t)
{
    if (!t) return CPU_ERR;
    t->running   = CPU_FALSE;
    t->callback  = NULL;
    t->cb_data   = NULL;
#if defined(UIOX_ARCH_ARM64)
    CPU_MRS(CNTFRQ_EL0, t->freq_hz);
#elif defined(UIOX_ARCH_X86_64)
    t->freq_hz = 1000000000ULL; /* TSC-based, calibrated later */
#elif defined(UIOX_ARCH_RISCV64)
    t->freq_hz = 10000000ULL;   /* 10 MHz default CLINT clock  */
#endif
    t->irq = 0;
    return CPU_OK;
}

int cpu_timer_set_period(cpu_timer_t *t, cpu_u64_t period_ns)
{
    if (!t) return CPU_ERR;
    t->period_ns = period_ns;
    return CPU_OK;
}

int cpu_timer_start(cpu_timer_t *t, cpu_timer_cb_t cb, void *data)
{
    if (!t) return CPU_ERR;
    t->callback  = cb;
    t->cb_data   = data;
    t->running   = CPU_TRUE;
    return CPU_OK;
}

void cpu_timer_stop(cpu_timer_t *t)
{
    if (t) t->running = CPU_FALSE;
}

cpu_u64_t cpu_timer_get_ticks(void)
{
#if defined(UIOX_ARCH_ARM64)
    cpu_u64_t v; CPU_MRS(CNTVCT_EL0, v); return v;
#elif defined(UIOX_ARCH_X86_64)
    return cpu_read_tsc();
#elif defined(UIOX_ARCH_RISCV64)
    cpu_u64_t v; CPU_CSR_READ(time, v); return v;
#else
    return 0;
#endif
}

cpu_u64_t cpu_timer_get_freq(void)   { return g_cpu_id.freq_mhz * 1000000ULL; }
cpu_u64_t cpu_timer_ticks_to_ns(cpu_u64_t t) { return t * 1000000000ULL / cpu_timer_get_freq(); }
cpu_u64_t cpu_timer_ns_to_ticks(cpu_u64_t ns){ return ns * cpu_timer_get_freq() / 1000000000ULL; }

void cpu_timer_udelay(cpu_u64_t us)
{
    cpu_u64_t start = cpu_timer_get_ticks();
    cpu_u64_t ticks = cpu_timer_ns_to_ticks(us * 1000ULL);
    while ((cpu_timer_get_ticks() - start) < ticks)
        cpu_nop();
}

void cpu_timer_mdelay(cpu_u64_t ms)
{
    cpu_timer_udelay(ms * 1000ULL);
}
