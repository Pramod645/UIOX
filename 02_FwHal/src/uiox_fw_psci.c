/**
 * @file  uiox_fw_psci.c
 * @brief UIOX Firmware — PSCI v1.1 implementation.
 * @date  2026-07-06
 */
#include "uiox_fw.h"
#include "uiox_fw_psci.h"
#include <string.h>

/* ── Forward declaration of EL3 vector table install ────────── */
#if defined(__aarch64__)
extern void uiox_fw_el3_vector_install(void);   /* in entry stub */
#else
static inline void uiox_fw_el3_vector_install(void) {}
#endif

/* ── MPIDR helpers ───────────────────────────────────────────── */
static uint64_t read_mpidr(void)
{
#if defined(__aarch64__)
    uint64_t v; __asm__ volatile("mrs %0, MPIDR_EL1":"=r"(v)); return v;
#elif defined(__arm__)
    uint32_t v; __asm__ volatile("mrc p15,0,%0,c0,c0,5":"=r"(v)); return v;
#else
    return 0u;
#endif
}

static uint64_t mpidr_to_aff(uint64_t mpidr)
{
    /* Affinity = Aff2[23:16] | Aff1[15:8] | Aff0[7:0] */
    return mpidr & 0x00FFFFFFu;
}

/* ── Platform CPU power on ───────────────────────────────────── */
static uiox_fw_err_t plat_cpu_on(uint64_t mpidr, uint64_t entry_pa,
                                    uint64_t context_id)
{
    (void)context_id;
#if defined(__aarch64__)
    /* On QEMU virt: write to PSCI CPU_ON SMC.
       On real SoC: program the power-on controller and release reset. */
    /* Stub: set warm boot entry in a per-CPU scratch register */
    uiox_fw_printf("  PSCI: powering on CPU MPIDR=0x%llx entry=0x%llx\n",
                   (unsigned long long)mpidr,
                   (unsigned long long)entry_pa);
    /* Real impl: write entry_pa to a mailbox or RVBAR register,
       then trigger GIC SGI to wake the target CPU. */
    (void)mpidr;
    return UIOX_FW_OK;
#else
    (void)mpidr; (void)entry_pa;
    return UIOX_FW_OK;
#endif
}

/* ── Platform power off ──────────────────────────────────────── */
static void __attribute__((noreturn)) plat_system_off(void)
{
#if defined(__aarch64__)
    /* QEMU ACPI power-off via PSCI SYSTEM_OFF SMC to higher EL */
    uiox_fw_printf("  PSCI: SYSTEM_OFF — halting\n");
    __asm__ volatile("wfi\n\t");
    for (;;) __asm__ volatile("wfi");
#elif defined(__x86_64__)
    /* QEMU: port 0x604 ACPI S5 */
    __asm__ volatile("outw %0, %1"
                     :: "a"((uint16_t)0x2000),
                        "Nd"((uint16_t)0x604));
    for (;;) __asm__ volatile("hlt");
#else
    for (;;) __asm__ volatile("wfi");
#endif
}

static void __attribute__((noreturn))
plat_system_reset(uint32_t reset_type, uint64_t cookie)
{
    (void)reset_type; (void)cookie;
#if defined(__aarch64__)
    uiox_fw_printf("  PSCI: SYSTEM_RESET\n");
    __asm__ volatile("wfi"); for (;;);
#elif defined(__x86_64__)
    __asm__ volatile("outb %0, %1" :: "a"((uint8_t)0xFE), "Nd"((uint16_t)0x64));
    for (;;) __asm__ volatile("hlt");
#else
    for (;;) __asm__ volatile("wfi");
#endif
}

/* =========================================================================
 * Public API
 * ====================================================================== */
uiox_fw_err_t uiox_fw_psci_init(uiox_fw_psci_ctx_t *ctx)
{
    if (!ctx) return UIOX_FW_ERR_INVAL;
    memset(ctx, 0, sizeof(*ctx));

    /* Register the primary CPU */
    uint64_t my_mpidr = read_mpidr() & 0x00FFFFFFu;
    ctx->cpus[0].mpidr        = my_mpidr;
    ctx->cpus[0].state        = PSCI_AFF_STATE_ON;
    ctx->cpus[0].warm_boot_pa = 0u;
    ctx->cpus[0].present      = true;
    ctx->num_cpus             = 1u;

    /* On QEMU virt, assume 4 CPUs maximum */
#if defined(__aarch64__)
    for (uint32_t i = 1u; i < 4u && i < UIOX_PSCI_MAX_CPUS; i++) {
        ctx->cpus[i].mpidr   = (uint64_t)i;  /* Aff0 = CPU index */
        ctx->cpus[i].state   = PSCI_AFF_STATE_OFF;
        ctx->cpus[i].present = true;
        ctx->num_cpus++;
    }
#endif

    /* Install EL3 SMC vector — calls uiox_fw_psci_smc_handler */
    uiox_fw_el3_vector_install();

    ctx->initialised = true;
    uiox_fw_printf("  PSCI: v%u.%u initialised  CPUs=%u\n",
                   PSCI_VERSION_MAJOR, PSCI_VERSION_MINOR,
                   ctx->num_cpus);
    return UIOX_FW_OK;
}

