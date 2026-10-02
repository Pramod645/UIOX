/**
 * @file  uiox_kix_ksign.c
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
 *   4. uiox_ks_boot_arm_runtime() — seed the monitor from the verified payload
 *   5. uiox_ks_boot_handoff()     — jump to kernel entry point
 *
 * ── what changed in 1.1.0 ─────────────────────────────────────────────
 *   verify_init is called with FOUR arguments — the measurement context
 *   in its own parameter, and no sim_mode.  The previous call passed
 *   &g_measure_ctx in the min_ver slot and omitted sim_mode entirely, so
 *   the argument that decides anti-rollback was a pointer.
 *
 *   boot_measure extends PCR[6] KEY_LOAD for the signing key, not PCR[5].
 *   PCR[5] is now RUNTIME_HASH — the baseline the runtime monitor
 *   compares against.
 *
 *   boot_arm_runtime takes the IMAGE, not two section extents.  Under
 *   option A the monitored region is the payload — what the signature
 *   covered — so its base and size come from the header.
 *
 *   The three context accessors the umbrella header advertises are
 *   defined at the end.
 *
 * @version 1.1.0
 * @date    2026-10-01
 */
#include "../include/uiox_kix_ksign.h"

extern void uiox_fw_printf(const char *fmt, ...);

/* =========================================================================
 * Global sub-contexts (static storage — no heap dependency)
 * ====================================================================== */
static uiox_ks_verify_ctx_t    g_verify_ctx;
static uiox_ks_rt_ctx_t        g_rt_ctx;
static uiox_ks_measure_ctx_t   g_measure_ctx;
static uiox_ks_keystore_t      g_keystore;
static uiox_ks_verify_report_t g_last_report;

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
 *        Default: returns 0.
 *
 * NOTE: 0 means NO CLOCK, not time zero.  With the default, the key
 * expiry window is not evaluated — and the verification report says so
 * ("NOT CHECKED (no clock)") rather than letting a zero read as a pass.
 * A real board overrides this so the window is checked.
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
    uint8_t       rot_id[UIOX_KS_KEY_ID_LEN];
    uiox_ks_err_t rc;

    /* Measurement log */
    rc = uiox_ks_measure_init(&g_measure_ctx, uiox_ks_plat_get_time_ms);
    if (rc != UIOX_KS_OK) {
        uiox_fw_printf("[ksign] measure_init failed: %d\n", rc);
        return rc;
    }

    /* Key store: seed Root-of-Trust from OTP */
    uiox_ks_plat_read_rot_key_id(rot_id);
    rc = uiox_ks_keystore_init(&g_keystore, rot_id);
    if (rc != UIOX_KS_OK) {
        uiox_fw_printf("[ksign] keystore_init failed: %d\n", rc);
        return rc;
    }

    /* Verify context — measure gets its OWN parameter, and there is no
     * sim_mode: an unsigned kernel does not boot. */
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
    uiox_ks_err_t rc;

    if (!image || image_size == 0u) return UIOX_KS_ERR_INVAL;

    rc = uiox_ks_verify_image(&g_verify_ctx, image, image_size,
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
                   g_last_report.sigs_valid);
    return UIOX_KS_OK;
}

/* =========================================================================
 * Step 3 — extend PCRs with verified image metadata
 *
 * PCR assignment follows uiox_kix_ksign_types.h:
 *   PCR[1] KERNEL_CODE   the payload hash the signature covered
 *   PCR[2] KERNEL_DATA   rodata
 *   PCR[6] KEY_LOAD      the signing key — NOT PCR[5], which is the
 *                        runtime monitor's baseline
 * ====================================================================== */
uiox_ks_err_t uiox_ks_boot_measure(const void *image, size_t image_size)
{
    const uiox_ks_img_hdr_t *hdr;
    uint8_t                  h384_trunc[UIOX_KS_SHA256_LEN];

    if (!image || image_size == 0u) return UIOX_KS_ERR_INVAL;

    hdr = (const uiox_ks_img_hdr_t *)image;

    /* PCR[1]: kernel code — the payload hash from the verified header */
    uiox_ks_measure_extend_hash(&g_measure_ctx, UIOX_KS_PCR_KERNEL_CODE,
                                 hdr->payload_hash,
                                 "kernel-payload-sha256",
                                 UIOX_KS_EVT_KERNEL_CODE);

    /* PCR[2]: kernel data — SHA-384 truncated to 32 bytes, because a PCR
     * is SHA-256 wide.  The truncation is stated rather than silent. */
    for (uint32_t i = 0; i < UIOX_KS_SHA256_LEN; i++)
        h384_trunc[i] = hdr->payload_hash384[i];

    uiox_ks_measure_extend_hash(&g_measure_ctx, UIOX_KS_PCR_KERNEL_DATA,
                                 h384_trunc,
                                 "kernel-payload-sha384",
                                 UIOX_KS_EVT_KERNEL_DATA);

    /* PCR[6]: signing key ID — KEY_LOAD, a slot of its own */
    uiox_ks_measure_extend_hash(&g_measure_ctx, UIOX_KS_PCR_KEY_LOAD,
                                 hdr->signing_key_id,
                                 "kernel-signing-key",
                                 UIOX_KS_EVT_KEY_LOAD);

    uiox_fw_printf("[ksign] PCR measurement complete (%u entries).\n",
                   g_measure_ctx.entry_count);
    return UIOX_KS_OK;
}

