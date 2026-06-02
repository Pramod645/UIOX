#ifndef CPU_SMP_H
#define CPU_SMP_H
/*
 * cpu_smp.h - SMP / multi-core interface
 */
#include "cpu_types.h"

#define CPU_MAX_CORES  16

typedef struct cpu_core_info {
    cpu_u32_t  id;
    cpu_u32_t  phys_id;      /* MPIDR Aff0 / APIC ID / hart ID  */
    cpu_bool_t online;
    cpu_bool_t primary;
    cpu_addr_t stack_top;
    cpu_u64_t  boot_time_ns;
} cpu_core_info_t;

extern cpu_core_info_t g_cores[CPU_MAX_CORES];
extern volatile cpu_u32_t g_online_cores;

/* -- API ---------------------------------------------------- */
int       cpu_smp_init         (void);
cpu_u32_t cpu_smp_core_id      (void);  /* current core index     */
cpu_u32_t cpu_smp_phys_id      (void);  /* MPIDR / APIC / hartid  */
cpu_u32_t cpu_smp_num_cores    (void);
int       cpu_smp_boot_core    (cpu_u32_t core_id,
                                 cpu_addr_t entry,
                                 cpu_addr_t stack);
void      cpu_smp_send_ipi     (cpu_u32_t core_mask, cpu_u32_t ipi_id);
void      cpu_smp_broadcast_ipi(cpu_u32_t ipi_id);
void      cpu_smp_barrier      (void);   /* all-core sync barrier  */
void      cpu_smp_print_info   (void);

/* -- Spinlock (bare-metal) ---------------------------------- */
typedef volatile cpu_u32_t cpu_spinlock_t;
#define CPU_SPINLOCK_INIT  0u

void cpu_spin_lock   (cpu_spinlock_t *lock);
void cpu_spin_unlock (cpu_spinlock_t *lock);
int  cpu_spin_trylock(cpu_spinlock_t *lock);

#endif /* CPU_SMP_H */
