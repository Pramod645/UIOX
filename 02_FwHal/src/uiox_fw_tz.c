/**
 * @file  uiox_fw_tz.c
 * @brief UIOX Firmware — ARM TrustZone setup implementation.
 * @date  2026-07-06
 */
#include "uiox_fw.h"
#include "uiox_fw_tz.h"
#include <string.h>

/* ── TZPC / TZASC base addresses (QEMU virt defaults) ──────── */
#define TZPC_BASE        0x08400000ULL  /* ARM TZPC BP141           */
#define TZASC_BASE       0x08800000ULL  /* ARM TZASC-400            */
#define TZASC_REGION_MAX 8u

/* ── GIC-400 base ───────────────────────────────────────────── */
#define GICD_BASE        0x08000000ULL
#define GICD_IGROUPR0    (GICD_BASE + 0x080u)

/* ── SCR_EL3 register access ────────────────────────────────── */
#if defined(__aarch64__)
uint64_t uiox_fw_read_scr_el3(void)
{
    uint64_t v;
    __asm__ volatile("mrs %0, SCR_EL3" : "=r"(v));
    return v;
}

void uiox_fw_write_scr_el3(uint64_t val)
{
    __asm__ volatile("msr SCR_EL3, %0\nisb" :: "r"(val) : "memory");
}

uint32_t uiox_fw_current_el(void)
{
    uint64_t v;
    __asm__ volatile("mrs %0, CurrentEL" : "=r"(v));
    return (uint32_t)((v >> 2u) & 3u);
}
#else
uint64_t uiox_fw_read_scr_el3(void)  { return 0u; }
void     uiox_fw_write_scr_el3(uint64_t v) { (void)v; }
uint32_t uiox_fw_current_el(void)
{
#if defined(__arm__)
    uint32_t cpsr;
    __asm__ volatile("mrs %0, cpsr" : "=r"(cpsr));
    return (cpsr & 0x1Fu) == 0x16u ? 3u : 1u; /* Monitor=EL3 equiv */
#else
    return 0u; /* x86 — no EL concept */
#endif
}
#endif

/* =========================================================================
 * Probe
 * ====================================================================== */
uiox_fw_err_t uiox_fw_tz_probe(uiox_fw_tz_ctx_t *ctx)
{
    if (!ctx) return UIOX_FW_ERR_INVAL;
    memset(ctx, 0, sizeof(*ctx));

    uint32_t el = uiox_fw_current_el();

#if defined(__aarch64__)
    ctx->el3_present = (el == 3u);
    if (ctx->el3_present) {
        ctx->scr_el3_val = (uint32_t)uiox_fw_read_scr_el3();
        /* Check NS bit: 0 = Secure, 1 = Non-Secure */
        bool ns = !!(ctx->scr_el3_val & UIOX_SCR_EL3_NS);
        strncpy(ctx->world, ns ? "NONSECURE" : "SECURE",
                sizeof(ctx->world) - 1u);
        ctx->tz_enabled = true;
    } else {
        strncpy(ctx->world, "NONSECURE", sizeof(ctx->world) - 1u);
        ctx->tz_enabled = false;
    }
#elif defined(__arm__)
    /* ARM32: TZ available if in Monitor mode (CPSR.M = 0x16) */
    ctx->el3_present = (el == 3u);
    ctx->tz_enabled  = ctx->el3_present;
    strncpy(ctx->world,
            ctx->el3_present ? "SECURE" : "NONSECURE",
            sizeof(ctx->world) - 1u);
#else
    ctx->el3_present = false;
    ctx->tz_enabled  = false;
    strncpy(ctx->world, "N/A (x86)", sizeof(ctx->world) - 1u);
#endif

    (void)el;
    return UIOX_FW_OK;
}

/* =========================================================================
 * Add memory region
 * ====================================================================== */
uiox_fw_err_t uiox_fw_tz_add_region(uiox_fw_tz_ctx_t  *ctx,
                                       uint64_t            base,
                                       uint64_t            size,
                                       uiox_tz_mem_attr_t  attr,
                                       const char         *name)
{
    if (!ctx) return UIOX_FW_ERR_INVAL;
    if (ctx->num_regions >= UIOX_TZ_MAX_REGIONS) return UIOX_FW_ERR_FULL;
    uiox_fw_tz_region_t *r = &ctx->regions[ctx->num_regions++];
    r->base = base;
    r->size = size;
    r->attr = attr;
    if (name) strncpy(r->name, name, sizeof(r->name) - 1u);
    return UIOX_FW_OK;
}

