/**
 * @file    uiox_soc_power.c
 * @brief   UIOX SoC — Power management (PSCI for ARM, ACPI for x86).
 * @version 1.0.1
 * @date    2026-06-27
 */

 #include "uiox_soc.h"

 uiox_soc_err_t uiox_soc_power_init(uiox_soc_power_ctx_t *ctx)
 {
     if (!ctx) return UIOX_SOC_ERR_INVAL;
     uiox_soc_memset(ctx, 0, sizeof(*ctx));
     ctx->system_state = UIOX_SOC_PWR_ACTIVE;
 
 #if defined(__aarch64__) || defined(__arm__)
     ctx->psci_available = true;
     ctx->num_cpus       = 4u;
 #else
     ctx->acpi_available = true;
     ctx->num_cpus       = 4u;
 #endif
 
     for (uint32_t i = 0u; i < ctx->num_cpus; i++)
         ctx->cpu_state[i] = (i == 0u)
                             ? UIOX_SOC_CPU_ON
                             : UIOX_SOC_CPU_OFF;
 
     SOC_LOG("PWR", "init  cpus=%u  psci=%d  acpi=%d",
             ctx->num_cpus,
             (int)ctx->psci_available,
             (int)ctx->acpi_available);
     return UIOX_SOC_OK;
 }
 
 void uiox_soc_power_idle(void)
 {
 #if defined(__aarch64__) || defined(__arm__)
     __asm__ volatile("wfi" ::: "memory");
 #elif defined(__x86_64__)
     __asm__ volatile("hlt" ::: "memory");
 #endif
 }
 
 uiox_soc_err_t uiox_soc_power_cpu_on(uiox_soc_power_ctx_t *ctx,
                                        uint32_t  cpu_id,
                                        uintptr_t entry_pa)
 {
     if (!ctx || cpu_id >= ctx->num_cpus) return UIOX_SOC_ERR_INVAL;
 
 #if defined(__aarch64__)
     register uint64_t x0 __asm__("x0") = PSCI_CPU_ON;
     register uint64_t x1 __asm__("x1") = (uint64_t)cpu_id;
     register uint64_t x2 __asm__("x2") = (uint64_t)entry_pa;
     register uint64_t x3 __asm__("x3") = 0u;
     __asm__ volatile("hvc #0"
                      : "=r"(x0)
                      : "r"(x0), "r"(x1), "r"(x2), "r"(x3)
                      : "memory");
     if (x0 == 0u) ctx->cpu_state[cpu_id] = UIOX_SOC_CPU_ON;
     return (x0 == 0u) ? UIOX_SOC_OK : UIOX_SOC_ERR_GENERIC;
 #else
     UIOX_SOC_UNUSED(entry_pa);
     return UIOX_SOC_ERR_UNSUP;
 #endif
 }
 
 uiox_soc_err_t uiox_soc_power_cpu_off(uiox_soc_power_ctx_t *ctx,
                                         uint32_t cpu_id)
 {
     if (!ctx || cpu_id == 0u || cpu_id >= ctx->num_cpus)
         return UIOX_SOC_ERR_INVAL;
     ctx->cpu_state[cpu_id] = UIOX_SOC_CPU_OFF;
     SOC_LOG("PWR", "CPU %u off", cpu_id);
     return UIOX_SOC_OK;
 }
 
 void __attribute__((noreturn)) uiox_soc_power_reset(void)
 {
 #if defined(__aarch64__)
     register uint64_t x0 __asm__("x0") = PSCI_SYSTEM_RESET;
     __asm__ volatile("hvc #0" :: "r"(x0));
 #elif defined(__arm__)
     /* VersatilePB system controller soft-reset */
     soc_mmio_write32(0x10000040u, 0x100u);
 #elif defined(__x86_64__)
     /* Keyboard controller reset line */
     __asm__ volatile("outb %%al, $0x64" :: "a"((uint8_t)0xFEu));
 #endif
     for (;;) ;
 }
 
 void __attribute__((noreturn)) uiox_soc_power_shutdown(void)
 {
 #if defined(__aarch64__)
     register uint64_t x0 __asm__("x0") = PSCI_SYSTEM_OFF;
     __asm__ volatile("hvc #0" :: "r"(x0));
 
 #elif defined(__arm__)
     /* ARM32 has no ACPI — disable IRQs and spin */
     __asm__ volatile("cpsid if" ::: "memory");
 
 #elif defined(__x86_64__)
     /*
      * QEMU q35 ACPI S5 power-off.
      * outw is x86-only — guarded so it never compiles on ARM.
      */
     __asm__ volatile(
         "outw %0, %1"
         :: "a"((uint16_t)(ACPI_S5_SLEEP_TYPE | ACPI_SLP_EN)),
            "dN"((uint16_t)ACPI_PM1A_CNT_BLOCK)
     );
 #endif
     for (;;) ;
 }
 