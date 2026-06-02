#ifndef CPU_TIMER_H
#define CPU_TIMER_H
/*
 * cpu_timer.h - System timer interface
 * ARM: Generic Timer (CNTV_CTL_EL0)
 * x86: LAPIC timer / TSC
 * RISC-V: mtime / mtimecmp
 */
#include "cpu_types.h"

typedef void (*cpu_timer_cb_t)(void *data);

typedef struct cpu_timer {
    cpu_u64_t      freq_hz;       /* timer frequency              */
    cpu_u64_t      period_ns;     /* configured period            */
    cpu_timer_cb_t callback;
    void          *cb_data;
    cpu_bool_t     running;
    cpu_u32_t      irq;
} cpu_timer_t;

int       cpu_timer_init        (cpu_timer_t *t);
int       cpu_timer_set_period  (cpu_timer_t *t, cpu_u64_t period_ns);
int       cpu_timer_start       (cpu_timer_t *t,
                                  cpu_timer_cb_t cb, void *data);
void      cpu_timer_stop        (cpu_timer_t *t);
cpu_u64_t cpu_timer_get_ticks   (void);
cpu_u64_t cpu_timer_get_freq    (void);
cpu_u64_t cpu_timer_ticks_to_ns (cpu_u64_t ticks);
cpu_u64_t cpu_timer_ns_to_ticks (cpu_u64_t ns);
void      cpu_timer_udelay      (cpu_u64_t us);
void      cpu_timer_mdelay      (cpu_u64_t ms);

#endif /* CPU_TIMER_H */