/* =========================================================================
 * Assign IRQ group
 * ====================================================================== */
uiox_fw_err_t uiox_fw_tz_assign_irq(uiox_fw_tz_ctx_t *ctx,
                                       uint32_t          irq,
                                       uiox_tz_irq_sec_t sec)
{
    if (!ctx || ctx->num_irqs >= UIOX_TZ_MAX_IRQS) return UIOX_FW_ERR_INVAL;
    ctx->irq_group[ctx->num_irqs++] = (irq & 0xFFFFu)
                                     | ((uint32_t)sec << 16u);
    return UIOX_FW_OK;
}

/* =========================================================================
 * Full TrustZone setup
 * ====================================================================== */
uiox_fw_err_t uiox_fw_tz_setup(uiox_fw_tz_ctx_t *ctx)
{
    if (!ctx) return UIOX_FW_ERR_INVAL;

    if (!ctx->el3_present) {
        uiox_fw_printf("  TZ: not at EL3 — skipping TrustZone setup\n");
        return UIOX_FW_OK;
    }

#if defined(__aarch64__)
    /* ── Step 1: Configure SCR_EL3 ─────────────────────────── */
    uint64_t scr = 0u;
    scr |= UIOX_SCR_EL3_RW;    /* EL1/EL2 are AArch64              */
    scr |= UIOX_SCR_EL3_HCE;   /* allow HVC instructions            */
    scr |= UIOX_SCR_EL3_ST;    /* secure timer access at EL1        */
    /* NS=0 for now — we are still in Secure World */
    uiox_fw_write_scr_el3(scr);
    ctx->scr_el3_val = (uint32_t)scr;
    uiox_fw_printf("  TZ: SCR_EL3 = 0x%08x\n", (uint32_t)scr);

    /* ── Step 2: Configure CPTR_EL3 (enable FP/SVE/SME) ────── */
    __asm__ volatile(
        "msr CPTR_EL3, xzr\n\t"
        "isb\n\t" ::: "memory");

    /* ── Step 3: Configure ACTLR_EL3 ──────────────────────── */
    __asm__ volatile("msr ACTLR_EL3, xzr\n\tisb\n\t" ::: "memory");

    /* ── Step 4: Apply TZASC memory regions ────────────────── */
    for (uint8_t i = 0; i < ctx->num_regions; i++) {
        const uiox_fw_tz_region_t *r = &ctx->regions[i];
        /* TZASC-400 region register layout (simplified):
           offset 0x100 + n*0x10: base low
           offset 0x104 + n*0x10: base high
           offset 0x108 + n*0x10: size + attr */
        volatile uint32_t *reg_base =
            (volatile uint32_t *)(TZASC_BASE + 0x100u + i * 0x10u);
        reg_base[0] = (uint32_t)(r->base & 0xFFFFFFFFu);
        reg_base[1] = (uint32_t)(r->base >> 32u);
        /* encode size as log2 - 1 */
        uint32_t sz_enc = 0u;
        for (uint32_t s = 1u; s < 32u; s++) {
            if ((r->size & ((uint64_t)1u << s)) && sz_enc == 0u)
                sz_enc = s - 1u;
        }
        reg_base[2] = (sz_enc << 1u) | (r->attr == UIOX_TZ_MEM_NONSECURE ? 0u : 1u) | 1u;
    }

    /* ── Step 5: GIC interrupt group assignment ─────────────── */
    for (uint8_t i = 0; i < ctx->num_irqs; i++) {
        uint32_t irq = ctx->irq_group[i] & 0xFFFFu;
        uint32_t grp = (ctx->irq_group[i] >> 16u) & 1u;
        uint32_t reg_off = GICD_IGROUPR0 + (irq / 32u) * 4u;
        volatile uint32_t *reg = (volatile uint32_t *)reg_off;
        if (grp == UIOX_TZ_IRQ_NONSECURE)
            *reg |=  (1u << (irq % 32u));
        else
            *reg &= ~(1u << (irq % 32u));
    }

    /* ── Step 6: Enable SMMU bypass in non-secure mode ──────── */
    /* Real: configure SMMU_STRTAB_BASE, SMMU_CR0 etc.
       Stub: just log intent. */
    uiox_fw_printf("  TZ: TZASC configured (%u regions)\n",
                   ctx->num_regions);

#elif defined(__arm__)
    /* ARM32 TrustZone (NSACR / SCR) */
    uint32_t scr;
    __asm__ volatile("mrc p15,0,%0,c1,c1,0" : "=r"(scr));
    scr |= (1u << 8);   /* SCD — disable SMC in NS world (optional) */
    scr &= ~(1u << 0);  /* keep NS=0 (Secure World) for now         */
    __asm__ volatile("mcr p15,0,%0,c1,c1,0" :: "r"(scr) : "memory");
    uiox_fw_printf("  TZ: SCR = 0x%08X (ARM32)\n", scr);
#else
    uiox_fw_printf("  TZ: x86 — no TrustZone hardware\n");
#endif

    uiox_fw_printf("  TZ: setup complete (world=%s)\n", ctx->world);
    return UIOX_FW_OK;
}

