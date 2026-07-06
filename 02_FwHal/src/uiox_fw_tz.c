/**
 * @file    uiox_fw_tz.c
 * @brief   UIOX Firmware — TrustZone / EL3 setup implementation.
 * @date    2026-07-06
 */
#include "uiox_fw.h"
#include <string.h>

bool uiox_fw_tz_supported(void)
{
#if defined(__aarch64__)
    /* AA64PFR0_EL1 [35:32] = EL3 field: 0001 = EL3 exists */
    uint64_t pfr0;
    __asm__ volatile("mrs %0, ID_AA64PFR0_EL1" : "=r"(pfr0));
    return ((pfr0 >> 32u) & 0xFu) != 0u;
#else
    return false;
#endif
}

uiox_fw_err_t uiox_fw_tz_init(uiox_fw_tz_ctx_t *ctx)
{
    if (!ctx) return UIOX_FW_ERR_INVAL;
    memset(ctx, 0, sizeof(*ctx));
    ctx->tz_supported = uiox_fw_tz_supported();

    if (!ctx->tz_supported) {
        uiox_fw_printf("  [tz] TrustZone not available on this platform\n");
        return UIOX_FW_OK;   /* not an error — graceful degradation */
    }

#if defined(__aarch64__)
    /* Check current EL */
    uint64_t el;
    __asm__ volatile("mrs %0, CurrentEL" : "=r"(el));
    el = (el >> 2u) & 3u;
    ctx->el3_active = (el == 3u);
    uiox_fw_printf("  [tz] current EL = %llu  TrustZone supported\n",
                    (unsigned long long)el);
#endif
    return UIOX_FW_OK;
}

uiox_fw_err_t uiox_fw_tz_configure_scr(uiox_fw_tz_ctx_t *ctx,
                                          uint64_t scr_bits)
{
    if (!ctx || !ctx->tz_supported) return UIOX_FW_ERR_NOTSUP;
#if defined(__aarch64__)
    if (!ctx->el3_active) return UIOX_FW_ERR_NOTSUP;

    /* Read current SCR_EL3 and merge */
    uint64_t scr;
    __asm__ volatile("mrs %0, SCR_EL3" : "=r"(scr));
    scr |= scr_bits;

    /* Always set RW=1 (EL1/EL2 runs AArch64) and NS=1 (non-secure) */
    scr |= SCR_EL3_RW | SCR_EL3_NS;
    /* Route FIQ to EL3 for secure interrupts */
    scr |= SCR_EL3_FIQ;

    __asm__ volatile("msr SCR_EL3, %0\nisb" :: "r"(scr) : "memory");
    ctx->scr_el3_val    = scr;
    ctx->ns_configured  = true;

    uiox_fw_printf("  [tz] SCR_EL3 = 0x%016llx\n",
                    (unsigned long long)scr);
    return UIOX_FW_OK;
#else
    (void)scr_bits;
    return UIOX_FW_ERR_NOTSUP;
#endif
}

uiox_fw_err_t uiox_fw_tz_add_region(uiox_fw_tz_ctx_t *ctx,
                                       uintptr_t base, uint64_t size,
                                       uiox_tz_mem_type_t type,
                                       const char *name)
{
    if (!ctx || !ctx->tz_supported) return UIOX_FW_ERR_NOTSUP;
    if (ctx->num_regions >= UIOX_TZ_MAX_REGIONS) return UIOX_FW_ERR_INVAL;

    uiox_tz_region_t *r = &ctx->regions[ctx->num_regions++];
    r->base = base;
    r->size = size;
    r->type = type;
    if (name) {
        uint32_t i = 0;
        while(name[i] && i < sizeof(r->name)-1u) { r->name[i]=name[i]; i++; }
    }
    return UIOX_FW_OK;
}

uiox_fw_err_t uiox_fw_tz_apply(uiox_fw_tz_ctx_t *ctx)
{
    if (!ctx || !ctx->tz_supported) return UIOX_FW_ERR_NOTSUP;

    for (uint8_t i = 0; i < ctx->num_regions; i++) {
        const uiox_tz_region_t *r = &ctx->regions[i];
        const char *tname =
            r->type == UIOX_TZ_MEM_SECURE    ? "SECURE" :
            r->type == UIOX_TZ_MEM_NONSECURE ? "NS" : "SHARED";
        uiox_fw_printf("  [tz] region[%u] %-8s  base=0x%llx  size=0x%llx  %s\n",
                        i, r->name,
                        (unsigned long long)r->base,
                        (unsigned long long)r->size,
                        tname);
        /*
         * Real implementation: write to TZASC / TZPC registers.
         * On the QEMU virt machine these controllers are not present
         * so we log only.
         */
    }
    return UIOX_FW_OK;
}

