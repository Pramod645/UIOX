#ifndef CPU_DRV_APIC_H
#define CPU_DRV_APIC_H
/*
 * cpu_drv_apic.h - x86-64 LAPIC / IOAPIC driver interface
 * Reference: Intel SDM Vol. 3A, Chapter 10
 */
#include "../cpu_types.h"
#include "../cpu_irq.h"

/* -- Local APIC base ---------------------------------------- */
#define LAPIC_DEFAULT_BASE  0xFEE00000u

/* -- IOAPIC base -------------------------------------------- */
#define IOAPIC_DEFAULT_BASE 0xFEC00000u
#define IOAPIC_REGSEL       0x00u
#define IOAPIC_IOWIN        0x10u
#define IOAPIC_REDTBL_BASE  0x10u   /* redirection table entry 0 */

/* -- LAPIC timer modes -------------------------------------- */
#define LAPIC_TIMER_ONESHOT     0x00000000u
#define LAPIC_TIMER_PERIODIC    0x00020000u
#define LAPIC_TIMER_TSCDEADLINE 0x00040000u

/* -- LAPIC IPI delivery modes ------------------------------- */
#define LAPIC_DM_FIXED      0x00000u
#define LAPIC_DM_LOWEST_PRI 0x00100u
#define LAPIC_DM_SMI        0x00200u
#define LAPIC_DM_NMI        0x00400u
#define LAPIC_DM_INIT       0x00500u
#define LAPIC_DM_STARTUP    0x00600u

/* -- LAPIC destination modes -------------------------------- */
#define LAPIC_DEST_PHYSICAL  0x000000u
#define LAPIC_DEST_LOGICAL   0x800000u
#define LAPIC_DEST_SELF      (2u << 18)
#define LAPIC_DEST_ALL       (3u << 18)

/* -- APIC context ------------------------------------------- */
typedef struct apic_ctx {
    cpu_addr_t lapic_base;
    cpu_addr_t ioapic_base;
    cpu_u32_t  lapic_id;
    cpu_u32_t  ioapic_id;
    cpu_u32_t  ioapic_max_redir;
    cpu_bool_t x2apic;
} apic_ctx_t;

extern apic_ctx_t g_apic;

/* -- LAPIC API ---------------------------------------------- */
int  lapic_init          (cpu_addr_t base);
void lapic_enable        (void);
void lapic_eoi           (void);
void lapic_send_ipi      (cpu_u32_t dest, cpu_u32_t vec,
                           cpu_u32_t delivery);
void lapic_send_init_ipi (cpu_u32_t apic_id);
void lapic_send_sipi     (cpu_u32_t apic_id, cpu_u8_t vector);
void lapic_timer_init    (cpu_u32_t vector, cpu_u32_t count,
                           cpu_u32_t mode);
void lapic_timer_stop    (void);
cpu_u32_t lapic_get_id   (void);
void lapic_write         (cpu_u32_t reg, cpu_u32_t val);
cpu_u32_t lapic_read     (cpu_u32_t reg);

/* -- IOAPIC API --------------------------------------------- */
int  ioapic_init         (cpu_addr_t base);
void ioapic_set_redir    (cpu_u32_t irq, cpu_u8_t vector,
                           cpu_u32_t flags, cpu_u8_t dest);
void ioapic_mask_irq     (cpu_u32_t irq);
void ioapic_unmask_irq   (cpu_u32_t irq);
void ioapic_write        (cpu_u32_t reg, cpu_u32_t val);
cpu_u32_t ioapic_read    (cpu_u32_t reg);
void apic_print_info     (void);

/* -- 8259A PIC (legacy disable) ----------------------------- */
void pic8259_disable     (void);
void pic8259_init        (cpu_u8_t master_vec, cpu_u8_t slave_vec);
void pic8259_eoi         (cpu_u32_t irq);

#endif /* CPU_DRV_APIC_H */
