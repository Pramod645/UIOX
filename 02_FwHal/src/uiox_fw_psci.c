/**
 * @file    uiox_fw_psci.c
 * @brief   UIOX Firmware — PSCI 1.1 implementation.
 * @date    2026-07-06
 */
#include "uiox_fw.h"
#include <string.h>

/* ── CPU affinity helpers ─────────────────────────────────── */
static uint8_t cpu_index(uiox_fw_psci_ctx_t *ctx, uint64_t mpidr)
{
    /* strip RES1 bits and match by Aff0 (CPU within cluster) */
    uint64_t m = mpidr & 0x00FFFFFFULL;
    for (uint8_t i = 0; i < ctx->num_cpus; i++)
        if ((ctx->cpus[i].mpidr & 0x00FFFFFFULL) == m) return i;
    return 0xFFu;
}

/* ── Init ─────────────────────────────────────────────────── */
uiox_fw_err_t uiox_fw_psci_init(uiox_fw_psci_ctx_t *ctx,
                                   uint8_t num_cpus,
                                   uintptr_t warm_boot_entry)
{
    if (!ctx || num_cpus == 0 || num_cpus > UIOX_PSCI_MAX_CPUS)
        return UIOX_FW_ERR_INVAL;

    memset(ctx, 0, sizeof(*ctx));
    ctx->num_cpus        = num_cpus;
    ctx->warm_boot_entry = warm_boot_entry;

    /* CPU 0 is already on */
    ctx->cpus[0].state = PSCI_AFF_ON;
#if defined(__aarch64__)
    uint64_t mpidr;
    __asm__ volatile("mrs %0, MPIDR_EL1" : "=r"(mpidr));
    ctx->cpus[0].mpidr = mpidr & 0x00FFFFFFull;
#endif

    /* CPUs 1..N start off */
    for (uint8_t i = 1; i < num_cpus; i++) {
        ctx->cpus[i].state = PSCI_AFF_OFF;
        ctx->cpus[i].mpidr = (uint64_t)i;  /* Aff0 = cpu index */
    }

    uiox_fw_printf("  [psci] init: %u CPUs  warm_boot=0x%llx\n",
                    num_cpus, (unsigned long long)warm_boot_entry);
    return UIOX_FW_OK;
}

uiox_fw_err_t uiox_fw_psci_register(uiox_fw_psci_ctx_t *ctx)
{
    if (!ctx) return UIOX_FW_ERR_INVAL;
#if defined(__aarch64__)
    /* SMC is the conduit when running from EL3 */
    ctx->smc_enabled = true;
    ctx->registered  = true;
    uiox_fw_printf("  [psci] registered via SMC conduit\n");
#elif defined(__arm__)
    /* HVC conduit for ARM32 without EL3 */
    ctx->hvc_enabled = true;
    ctx->registered  = true;
    uiox_fw_printf("  [psci] registered via HVC conduit\n");
#else
    uiox_fw_printf("  [psci] ACPI power path (x86-64 stub)\n");
    ctx->registered = true;
#endif
    return UIOX_FW_OK;
}

/* ── PSCI function implementations ───────────────────────── */

uint32_t uiox_fw_psci_version(void)
{
    return PSCI_VERSION;
}

int64_t uiox_fw_psci_cpu_on(uiox_fw_psci_ctx_t *ctx,
                               uint64_t  target_cpu,
                               uintptr_t entry_point,
                               uint64_t  context_id)
{
    if (!ctx) return PSCI_RET_INVALID_PARAMS;

    uint8_t idx = cpu_index(ctx, target_cpu);
    if (idx == 0xFFu) return PSCI_RET_INVALID_PARAMS;
    if (ctx->cpus[idx].state == PSCI_AFF_ON)
        return PSCI_RET_ALREADY_ON;
    if (ctx->cpus[idx].state == PSCI_AFF_ON_PEND)
        return PSCI_RET_ON_PENDING;

    ctx->cpus[idx].entry_point = entry_point;
    ctx->cpus[idx].context_id  = context_id;
    ctx->cpus[idx].state       = PSCI_AFF_ON_PEND;
    ctx->cpus[idx].on_count++;
    ctx->total_cpu_on++;

    /*
     * Real implementation: write entry_point to the CPU's
     * spin-table hold address, then send SEV to wake it.
     *
     * For QEMU virt the secondary CPU spin-table is at
     * 0x40000000 + cpu_idx * 8.
     */
#if defined(__aarch64__)
    volatile uint64_t *hold =
        (volatile uint64_t *)(0x40000000ULL + (uint64_t)idx * 8ULL);
    *hold = (uint64_t)entry_point;
    __asm__ volatile("dc civac, %0\ndsb sy\nsev" :: "r"(hold) : "memory");
    ctx->cpus[idx].state = PSCI_AFF_ON;
#endif

    uiox_fw_printf("  [psci] CPU_ON cpu=0x%llx  entry=0x%llx\n",
                    (unsigned long long)target_cpu,
                    (unsigned long long)entry_point);
    return PSCI_RET_SUCCESS;
}

