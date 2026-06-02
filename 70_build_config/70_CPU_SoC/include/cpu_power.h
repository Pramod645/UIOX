#ifndef CPU_POWER_H
#define CPU_POWER_H
/*
 * cpu_power.h - CPU power management
 */
#include "cpu_types.h"

typedef enum cpu_power_state {
    CPU_PWR_RUN     = 0,
    CPU_PWR_IDLE    = 1,   /* WFI / HLT                        */
    CPU_PWR_STANDBY = 2,   /* clock-gated                       */
    CPU_PWR_SUSPEND = 3,   /* PSCI SYSTEM_SUSPEND / ACPI S3     */
    CPU_PWR_OFF     = 4,   /* PSCI CPU_OFF                      */
} cpu_power_state_t;

/* -- PSCI function IDs (ARM) -------------------------------- */
#define PSCI_CPU_ON         0x84000003u
#define PSCI_CPU_OFF        0x84000002u
#define PSCI_CPU_SUSPEND    0x84000001u
#define PSCI_SYSTEM_RESET   0x84000009u
#define PSCI_SYSTEM_OFF     0x84000008u

/* -- x86 ACPI power states ---------------------------------- */
#define ACPI_S0  0   /* running                                 */
#define ACPI_S3  3   /* suspend to RAM                          */
#define ACPI_S5  5   /* soft off                                */

/* -- API ---------------------------------------------------- */
void cpu_idle          (void);               /* WFI / HLT / WFI  */
void cpu_halt          (void);               /* permanent halt    */
int  cpu_core_on       (cpu_u32_t core_id,
                         cpu_addr_t entry,
                         cpu_u64_t ctx);
int  cpu_core_off      (void);
int  cpu_suspend       (cpu_power_state_t state, cpu_addr_t resume);
void cpu_system_reset  (void);
void cpu_system_off    (void);
cpu_power_state_t cpu_power_state(void);

/* -- Frequency scaling -------------------------------------- */
int  cpu_set_freq      (cpu_u32_t freq_mhz);
cpu_u32_t cpu_get_freq (void);

#endif /* CPU_POWER_H */
