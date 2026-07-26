/**
 * @file  uiox_ksign.c
 * @brief UIOX Signed Kernel — top-level orchestration.
 *
 * Serves two roles depending on compile-time flag UIOX_KSIGN_TOOL:
 *
 *   UIOX_KSIGN_TOOL=1  → Host-side build tool (signs a kernel ELF)
 *   UIOX_KSIGN_TOOL=0  → Boot-time entry point called from Stage 0d
 *                         (verifies, measures, and hands off to kernel_main)
 *
 * Boot-time call sequence:
 *   1. uiox_ks_boot_init()        — initialise all sub-contexts
 *   2. uiox_ks_boot_verify()      — full signature + chain verification
 *   3. uiox_ks_boot_measure()     — extend PCRs with verified image
 *   4. uiox_ks_boot_arm_runtime() — register regions for periodic checks
 *   5. uiox_ks_boot_handoff()     — jump to kernel entry point
 *
 * Integrates with:
 *   02_FwHal/uiox_fw_secboot.c — Stage 0d boot path
 *   33_ProcessControlSubsystem — scheduler calls uiox_ks_rt_tick()
 *   40_SystemCallInterface     — syscall dispatch table
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
#include "../include/uiox_ksign_verify.h"
#include "../include/uiox_ksign_runtime.h"
#include "../include/uiox_ksign_measure.h"
#include "../include/uiox_ksign_key.h"
#include "../include/uiox_ksign_image.h"

extern void uiox_fw_printf(const char *fmt, ...);

/* =========================================================================
 * Global sub-contexts (static storage — no heap dependency)
 * ====================================================================== */
static uiox_ks_verify_ctx_t   g_verify_ctx;
static uiox_ks_rt_ctx_t       g_rt_ctx;
static uiox_ks_measure_ctx_t  g_measure_ctx;
static uiox_ks_keystore_t     g_keystore;
static uiox_ks_verify_report_t g_last_report;

/* Publicly accessible runtime context pointer (used by scheduler tick) */
uiox_ks_rt_ctx_t *uiox_ks_global_rt_ctx = &g_rt_ctx;

/* =========================================================================
 * Weak platform hooks — override in BSP / board-support layer
 * ====================================================================== */

/**
 * @brief Read the Root-of-Trust key ID from OTP fuses / ROM.
 *        Default: zero-filled (test/development only).
 *        Production: replace with real OTP read.
 */
__attribute__((weak))
void uiox_ks_plat_read_rot_key_id(uint8_t out[UIOX_KS_KEY_ID_LEN])
{
    for (uint32_t i = 0; i < UIOX_KS_KEY_ID_LEN; i++) out[i] = 0;
}

/**
 * @brief Read the minimum kernel version from OTP / NVRAM (anti-rollback).
 *        Default: 0 (allow all).
 */
__attribute__((weak))
uint32_t uiox_ks_plat_read_min_version(void) { return 0u; }

/**
 * @brief Return current monotonic time in milliseconds.
 *        Default: returns 0 (no timer available before kernel runs).
 */
__attribute__((weak))
uint64_t uiox_ks_plat_get_time_ms(void) { return 0u; }

/**
 * @brief Platform halt — called on fatal verification failure.
 *        Must never return.
 */
__attribute__((weak))
void uiox_ks_plat_halt(void)
{
    uiox_fw_printf("[ksign] FATAL: halting.\n");
    for (;;) { /* spin */ }
}

/* =========================================================================
 * Step 1 — initialise all sub-contexts
 * ====================================================================== */