int64_t uiox_fw_psci_cpu_off(uiox_fw_psci_ctx_t *ctx)
{
    if (!ctx) return PSCI_RET_INVALID_PARAMS;

#if defined(__aarch64__)
    uint64_t mpidr;
    __asm__ volatile("mrs %0, MPIDR_EL1" : "=r"(mpidr));
    uint8_t idx = cpu_index(ctx, mpidr);
    if (idx == 0u) return PSCI_RET_DENIED; /* can't off primary CPU */
    if (idx != 0xFFu) {
        ctx->cpus[idx].state = PSCI_AFF_OFF;
        ctx->cpus[idx].off_count++;
        ctx->total_cpu_off++;
    }
    uiox_fw_printf("  [psci] CPU_OFF mpidr=0x%llx\n",
                    (unsigned long long)(mpidr & 0xFFFFFFFFull));
    for (;;) uiox_fw_hw_wfi();
#else
    return PSCI_RET_NOT_SUPPORTED;
#endif
}

int64_t uiox_fw_psci_cpu_suspend(uiox_fw_psci_ctx_t *ctx,
                                    uint32_t  power_state,
                                    uintptr_t entry_point,
                                    uint64_t  context_id)
{
    if (!ctx) return PSCI_RET_INVALID_PARAMS;
    (void)power_state; (void)entry_point; (void)context_id;

#if defined(__aarch64__)
    uint64_t mpidr;
    __asm__ volatile("mrs %0, MPIDR_EL1" : "=r"(mpidr));
    uint8_t idx = cpu_index(ctx, mpidr);
    if (idx != 0xFFu) {
        ctx->cpus[idx].suspend_count++;
        ctx->total_suspend++;
    }
    /* Simple WFI suspend — no power domain off for QEMU */
    uiox_fw_hw_wfi();
    return PSCI_RET_SUCCESS;
#else
    return PSCI_RET_NOT_SUPPORTED;
#endif
}

int64_t uiox_fw_psci_affinity_info(uiox_fw_psci_ctx_t *ctx,
                                     uint64_t target_affinity,
                                     uint32_t lowest_affinity_level)
{
    if (!ctx) return PSCI_RET_INVALID_PARAMS;
    (void)lowest_affinity_level;
    uint8_t idx = cpu_index(ctx, target_affinity);
    if (idx == 0xFFu) return PSCI_RET_INVALID_PARAMS;
    return (int64_t)ctx->cpus[idx].state;
}

void __attribute__((noreturn))
uiox_fw_psci_system_off(uiox_fw_psci_ctx_t *ctx)
{
    if (ctx) ctx->total_system_reset++;
    uiox_fw_printf("  [psci] SYSTEM_OFF\n");
#if defined(__aarch64__)
    /* PSCI SYSTEM_OFF via SMC */
    __asm__ volatile(
        "mov x0, #0x84000000\n\t"
        "movk x0, #0x0008, lsl #0\n\t"  /* PSCI_FN_SYSTEM_OFF */
        "smc #0\n\t" ::: "x0","memory");
#elif defined(__x86_64__)
    /* QEMU ACPI shutdown via port 0x604 */
    __asm__ volatile("outw %0, %1" :: "a"((uint16_t)0x2000), "Nd"((uint16_t)0x604));
#endif
    for (;;) uiox_fw_hw_wfi();
}

void __attribute__((noreturn))
uiox_fw_psci_system_reset(uiox_fw_psci_ctx_t *ctx)
{
    if (ctx) ctx->total_system_reset++;
    uiox_fw_printf("  [psci] SYSTEM_RESET\n");
#if defined(__aarch64__)
    __asm__ volatile(
        "mov x0, #0x84000000\n\t"
        "movk x0, #0x0009, lsl #0\n\t"  /* PSCI_FN_SYSTEM_RESET */
        "smc #0\n\t" ::: "x0","memory");
#elif defined(__arm__)
    __asm__ volatile(
        "mov r0, #0x84000000\n\t"
        "add r0, r0, #0x9\n\t"
        "smc #0\n\t" ::: "r0","memory");
#elif defined(__x86_64__)
    __asm__ volatile("outb %0, %1" :: "a"((uint8_t)0xFE), "Nd"((uint16_t)0x64));
#endif
    for (;;) uiox_fw_hw_wfi();
}

