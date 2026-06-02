/*
 * cpu_drv_timer.c - Generic timer driver
 */
#include "../../include/drivers/cpu_drv_timer.h"
#include "../../include/cpu_regs.h"
#include <stdio.h>

cpu_drv_timer_ctx_t g_drv_timer;

int cpu_drv_timer_init(cpu_drv_timer_type_t type, cpu_addr_t clint_base)
{
    g_drv_timer.type       = type;
    g_drv_timer.clint_base = clint_base;
    g_drv_timer.callback   = NULL;
    g_drv_timer.cb_data    = NULL;

#if defined(UIOX_ARCH_ARM64)
    cpu_u64_t freq;
    CPU_MRS(CNTFRQ_EL0, freq);
    g_drv_timer.freq_hz = freq;
    g_drv_timer.irq     = 27u;  /* virtual timer PPI             */
    g_drv_timer.type    = TIMER_DRV_ARM_GENERIC;

#elif defined(UIOX_ARCH_X86_64)
    /* calibrate TSC against PIT channel 2 */
    g_drv_timer.freq_hz = 2400000000ULL; /* 2.4 GHz placeholder   */
    g_drv_timer.irq     = 0x20u;         /* IRQ0 LAPIC timer vec  */
    g_drv_timer.type    = TIMER_DRV_X86_LAPIC;

#elif defined(UIOX_ARCH_RISCV64)
    g_drv_timer.freq_hz = 10000000ULL;   /* 10 MHz CLINT          */
    g_drv_timer.irq     = 5u;            /* S-mode timer IRQ      */
    g_drv_timer.type    = TIMER_DRV_RISCV_CLINT;
#endif

    printf("[timer] type=%d  freq=%llu Hz  irq=%u\n",
           g_drv_timer.type,
           (unsigned long long)g_drv_timer.freq_hz,
           g_drv_timer.irq);
    return CPU_OK;
}

int cpu_drv_timer_set_period(cpu_u64_t period_ns)
{
    cpu_u64_t ticks = (period_ns * g_drv_timer.freq_hz) / 1000000000ULL;

#if defined(UIOX_ARCH_ARM64)
    cpu_u64_t cval;
    CPU_MRS(CNTVCT_EL0, cval);
    cval += ticks;
    CPU_MSR(CNTV_CVAL_EL0, cval);
    cpu_u64_t ctl = CNTV_CTL_ENABLE;
    CPU_MSR(CNTV_CTL_EL0, ctl);

#elif defined(UIOX_ARCH_X86_64)
    lapic_timer_init(g_drv_timer.lapic_vec,
                      (cpu_u32_t)ticks,
                      LAPIC_TIMER_PERIODIC);

#elif defined(UIOX_ARCH_RISCV64)
    cpu_u64_t mtime = cpu_mmio_read64(
                          g_drv_timer.clint_base + CLINT_MTIME_OFFSET);
    cpu_u64_t cmp   = mtime + ticks;
    /* mtimecmp[hartid] */
    cpu_mmio_write64(g_drv_timer.clint_base + CLINT_MTIMECMP_OFFSET, cmp);
    /* enable S-mode timer interrupt */
    CPU_CSR_SET(sie, MIE_STIE);
#endif
    return CPU_OK;
}

void cpu_drv_timer_start(cpu_timer_cb_t cb, void *data)
{
    g_drv_timer.callback = cb;
    g_drv_timer.cb_data  = data;
}

void cpu_drv_timer_stop(void)
{
#if defined(UIOX_ARCH_ARM64)
    cpu_u64_t ctl = CNTV_CTL_IMASK;
    CPU_MSR(CNTV_CTL_EL0, ctl);
#elif defined(UIOX_ARCH_X86_64)
    lapic_timer_stop();
#elif defined(UIOX_ARCH_RISCV64)
    CPU_CSR_CLR(sie, MIE_STIE);
#endif
}

cpu_u64_t cpu_drv_timer_get_count(void)
{
#if defined(UIOX_ARCH_ARM64)
    cpu_u64_t v; CPU_MRS(CNTVCT_EL0, v); return v;
#elif defined(UIOX_ARCH_X86_64)
    return cpu_read_tsc();
#elif defined(UIOX_ARCH_RISCV64)
    return cpu_mmio_read64(g_drv_timer.clint_base + CLINT_MTIME_OFFSET);
#else
    return 0;
#endif
}

cpu_u64_t cpu_drv_timer_get_freq(void)
{ return g_drv_timer.freq_hz; }

void cpu_drv_timer_handler(void)
{
    /* re-arm for next period */
    cpu_drv_timer_set_period(
        g_drv_timer.freq_hz > 0
        ? (1000000000ULL / 100u)  /* 100 Hz default */
        : 10000000ULL);

    if (g_drv_timer.callback)
        g_drv_timer.callback(g_drv_timer.cb_data);
}

void cpu_drv_timer_print(void)
{
    printf("[timer] freq=%llu Hz  irq=%u  running=%u\n",
           (unsigned long long)g_drv_timer.freq_hz,
           g_drv_timer.irq,
           g_drv_timer.callback ? 1u : 0u);
}