/* ── SMC dispatcher ──────────────────────────────────────────── */
void uiox_fw_psci_smc_handler(uiox_fw_psci_ctx_t *ctx,
                                 uiox_fw_smc_frame_t *frame)
{
    if (!ctx || !frame) return;
    ctx->smc_count++;

    uint32_t fn_id = (uint32_t)frame->x[0];
    int64_t  ret   = PSCI_RET_NOT_SUPPORTED;

    switch (fn_id) {

    case PSCI_FN_VERSION:
        ret = (int64_t)PSCI_VERSION_VAL;
        break;

    case PSCI_FN_PSCI_FEATURES:
        /* Report which functions are supported */
        switch ((uint32_t)frame->x[1]) {
        case PSCI_FN_VERSION:
        case PSCI_FN_CPU_ON_32:
        case PSCI_FN_CPU_ON_64:
        case PSCI_FN_CPU_OFF:
        case PSCI_FN_AFFINITY_INFO_32:
        case PSCI_FN_AFFINITY_INFO_64:
        case PSCI_FN_SYSTEM_OFF:
        case PSCI_FN_SYSTEM_RESET:
        case PSCI_FN_PSCI_FEATURES:
            ret = PSCI_RET_SUCCESS;
            break;
        default:
            ret = PSCI_RET_NOT_SUPPORTED;
        }
        break;

    case PSCI_FN_CPU_ON_32:
    case PSCI_FN_CPU_ON_64: {
        uint64_t mpidr    = frame->x[1];
        uint64_t entry    = frame->x[2];
        uint64_t ctx_id   = frame->x[3];
        uiox_fw_err_t rc  = uiox_fw_psci_cpu_on(ctx, mpidr,
                                                   entry, ctx_id);
        ret = (rc == UIOX_FW_OK)
              ? PSCI_RET_SUCCESS : PSCI_RET_INTERNAL_FAILURE;
        break;
    }

    case PSCI_FN_CPU_OFF: {
        uiox_fw_err_t rc = uiox_fw_psci_cpu_off(ctx);
        ret = (rc == UIOX_FW_OK)
              ? PSCI_RET_SUCCESS : PSCI_RET_INTERNAL_FAILURE;
        break;
    }

    case PSCI_FN_AFFINITY_INFO_32:
    case PSCI_FN_AFFINITY_INFO_64: {
        uint64_t mpidr = frame->x[1];
        psci_affinity_state_t st =
            uiox_fw_psci_affinity_info(ctx, mpidr);
        ret = (int64_t)st;
        break;
    }

    case PSCI_FN_CPU_SUSPEND_32:
    case PSCI_FN_CPU_SUSPEND_64: {
        uint32_t pwr_st = (uint32_t)frame->x[1];
        uint64_t entry  = frame->x[2];
        uint64_t ctx_id = frame->x[3];
        uiox_fw_err_t rc = uiox_fw_psci_suspend(ctx, pwr_st,
                                                   entry, ctx_id);
        ret = (rc == UIOX_FW_OK)
              ? PSCI_RET_SUCCESS : PSCI_RET_INTERNAL_FAILURE;
        break;
    }

    case PSCI_FN_SYSTEM_OFF:
        uiox_fw_psci_system_off(ctx);
        /* never returns */

    case PSCI_FN_SYSTEM_RESET:
        uiox_fw_psci_system_reset(ctx, 0u, 0u);
        /* never returns */

    case PSCI_FN_SYSTEM_RESET2_32:
    case PSCI_FN_SYSTEM_RESET2_64:
        uiox_fw_psci_system_reset(ctx,
                                    (uint32_t)frame->x[1],
                                    frame->x[2]);
        /* never returns */

    case PSCI_FN_MEM_PROTECT:
        /* Return 0: memory protection disabled
        ret = 0; /* disabled */
        break;

    default:
        ret = PSCI_RET_NOT_SUPPORTED;
        break;
    }

    frame->x[0] = (uint64_t)(int64_t)ret;
}

/* ── Individual PSCI handlers ────────────────────────────── */

uiox_fw_err_t uiox_fw_psci_cpu_on(uiox_fw_psci_ctx_t *ctx,
                                     uint64_t mpidr,
                                     uint64_t entry_point_pa,
                                     uint64_t context_id)
{
    if (!ctx || !ctx->initialised) return UIOX_FW_ERR_INVAL;

    /* Find this CPU in our table */
    uint64_t aff = mpidr_to_aff(mpidr);
    for (uint32_t i = 0; i < ctx->num_cpus; i++) {
        if (mpidr_to_aff(ctx->cpus[i].mpidr) != aff) continue;
        if (!ctx->cpus[i].present)     return UIOX_FW_ERR_INVAL;
        if (ctx->cpus[i].state == PSCI_AFF_STATE_ON)
            return UIOX_FW_ERR_POST; /* already on */

        ctx->cpus[i].warm_boot_pa = entry_point_pa;
        ctx->cpus[i].context_id   = context_id;
        ctx->cpus[i].state        = PSCI_AFF_STATE_ON_PEND;

        uiox_fw_err_t rc = plat_cpu_on(mpidr, entry_point_pa,
                                          context_id);
        if (rc == UIOX_FW_OK) {
            ctx->cpus[i].state = PSCI_AFF_STATE_ON;
            ctx->cpu_on_count++;
        }
        return rc;
    }
    return UIOX_FW_ERR_INVAL;
}

