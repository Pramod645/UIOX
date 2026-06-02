#ifndef CPU_DRV_TIMER_H
#define CPU_DRV_TIMER_H
/*
 * cpu_drv_timer.h - Generic timer driver interface
 * ARM: Generic Timer (CNTV_CTL_EL0 / CNTV_CVAL_EL0)
 * x86: LAPIC Timer / TSC-deadline
 * RISC-V: mtime / mtimecmp
 */
#include "../cpu_types.h"
#include "../cpu_timer.h"

/* -- ARM Generic Timer registers ---------------------------- */
#define CNTV_CTL_ENABLE   (1u << 0)
#define CNTV_CTL_IMASK    (1u << 1)
#define CNTV_CTL_ISTATUS  (1u << 2)

/* -- x86 TSC-deadline MSR ----------------------------------- */
#define MSR_TSC_DEADLINE  0x6E0u

/* -- RISC-V timer MMIO (CLINT) ------------------------------ */
#define CLINT_MTIME_OFFSET    0x0BFF8u
#define CLINT_MTIMECMP_OFFSET 0x04000u

typedef enum cpu_drv_timer_type {
    TIMER_DRV_ARM_GENERIC = 0,
    TIMER_DRV_X86_LAPIC   = 1,
    TIMER_DRV_X86_TSC     = 2,
    TIMER_DRV_RISCV_CLINT = 3,
} cpu_drv_timer_type_t;

typedef struct cpu_drv_timer_ctx {
    cpu_drv_timer_type_t type;
    cpu_addr_t           clint_base;   /* RISC-V CLINT base        */
    cpu_u32_t            lapic_vec;    /* x86 LAPIC timer vector   */
    cpu_u64_t            freq_hz;
    cpu_u32_t            irq;
    cpu_timer_cb_t       callback;
    void                *cb_data;
} cpu_drv_timer_ctx_t;

extern cpu_drv_timer_ctx_t g_drv_timer;

int       cpu_drv_timer_init      (cpu_drv_timer_type_t type,
                                    cpu_addr_t clint_base);
int       cpu_drv_timer_set_period(cpu_u64_t period_ns);
void      cpu_drv_timer_start     (cpu_timer_cb_t cb, void *data);
void      cpu_drv_timer_stop      (void);
cpu_u64_t cpu_drv_timer_get_count (void);
cpu_u64_t cpu_drv_timer_get_freq  (void);
void      cpu_drv_timer_handler   (void);   /* called from IRQ    */
void      cpu_drv_timer_print     (void);

#endif /* CPU_DRV_TIMER_H */