uiox_ks_err_t uiox_ks_boot_init(void)
{
    uiox_ks_err_t rc;

    /* Measurement log */
    rc = uiox_ks_measure_init(&g_measure_ctx, uiox_ks_plat_get_time_ms);
    if (rc != UIOX_KS_OK) {
        uiox_fw_printf("[ksign] measure_init failed: %d\n", rc);
        return rc;
    }

    /* Key store: seed Root-of-Trust from OTP */
    uint8_t rot_id[UIOX_KS_KEY_ID_LEN];
    uiox_ks_plat_read_rot_key_id(rot_id);
    rc = uiox_ks_keystore_init(&g_keystore, rot_id);
    if (rc != UIOX_KS_OK) {
        uiox_fw_printf("[ksign] keystore_init failed: %d\n", rc);
        return rc;
    }

    /* Verify context */
    rc = uiox_ks_verify_init(&g_verify_ctx,
                              &g_keystore,
                              &g_measure_ctx,
                              uiox_ks_plat_read_min_version());
    if (rc != UIOX_KS_OK) {
        uiox_fw_printf("[ksign] verify_init failed: %d\n", rc);
        return rc;
    }

    /* Runtime monitor */
    rc = uiox_ks_rt_init(&g_rt_ctx, &g_measure_ctx, uiox_ks_plat_get_time_ms);
    if (rc != UIOX_KS_OK) {
        uiox_fw_printf("[ksign] rt_init failed: %d\n", rc);
        return rc;
    }

    uiox_fw_printf("[ksign] Boot init complete.\n");
    return UIOX_KS_OK;
}

/* =========================================================================
 * Step 2 — full signature + chain verification of the kernel image
 * ====================================================================== */
uiox_ks_err_t uiox_ks_boot_verify(const void *image, size_t image_size)
{
    if (!image || image_size == 0u) return UIOX_KS_ERR_INVAL;

    uiox_ks_err_t rc = uiox_ks_verify_image(&g_verify_ctx,
                                              image, image_size,
                                              &g_last_report);
    uiox_ks_verify_print(&g_last_report);

    if (rc != UIOX_KS_OK) {
        uiox_fw_printf("[ksign] Verification FAILED: %s\n",
                       uiox_ks_err_str(rc));
        return rc;
    }

    uiox_fw_printf("[ksign] Kernel image verified OK "
                   "(version=%u, sigs=%u).\n",
                   g_last_report.kernel_version,
                   g_last_report.sigs_verified);
    return UIOX_KS_OK;
}

/* =========================================================================
 * Step 3 — extend PCRs with verified image metadata
 * ====================================================================== */
uiox_ks_err_t uiox_ks_boot_measure(const void *image, size_t image_size)
{
    if (!image || image_size == 0u) return UIOX_KS_ERR_INVAL;

    const uiox_ks_img_hdr_t *hdr = (const uiox_ks_img_hdr_t *)image;

    /* PCR[1]: kernel code (payload hash from verified header) */
    uiox_ks_measure_extend_hash(&g_measure_ctx, 1u,
                                 hdr->payload_hash,
                                 "kernel-payload-sha256",
                                 UIOX_KS_EVT_KERNEL_CODE);

    /* PCR[2]: kernel rodata — extend with SHA-384 hash (truncated to 32 B) */
    uint8_t h384_trunc[UIOX_KS_SHA256_LEN];
    for (uint32_t i = 0; i < UIOX_KS_SHA256_LEN; i++)
        h384_trunc[i] = hdr->payload_hash384[i];
    uiox_ks_measure_extend_hash(&g_measure_ctx, 2u,
                                 h384_trunc,
                                 "kernel-payload-sha384",
                                 UIOX_KS_EVT_KERNEL_DATA);

    /* PCR[5]: signing key ID */
    uiox_ks_measure_extend_hash(&g_measure_ctx, 5u,
                                 hdr->signing_key_id,
                                 "kernel-signing-key",
                                 UIOX_KS_EVT_KEY_LOAD);

    uiox_fw_printf("[ksign] PCR measurement complete (%u entries).\n",
                   g_measure_ctx.entry_count);
    return UIOX_KS_OK;
}

/* =========================================================================
 * Step 4 — arm runtime monitor with boot-time hashes
 * ====================================================================== */
