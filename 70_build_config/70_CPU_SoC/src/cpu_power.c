/*
 * cpu_power.c - CPU power management
 */
#include "../include/cpu_power.h"
#include "../include/cpu_regs.h"
#include <stdio.h>

void cpu_idle(void)   { cpu_wfi(); }
void cpu_halt(void)   { cpu_irq_disable(); for(;;) cpu_wfi(); }

#if defined(UIOX_ARCH_ARM64)
/* PSCI via SMC/HVC */
static cpu_s64_t psci_call(cpu_u64_t fn,
                             cpu_u64_t a0, cpu_u64_t a1, cpu_u64_t a2)
{
    cpu_s64_t ret;
    __asm__ volatile(
        "mov x0, %1\n\t"
        "mov x1, %2\n\t"
        "mov x2, %3\n\t"
        "mov x3, %4\n\t"
        "smc #0\n\t"
        "mov %0, x0\n\t"
        : "=r"(ret)
        : "r"(fn), "r"(a0), "r"(a1), "r"(a2)
        : "x0","x1","x2","x3","memory");
    return ret;
}

int cpu_core_on(cpu_u32_t core_id, cpu_addr_t entry, cpu_u64_t ctx)
{ return (int)psci_call(PSCI_CPU_ON, core_id, entry, ctx); }

int cpu_core_off(void)
{ return (int)psci_call(PSCI_CPU_OFF, 0, 0, 0); }

int cpu_suspend(cpu_power_state_t state, cpu_addr_t resume)
{ return (int)psci_call(PSCI_CPU_SUSPEND, state, resume, 0); }

void cpu_system_reset(void)
{ psci_call(PSCI_SYSTEM_RESET, 0, 0, 0); for(;;); }

void cpu_system_off(void)
{ psci_call(PSCI_SYSTEM_OFF, 0, 0, 0); for(;;); }

#elif defined(UIOX_ARCH_X86_64)
int cpu_core_on(cpu_u32_t core_id, cpu_addr_t entry, cpu_u64_t ctx)
{
    (void)core_id; (void)entry; (void)ctx;
    /* Trigger INIT/
    /* Trigger INIT/SIPI via LAPIC — handled in cpu_drv_apic.c */
    return CPU_ENOSUP;
}

int cpu_core_off(void)
{
    cpu_irq_disable();
    for (;;) cpu_wfi();
    return CPU_OK;
}

int cpu_suspend(cpu_power_state_t state, cpu_addr_t resume)
{
    (void)state; (void)resume;
    cpu_wfi();
    return CPU_OK;
}

void cpu_system_reset(void)
{
    /* write 0xFE to port 0x64 (keyboard controller reset line) */
    __asm__ volatile("outb %0, %1" :: "a"((cpu_u8_t)0xFE), "Nd"((cpu_u16_t)0x64));
    for (;;) cpu_wfi();
}

void cpu_system_off(void)
{
    /* ACPI S5: write SLP_TYP + SLP_EN to PM1a_CNT */
    cpu_outw(0x604, 0x2000); /* QEMU ACPI off */
    for (;;) cpu_wfi();
}

#elif defined(UIOX_ARCH_RISCV64)
int cpu_core_on(cpu_u32_t core_id, cpu_addr_t entry, cpu_u64_t ctx)
{
    (void)core_id; (void)entry; (void)ctx;
    /* SBI HSM extension: sbi_hart_start */
    cpu_u64_t ret;
    __asm__ volatile(
        "li a7, 0x48534D\n\t"   /* SBI_EXT_HSM         */
        "li a6, 0\n\t"           /* SBI_HSM_HART_START  */
        "mv a0, %1\n\t"
        "mv a1, %2\n\t"
        "mv a2, %3\n\t"
        "ecall\n\t"
        "mv %0, a0\n\t"
        : "=r"(ret)
        : "r"((cpu_u64_t)core_id),
          "r"((cpu_u64_t)entry),
          "r"(ctx)
        : "a0","a1","a2","a6","a7","memory");
    return (ret == 0) ? CPU_OK : CPU_ERR;
}

int cpu_core_off(void)
{
    /* SBI HSM: sbi_hart_stop */
    __asm__ volatile(
        "li a7, 0x48534D\n\t"
        "li a6, 1\n\t"
        "ecall\n\t" ::: "a6","a7","memory");
    for (;;) cpu_wfi();
    return CPU_OK;
}

int cpu_suspend(cpu_power_state_t state, cpu_addr_t resume)
{
    (void)state; (void)resume;
    cpu_wfi();
    return CPU_OK;
}

void cpu_system_reset(void)
{
    /* SBI SRST extension */
    __asm__ volatile(
        "li a7, 0x53525354\n\t"
        "li a6, 0\n\t"
        "li a0, 0\n\t"   /* RESET_TYPE_COLD_REBOOT */
        "li a1, 0\n\t"   /* RESET_REASON_NONE       */
        "ecall\n\t" ::: "a0","a1","a6","a7","memory");
    for (;;) cpu_wfi();
}

void cpu_system_off(void)
{
    __asm__ volatile(
        "li a7, 0x53525354\n\t"
        "li a6, 0\n\t"
        "li a0, 1\n\t"   /* RESET_TYPE_SHUTDOWN */
        "li a1, 0\n\t"
        "ecall\n\t" ::: "a0","a1","a6","a7","memory");
    for (;;) cpu_wfi();
}
#endif

cpu_power_state_t cpu_power_state(void) { return CPU_PWR_RUN; }
cpu_u32_t cpu_get_freq(void) { return g_cpu_id.freq_mhz; }
int cpu_set_freq(cpu_u32_t freq_mhz)
{
    (void)freq_mhz;
    return CPU_ENOSUP;
}
