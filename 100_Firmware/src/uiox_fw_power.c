/**
 * @file  uiox_fw_power.c
 * @brief UIOX Firmware — Power management (PSCI / ACPI).
 * @date  2026-06-21
 */

 #include "uiox_fw.h"

 uiox_fw_err_t uiox_fw_power_init(uiox_fw_power_ctx_t *ctx)
 {
     if (!ctx) return UIOX_FW_ERR_INVAL;
     uiox_fw_memset(ctx, 0, sizeof(*ctx));
     ctx->system_state = UIOX_FW_PWR_ACTIVE;
 
 #if defined(__aarch64__) || defined(__arm__)
     ctx->psci_available = true;
     ctx->num_cpus       = 4u;  /* QEMU virt default */
 #else
     ctx->acpi_available = true;
     ctx->num_cpus       = 4u;
 #endif
     for (uint32_t i = 0u; i < ctx->num_cpus; i++)
         ctx->cpu_state[i] = (i == 0u) ? UIOX_FW_CPU_ON : UIOX_FW_CPU_OFF;
 
     FW_LOG("PWR", "init  cpus=%u  psci=%d  acpi=%d",
            ctx->num_cpus,
            (int)ctx->psci_available,
            (int)ctx->acpi_available);
     return UIOX_FW_OK;
 }
 
 void uiox_fw_power_idle(void)
 {
 #if defined(__aarch64__) || defined(__arm__)
     __asm__ volatile("wfi" ::: "memory");
 #else
     __asm__ volatile("hlt" ::: "memory");
 #endif
 }
 
 uiox_fw_err_t uiox_fw_power_cpu_on(uiox_fw_power_ctx_t *ctx,
                                      uint32_t cpu_id,
                                      uintptr_t entry_pa)
 {
     if (!ctx || cpu_id >= ctx->num_cpus) return UIOX_FW_ERR_INVAL;
 #if defined(__aarch64__)
     /* PSCI CPU_ON via HVC */
     register uint64_t x0 __asm__("x0") = PSCI_CPU_ON;
     register uint64_t x1 __asm__("x1") = (uint64_t)cpu_id;
     register uint64_t x2 __asm__("x2") = (uint64_t)entry_pa;
     register uint64_t x3 __asm__("x3") = 0u;
     __asm__ volatile("hvc #0"
                      : "=r"(x0)
                      : "r"(x0),"r"(x1),"r"(x2),"r"(x3)
                      : "memory");
     if (x0 == 0u) ctx->cpu_state[cpu_id] = UIOX_FW_CPU_ON;
     return (x0 == 0u) ? UIOX_FW_OK : UIOX_FW_ERR_GENERIC;
 #else
     UIOX_FW_UNUSED(entry_pa);
     return UIOX_FW_ERR_UNSUP;
 #endif
 }
 
 uiox_fw_err_t uiox_fw_power_cpu_off(uiox_fw_power_ctx_t *ctx,
                                       uint32_t cpu_id)
 {
     if (!ctx || cpu_id == 0u || cpu_id >= ctx->num_cpus)
         return UIOX_FW_ERR_INVAL;
     ctx->cpu_state[cpu_id] = UIOX_FW_CPU_OFF;
     FW_LOG("PWR", "CPU %u off", cpu_id);
     return UIOX_FW_OK;
 }
 
 void __attribute__((noreturn)) uiox_fw_power_reset(void)
 {
 #if defined(__aarch64__)
     register uint64_t x0 __asm__("x0") = PSCI_SYSTEM_RESET;
     __asm__ volatile("hvc #0" :: "r"(x0));
 #elif defined(__arm__)
     /* Watchdog reset (versatilepb) */
     fw_mmio_write32(0x10000040u, 0x100u);
 #else
     /* x86: keyboard controller reset */
     __asm__ volatile("outb %%al, $0x64" :: "a"((uint8_t)0xFE));
 #endif
     for (;;) ;
 }
 
 void __attribute__((noreturn)) uiox_fw_power_shutdown(void)
 {
 #if defined(__aarch64__)
     register uint64_t x0 __asm__("x0") = PSCI_SYSTEM_OFF;
     __asm__ volatile("hvc #0" :: "r"(x0));
 #else
     /* ACPI S5 via QEMU q35 PM register */
     __asm__ volatile("outw %%ax, %%dx"
                      :: "a"((uint16_t)(ACPI_S5_SLEEP_TYPE | ACPI_SLP_EN)),
                         "d"((uint16_t)ACPI_PM1A_CNT_BLOCK));
 #endif
     for (;;) ;
 }
 