int64_t uiox_fw_psci_features(uint32_t fn_id)
{
    switch (fn_id) {
    case PSCI_FN_VERSION:           return PSCI_RET_SUCCESS;
    case PSCI_FN_CPU_SUSPEND:       return PSCI_RET_SUCCESS;
    case PSCI_FN_CPU_OFF:           return PSCI_RET_SUCCESS;
    case PSCI_FN_CPU_ON:            return PSCI_RET_SUCCESS;
    case PSCI_FN_AFFINITY_INFO:     return PSCI_RET_SUCCESS;
    case PSCI_FN_SYSTEM_OFF:        return PSCI_RET_SUCCESS;
    case PSCI_FN_SYSTEM_RESET:      return PSCI_RET_SUCCESS;
    case PSCI_FN_FEATURES:          return PSCI_RET_SUCCESS;
    default:                        return PSCI_RET_NOT_SUPPORTED;
    }
}

/* ── Main SMC dispatcher ─────────────────────────────────── */
int64_t uiox_fw_psci_dispatch(uiox_fw_psci_ctx_t *ctx,
                                 uint64_t fn_id,
                                 uint64_t arg0,
                                 uint64_t arg1,
                                 uint64_t arg2)
{
    if (!ctx) return PSCI_RET_NOT_SUPPORTED;

    switch ((uint32_t)fn_id) {
    case PSCI_FN_VERSION:
        return (int64_t)uiox_fw_psci_version();

    case PSCI_FN_CPU_ON:
        return uiox_fw_psci_cpu_on(ctx, arg0, (uintptr_t)arg1, arg2);

    case PSCI_FN_CPU_OFF:
        return uiox_fw_psci_cpu_off(ctx);

    case PSCI_FN_CPU_SUSPEND:
        return uiox_fw_psci_cpu_suspend(ctx, (uint32_t)arg0,
                                          (uintptr_t)arg1, arg2);

    case PSCI_FN_AFFINITY_INFO:
        return uiox_fw_psci_affinity_info(ctx, arg0, (uint32_t)arg1);

    case PSCI_FN_FEATURES:
        return uiox_fw_psci_features((uint32_t)arg0);

    case PSCI_FN_SYSTEM_OFF:
        uiox_fw_psci_system_off(ctx);
        /* never returns */

    case PSCI_FN_SYSTEM_RESET:
        uiox_fw_psci_system_reset(ctx);
        /* never returns */

    default:
        return PSCI_RET_NOT_SUPPORTED;
    }
}

void uiox_fw_psci_set_entry(uiox_fw_psci_ctx_t *ctx,
                               uint8_t cpu_idx, uintptr_t entry)
{
    if (!ctx || cpu_idx >= ctx->num_cpus) return;
    ctx->cpus[cpu_idx].entry_point = entry;
}

void uiox_fw_psci_print(const uiox_fw_psci_ctx_t *ctx)
{
    if (!ctx) return;
    static const char *st[] = {"ON","OFF","ON_PEND","?"};
    uiox_fw_printf("  PSCI v%u.%u:\n",
                    PSCI_VERSION_MAJOR, PSCI_VERSION_MINOR);
    uiox_fw_printf("    Registered  : %s  (SMC=%s  HVC=%s)\n",
                    ctx->registered  ? "YES":"NO",
                    ctx->smc_enabled ? "YES":"NO",
                    ctx->hvc_enabled ? "YES":"NO");
    uiox_fw_printf("    CPUs        : %u\n", ctx->num_cpus);
    for (uint8_t i = 0; i < ctx->num_cpus; i++) {
        const uiox_psci_cpu_t *c = &ctx->cpus[i];
        uiox_fw_printf("    CPU[%u] mpidr=0x%llx  state=%-8s"
                        "  on=%u off=%u susp=%u\n",
                        i, (unsigned long long)c->mpidr,
                        st[(uint8_t)c->state < 3 ? c->state : 3],
                        c->on_count, c->off_count, c->suspend_count);
    }
    uiox_fw_printf("    system_reset: %u\n", ctx->total_system_reset);
}