/* =========================================================================
 * Step 4 — seed the runtime monitor from the VERIFIED payload
 *
 * Under option A the monitored region is the payload — the extent the
 * signature covered — so the caller passes the IMAGE, and the base and
 * length come from the header.
 * ====================================================================== */
uiox_ks_err_t uiox_ks_boot_arm_runtime(const void *image,
                                          uintptr_t   rodata_base,
                                          size_t      rodata_size)
{
    const uiox_ks_img_hdr_t *hdr;
    uiox_ks_err_t            rc;

    if (!image) return UIOX_KS_ERR_INVAL;

    hdr = (const uiox_ks_img_hdr_t *)image;

    rc = uiox_ks_rt_seed_from_image(&g_rt_ctx, hdr, image,
                                      rodata_base, rodata_size);
    if (rc != UIOX_KS_OK) {
        uiox_fw_printf("[ksign] rt_seed failed: %d\n", rc);
        return rc;
    }

    uiox_fw_printf("[ksign] Runtime monitor armed: "
                   "payload=0x%lx(%llu B)  .rodata=0x%lx(%zu B)\n",
                   (unsigned long)((uintptr_t)image +
                                   (uintptr_t)hdr->payload_offset),
                   (unsigned long long)hdr->payload_size,
                   (unsigned long)rodata_base, rodata_size);
    return UIOX_KS_OK;
}

/* =========================================================================
 * Step 5 — lock PCRs and hand off to kernel entry point
 *
 * The jump target comes from the header, which was verified in step 2 —
 * so by the time control leaves here, entry_addr has been covered by the
 * payload hash and the signature.  Without step 2 running, this would be
 * an unverified jump into attacker-chosen code.
 * ====================================================================== */
typedef void (*uiox_kernel_entry_t)(void);

uiox_ks_err_t uiox_ks_boot_handoff(const void *image)
{
    const uiox_ks_img_hdr_t *hdr;
    uiox_kernel_entry_t      entry;

    if (!image) return UIOX_KS_ERR_INVAL;

    hdr = (const uiox_ks_img_hdr_t *)image;

    /* Seal PCRs — no more extends after this */
    uiox_ks_measure_lock(&g_measure_ctx);

    uiox_fw_printf("[ksign] Handing off to kernel entry 0x%016llx...\n",
                   (unsigned long long)hdr->entry_addr);

    /* Jump — this must not return on success */
    entry = (uiox_kernel_entry_t)(uintptr_t)hdr->entry_addr;
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
    uiox_ks_err_t rc;

    /* text_base/text_size are retained in the signature because Stage 0d
     * supplies them, but under option A the monitored extent comes from
     * the header — so they are informational here.  Kept rather than
     * dropped so the BSP call site does not change. */
    (void)text_base;
    (void)text_size;

    uiox_fw_printf("[ksign] === UIOX Kernel Signing Boot ===\n");

#define KS_CHECK(step)                           \
    do {                                         \
        rc = (step);                             \
        if (rc != UIOX_KS_OK) {                  \
            uiox_fw_printf("[ksign] FATAL at "   \
                #step " => %d\n", rc);           \
            uiox_ks_plat_halt();                 \
        }                                        \
    } while (0)

    KS_CHECK(uiox_ks_boot_init());
    KS_CHECK(uiox_ks_boot_verify(image, image_size));
    KS_CHECK(uiox_ks_boot_measure(image, image_size));
    KS_CHECK(uiox_ks_boot_arm_runtime(image, rodata_base, rodata_size));

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
    uiox_ks_rt_ctx_t *ctx = uiox_ksign_get_rt_ctx();

    uiox_ks_rt_tick(ctx, 0u);

    /* The tick has no return value — it records the outcome in the
     * context's state.  Read it back rather than expecting a return:
     * rt_tick sets UIOX_KS_RT_STATE_OK or _TAMPERED on every call. */
    if (ctx->state == UIOX_KS_RT_STATE_TAMPERED) {
        uiox_fw_printf("[ksign] CRITICAL: runtime integrity violation "
                       "detected on scheduler tick!\n");
        /* Policy decision: panic, log-only, or notify an audit subsystem.
         * Default: log only.  A platform wanting strict mode overrides
         * uiox_ks_plat_halt() to make this fatal. */
    }
}

/* =========================================================================
 * Attestation helper — used by sys_ksign_quote
 * ====================================================================== */
void uiox_ks_get_attestation_quote(uint8_t quote[UIOX_KS_SHA256_LEN])
{
    uiox_ks_measure_quote(&g_measure_ctx, quote);
}

/* =========================================================================
 * Context accessors
 *
 * Declared by uiox_kix_ksign.h and previously undefined — the umbrella
 * advertised six functions and this file defined none of them.
 * ====================================================================== */
uiox_ks_verify_ctx_t  *uiox_ksign_get_verify_ctx (void) { return &g_verify_ctx;  }
uiox_ks_measure_ctx_t *uiox_ksign_get_measure_ctx(void) { return &g_measure_ctx; }
uiox_ks_rt_ctx_t      *uiox_ksign_get_rt_ctx     (void) { return &g_rt_ctx;      }