uiox_ks_err_t uiox_ks_boot_arm_runtime(uintptr_t text_base,
                                          size_t    text_size,
                                          uintptr_t rodata_base,
                                          size_t    rodata_size)
{
    const uiox_ks_img_hdr_t *hdr =
        (const uiox_ks_img_hdr_t *)(uintptr_t)g_verify_ctx.image_base;
    if (!hdr) return UIOX_KS_ERR_INVAL;

    uiox_ks_err_t rc = uiox_ks_rt_seed_from_image(&g_rt_ctx, hdr,
                                                    text_base,  text_size,
                                                    rodata_base, rodata_size);
    if (rc != UIOX_KS_OK) {
        uiox_fw_printf("[ksign] rt_seed failed: %d\n", rc);
        return rc;
    }

    uiox_fw_printf("[ksign] Runtime monitor armed: "
                   ".text=0x%lx(%zu B)  .rodata=0x%lx(%zu B)\n",
                   (unsigned long)text_base,  text_size,
                   (unsigned long)rodata_base, rodata_size);
    return UIOX_KS_OK;
}

/* =========================================================================
 * Step 5 — lock PCRs and hand off to kernel entry point
 * ====================================================================== */
typedef void (*uiox_kernel_entry_t)(void);

uiox_ks_err_t uiox_ks_boot_handoff(const void *image)
{
    if (!image) return UIOX_KS_ERR_INVAL;

    const uiox_ks_img_hdr_t *hdr = (const uiox_ks_img_hdr_t *)image;

    /* Seal PCRs — no more extends after this */
    uiox_ks_measure_lock(&g_measure_ctx);

    uiox_fw_printf("[ksign] Handing off to kernel entry 0x%016llx...\n",
                   (unsigned long long)hdr->entry_addr);

    /* Jump — this must not return on success */
    uiox_kernel_entry_t entry =
        (uiox_kernel_entry_t)(uintptr_t)hdr->entry_addr;
    entry();

    /* Should be unreachable */
    return UIOX_KS_ERR_INVAL;
}

/* =========================================================================
 * Master boot entry — called from uiox_fw_secboot Stage 0d
 * ====================================================================== */
void uiox_ks_boot_entry(const void *image,
                          size_t      image_size,
                          uintptr_t   text_base,
                          size_t      text_size,
                          uintptr_t   rodata_base,
                          size_t      rodata_size)
{
    uiox_fw_printf("[ksign] === UIOX Kernel Signing Boot ===\n");

    uiox_ks_err_t rc;

#define KS_CHECK(step)                           \
    do {                                         \
        rc = (step);                             \
        if (rc != UIOX_KS_OK) {                 \
            uiox_fw_printf("[ksign] FATAL at "  \
                #step " => %d\n", rc);           \
            uiox_ks_plat_halt();                 \
        }                                        \
    } while (0)

    KS_CHECK(uiox_ks_boot_init());
    KS_CHECK(uiox_ks_boot_verify(image, image_size));
    KS_CHECK(uiox_ks_boot_measure(image, image_size));
    KS_CHECK(uiox_ks_boot_arm_runtime(text_base, text_size,
                                       rodata_base, rodata_size));

#undef KS_CHECK

    /* Handoff — does not return on success */
    (void)uiox_ks_boot_handoff(image);

    /* If we reach here, entry point returned — fatal */
    uiox_fw_printf("[ksign] FATAL: kernel entry returned unexpectedly.\n");
    uiox_ks_plat_halt();
}

/* =========================================================================
 * Scheduler integration — call from 33_ProcessControlSubsystem tick
 * ====================================================================== */
void uiox_ks_scheduler_tick(void)
{
    uiox_ks_rt_state_t state = uiox_ks_rt_tick(&g_rt_ctx);
    if (state == UIOX_KS_RT_STATE_TAMPERED) {
        uiox_fw_printf("[ksign] CRITICAL: runtime integrity violation "
                       "detected on scheduler tick!\n");
        /* Policy decision: panic, log-only, or notify audit subsystem.
         * Default: log only. Override uiox_ks_plat_halt() for strict mode. */
    }
}

/* =========================================================================
 * Attestation helper — used by sys_ksign_quote
 * ====================================================================== */
void uiox_ks_get_attestation_quote(uint8_t quote[UIOX_KS_SHA256_LEN])
{
    uiox_ks_measure_quote(&g_measure_ctx, quote);
}