uiox_fw_err_t uiox_fw_tz_configure_gic(uiox_fw_tz_ctx_t *ctx,
                                          uintptr_t gicd_base,
                                          uintptr_t gicc_base)
{
    if (!ctx || !ctx->tz_supported || !gicd_base) return UIOX_FW_ERR_NOTSUP;
    (void)gicc_base;

    /* GICD_IGROUPR0 = 0x00000000 → all IRQs in Group 0 (Secure / FIQ)
     * Real production split: SGI/PPI in Group 0, SPI in Group 1       */
    uiox_fw_hw_mmio_w32((uint32_t)gicd_base + 0x080u, 0x00000000u);
    /* Enable distributor secure (bit 1) + non-secure (bit 2) enable   */
    uiox_fw_hw_mmio_w32((uint32_t)gicd_base + 0x000u, 0x3u);

    ctx->gic_configured = true;
    uiox_fw_printf("  [tz] GIC security configured\n");
    return UIOX_FW_OK;
}

uiox_fw_err_t uiox_fw_tz_install_vbar(uiox_fw_tz_ctx_t *ctx,
                                         uintptr_t vbar_addr)
{
    if (!ctx || !ctx->el3_active) return UIOX_FW_ERR_NOTSUP;
#if defined(__aarch64__)
    __asm__ volatile("msr VBAR_EL3, %0\nisb" :: "r"(vbar_addr) : "memory");
    ctx->vbar_el3 = vbar_addr;
    uiox_fw_printf("  [tz] VBAR_EL3 = 0x%llx\n",
                    (unsigned long long)vbar_addr);
    return UIOX_FW_OK;
#else
    (void)vbar_addr;
    return UIOX_FW_ERR_NOTSUP;
#endif
}

uiox_fw_err_t uiox_fw_tz_register_monitor(uiox_fw_tz_ctx_t *ctx,
                                             uintptr_t entry)
{
    if (!ctx) return UIOX_FW_ERR_INVAL;
    ctx->optee_entry = entry;
    uiox_fw_printf("  [tz] Secure monitor entry = 0x%llx\n",
                    (unsigned long long)entry);
    return UIOX_FW_OK;
}

void __attribute__((noreturn))
uiox_fw_tz_drop_to_el1(uiox_fw_tz_ctx_t *ctx,
                         uintptr_t el1_entry,
                         uint64_t  dtb_pa)
{
    (void)ctx;
#if defined(__aarch64__)
    /* SPSR_EL3: EL1h (0x05), DAIF masked (D+A+I+F = bits [9:6]) */
    uint64_t spsr = 0x3C5ULL;  /* EL1h + DAIF all masked */
    __asm__ volatile(
        "msr SPSR_EL3, %0\n\t"
        "msr ELR_EL3,  %1\n\t"
        "mov x0, %2\n\t"       /* x0 = DTB physical address */
        "mov x1, xzr\n\t"
        "mov x2, xzr\n\t"
        "mov x3, xzr\n\t"
        "eret\n\t"
        :
        : "r"(spsr), "r"((uint64_t)el1_entry), "r"(dtb_pa)
        : "x0","x1","x2","x3","memory"
    );
#else
    (void)el1_entry; (void)dtb_pa;
#endif
    for (;;) uiox_fw_hw_wfi();
}

void uiox_fw_tz_print(const uiox_fw_tz_ctx_t *ctx)
{
    if (!ctx) return;
    uiox_fw_printf("  TrustZone:\n");
    uiox_fw_printf("    Supported  : %s\n", ctx->tz_supported ? "YES":"NO");
    uiox_fw_printf("    EL3 active : %s\n", ctx->el3_active   ? "YES":"NO");
    uiox_fw_printf("    NS set     : %s\n", ctx->ns_configured ? "YES":"NO");
    uiox_fw_printf("    GIC secure : %s\n", ctx->gic_configured? "YES":"NO");
    uiox_fw_printf("    SCR_EL3    : 0x%016llx\n",
                    (unsigned long long)ctx->scr_el3_val);
    uiox_fw_printf("    VBAR_EL3   : 0x%llx\n",
                    (unsigned long long)ctx->vbar_el3);
    uiox_fw_printf("    Monitor    : 0x%llx\n",
                    (unsigned long long)ctx->optee_entry);
    uiox_fw_printf("    Regions    : %u\n", ctx->num_regions);
}