/* =========================================================================
 * Drop to EL1 (Non-Secure)
 * ====================================================================== */
#if defined(__aarch64__)
void __attribute__((noreturn))
uiox_fw_tz_drop_to_el1(uiox_fw_tz_ctx_t *ctx,
                          uint64_t          entry_pa,
                          uint64_t          arg0)
{
    (void)ctx;

    /* Set NS bit in SCR_EL3 — transition to Non-Secure EL1 */
    uint64_t scr = uiox_fw_read_scr_el3();
    scr |= UIOX_SCR_EL3_NS;   /* set NS=1 */
    scr |= UIOX_SCR_EL3_RW;   /* EL1 is AArch64 */
    uiox_fw_write_scr_el3(scr);

    /* Configure SPSR_EL3 for EL1h with DAIF masked */
    __asm__ volatile(
        "mov x0, %0\n\t"         /* arg0 = DTB phys addr       */
        "msr ELR_EL3, %1\n\t"   /* entry point                */
        "mov x2, #0x3C5\n\t"    /* SPSR: EL1h + DAIF masked   */
        "msr SPSR_EL3, x2\n\t"
        "isb\n\t"
        "eret\n\t"               /* jump to EL1                */
        :
        : "r"(arg0), "r"(entry_pa)
        : "x0", "x2", "memory"
    );
    __builtin_unreachable();
}
#else
void __attribute__((noreturn))
uiox_fw_tz_drop_to_el1(uiox_fw_tz_ctx_t *ctx,
                          uint64_t          entry_pa,
                          uint64_t          arg0)
{
    (void)ctx;
    /* ARM32 / x86: direct call — no EL switching needed */
    typedef void __attribute__((noreturn)) (*entry_fn_t)(uint64_t);
    ((entry_fn_t)(uintptr_t)entry_pa)(arg0);
    __builtin_unreachable();
}
#endif

void uiox_fw_tz_print(const uiox_fw_tz_ctx_t *ctx)
{
    if (!ctx) return;
    uiox_fw_printf("  TrustZone:\n");
    uiox_fw_printf("    EL3 present : %s\n",
                   ctx->el3_present ? "YES" : "NO");
    uiox_fw_printf("    TZ enabled  : %s\n",
                   ctx->tz_enabled  ? "YES" : "NO");
    uiox_fw_printf("    Current world: %s\n", ctx->world);
    uiox_fw_printf("    Regions     : %u\n", ctx->num_regions);
    for (uint8_t i = 0; i < ctx->num_regions; i++) {
        const uiox_fw_tz_region_t *r = &ctx->regions[i];
        uiox_fw_printf("    [%u] %-16s base=0x%llx size=0x%llx attr=%s\n",
                       i, r->name,
                       (unsigned long long)r->base,
                       (unsigned long long)r->size,
                       r->attr == UIOX_TZ_MEM_SECURE   ? "SECURE"
                     : r->attr == UIOX_TZ_MEM_NONSECURE? "NONSECURE"
                     : "INVALID");
    }
}
