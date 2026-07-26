/**
 * @file  uiox_sec.c
 * @brief UIOX Security — master init, combined ASLR + MAC lifecycle.
 *
 * Integrates:
 *   33_ProcessControlSubsystem/02_memory-management  — ASLR applied on exec()
 *   33_ProcessControlSubsystem/40_procStruct         — per-process sec ctx
 *   32_FileSystem/VirtualFileSystem.h                — MAC on open/exec/unlink
 *   50_UIX/12_ksign                                  — policy hash verified
 *   40_SystemCallInterface                           — syscall dispatch
 *
 * @date  2026-07-08
 */
#include "../include/uiox_sec.h"

extern void uiox_fw_printf(const char *fmt, ...);

static void sec_memset(void *d, int v, size_t n)
{ uint8_t *p = (uint8_t *)d; while (n--) *p++ = (uint8_t)v; }

/* =========================================================================
 * Default kernel policy — minimal allow rules installed at boot before
 * the policy binary is loaded from the filesystem.
 * ====================================================================== */
static void install_default_policy(uiox_mac_policy_ctx_t *ctx)
{
    /* Register minimum required types */
    uint32_t kernel_t, init_t, unlab_t;
    uiox_mac_policy_add_type(ctx, "kernel_t",      &kernel_t);
    uiox_mac_policy_add_type(ctx, "init_t",         &init_t);
    uiox_mac_policy_add_type(ctx, "unlabelled_t",   &unlab_t);

    /* kernel → everything: full access (boot phase only) */
    uiox_mac_rule_t r;
    sec_memset(&r, 0, sizeof(r));
    r.subject_type = kernel_t;
    r.object_type  = kernel_t;
    r.obj_class    = UIOX_MAC_CLASS_KERNEL;
    r.allow        = UIOX_MAC_PERM_ALL;
    uiox_mac_policy_add_rule(ctx, &r);

    /* init_t → files: read + exec */
    r.subject_type = init_t;
    r.object_type  = unlab_t;
    r.obj_class    = UIOX_MAC_CLASS_FILE;
    r.allow        = UIOX_MAC_PERM_READ | UIOX_MAC_PERM_EXEC;
    uiox_mac_policy_add_rule(ctx, &r);

    /* init_t → dirs: read */
    r.obj_class    = UIOX_MAC_CLASS_DIR;
    r.allow        = UIOX_MAC_PERM_READ;
    uiox_mac_policy_add_rule(ctx, &r);

    uiox_fw_printf("[sec] Default policy installed: "
                   "types=%u  rules=%u\n",
                   ctx->type_count, ctx->rule_count);
}

/* =========================================================================
 * Master init
 * ====================================================================== */
uiox_sec_err_t uiox_sec_init(uiox_sec_ctx_t *ctx,
                               uint8_t         aslr_level,
                               uiox_mac_mode_t mac_mode)
{
    if (!ctx) return UIOX_SEC_ERR_INVAL;

    sec_memset(ctx, 0, sizeof(*ctx));

    /* ASLR */
    uiox_sec_err_t rc = uiox_aslr_init(&ctx->aslr, aslr_level);
    if (rc != UIOX_SEC_OK) {
        uiox_fw_printf("[sec] ASLR init failed: %s\n",
                       uiox_sec_err_str(rc));
        return rc;
    }

    /* MAC */
    rc = uiox_mac_policy_init(&ctx->mac, mac_mode);
    if (rc != UIOX_SEC_OK) {
        uiox_fw_printf("[sec] MAC policy init failed: %s\n",
                       uiox_sec_err_str(rc));
        return rc;
    }

    /* Install minimal boot-time policy */
    install_default_policy(&ctx->mac);

    /* Register global context for syscall handlers */
    uiox_sec_set_global_ctx(ctx);

    ctx->initialized = true;
    uiox_fw_printf("[sec] Security subsystem initialised: "
                   "ASLR level=%u  MAC mode=%s\n",
                   aslr_level, uiox_mac_mode_str(mac_mode));
    return UIOX_SEC_OK;
}

/* =========================================================================
 * Print summary
 * ====================================================================== */
void uiox_sec_print(const uiox_sec_ctx_t *ctx)
{
    if (!ctx) return;
    uiox_fw_printf("\n[sec] ══ Security Subsystem Status ══════════\n");

    /* ASLR */
    uiox_fw_printf("[sec] ASLR:\n");
    uiox_aslr_print(&ctx->aslr, NULL);

    /* MAC */
    uiox_fw_printf("[sec] MAC:\n");
    uiox_mac_policy_print(&ctx->mac);

    uiox_fw_printf("[sec] ════════════════════════════════════════\n\n");
}