uiox_fw_err_t uiox_fw_psci_cpu_off(uiox_fw_psci_ctx_t *ctx)
{
    if (!ctx || !ctx->initialised) return UIOX_FW_ERR_INVAL;
    uint64_t my_mpidr = mpidr_to_aff(read_mpidr());

    for (uint32_t i = 0; i < ctx->num_cpus; i++) {
        if (mpidr_to_aff(ctx->cpus[i].mpidr) != my_mpidr) continue;
        if (i == 0u) return UIOX_FW_ERR_INVAL; /* cannot off primary */
        ctx->cpus[i].state = PSCI_AFF_STATE_OFF;
        ctx->cpu_off_count++;
        break;
    }
    /* Spin — real impl powers down the CPU core */
    for (;;) {
#if defined(__aarch64__) || defined(__arm__)
        __asm__ volatile("wfi");
#else
        __asm__ volatile("hlt");
#endif
    }
}

uiox_fw_err_t uiox_fw_psci_suspend(uiox_fw_psci_ctx_t *ctx,
                                      uint32_t  power_state,
                                      uint64_t  entry_pa,
                                      uint64_t  context_id)
{
    if (!ctx) return UIOX_FW_ERR_INVAL;
    (void)power_state; (void)entry_pa; (void)context_id;

    ctx->suspend_count++;
    /* Simple WFI suspend — no clock/power gating in simulation */
#if defined(__aarch64__) || defined(__arm__)
    __asm__ volatile("wfi");
#else
    __asm__ volatile("hlt");
#endif
    return UIOX_FW_OK;
}

psci_affinity_state_t
uiox_fw_psci_affinity_info(const uiox_fw_psci_ctx_t *ctx,
                              uint64_t mpidr)
{
    if (!ctx) return PSCI_AFF_STATE_OFF;
    uint64_t aff = mpidr_to_aff(mpidr);
    for (uint32_t i = 0; i < ctx->num_cpus; i++) {
        if (mpidr_to_aff(ctx->cpus[i].mpidr) == aff)
            return ctx->cpus[i].state;
    }
    return PSCI_AFF_STATE_OFF;
}

void __attribute__((noreturn))
uiox_fw_psci_system_off(uiox_fw_psci_ctx_t *ctx)
{
    if (ctx) ctx->off_count++;
    plat_system_off();
}

void __attribute__((noreturn))
uiox_fw_psci_system_reset(uiox_fw_psci_ctx_t *ctx,
                             uint32_t reset_type, uint64_t cookie)
{
    if (ctx) ctx->reset_count++;
    plat_system_reset(reset_type, cookie);
}

uiox_fw_err_t uiox_fw_psci_dt_register(uiox_fw_psci_ctx_t *ctx,
                                           uint64_t dtb_pa)
{
    /* Stub: a real implementation would walk the DTB and add
       /psci { method = "smc"; } or "hvc" node entries. */
    (void)ctx; (void)dtb_pa;
    uiox_fw_printf("  PSCI: DT node registration skipped (stub)\n");
    return UIOX_FW_OK;
}

void uiox_fw_psci_print(const uiox_fw_psci_ctx_t *ctx)
{
    if (!ctx) return;
    static const char *st[] = { "ON", "OFF", "ON_PEND" };
    uiox_fw_printf("  PSCI v%u.%u:\n",
                   PSCI_VERSION_MAJOR, PSCI_VERSION_MINOR);
    uiox_fw_printf("    Initialised : %s\n",
                   ctx->initialised ? "YES" : "NO");
    uiox_fw_printf("    CPUs        : %u\n", ctx->num_cpus);
    for (uint32_t i = 0; i < ctx->num_cpus; i++) {
        const uiox_psci_cpu_t *c = &ctx->cpus[i];
        uiox_fw_printf("    CPU[%u] MPIDR=0x%08llx  state=%-8s\n",
                       i,
                       (unsigned long long)c->mpidr,
                       c->state < 3u ? st[c->state] : "?");
    }
    uiox_fw_printf("    SMC calls   : %u\n", ctx->smc_count);
    uiox_fw_printf("    CPU_ON      : %u\n", ctx->cpu_on_count);
    uiox_fw_printf("    CPU_OFF     : %u\n", ctx->cpu_off_count);
    uiox_fw_printf("    SUSPEND     : %u\n", ctx->suspend_count);
    uiox_fw_printf("    SYS_RESET   : %u\n", ctx->reset_count);
}
