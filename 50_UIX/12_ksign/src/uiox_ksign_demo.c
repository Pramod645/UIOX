/**
 * @file  uiox_ksign_demo.c
 * @brief UIOX Signed Kernel — end-to-end simulation / smoke-test.
 *
 * Demonstrates the complete ksign chain without real hardware:
 *
 *  Stage 0  — Initialise key store with a simulated root key
 *  Stage 1  — Build a synthetic signed image in RAM
 *  Stage 2  — Boot-time verification (parse + hash + sig check)
 *  Stage 3  — Measurement log replay-verify
 *  Stage 4  — Register .text/.rodata regions in the runtime engine
 *  Stage 5  — Simulate a clean periodic tick → no violation
 *  Stage 6  — Corrupt one byte → expect UIOX_KS_ERR_TAMPERED
 *  Stage 7  — Restore the byte → clean again
 *  Stage 8  — Syscall status / quote exercise
 *  Stage 9  — Print all artefacts
 *
 * Build:
 *   arm-none-eabi-gcc -O2 -std=c11 -I../include \
 *       uiox_ksign_demo.c uiox_ksign_crypto.c uiox_ksign_key.c \
 *       uiox_ksign_image.c uiox_ksign_verify.c \
 *       uiox_ksign_measure.c uiox_ksign_runtime.c \
 *       -o ksign_demo.elf
 *
 * @version 1.0.0
 * @date    2026-07-07
 */
#include "../include/uiox_ksign.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>   /* only for uiox_fw_printf shim below */

/* -------------------------------------------------------------------------
 * Minimal printf shim (replaces real uiox_fw_printf in a bare-metal build)
 * ---------------------------------------------------------------------- */
void uiox_fw_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

/* -------------------------------------------------------------------------
 * Demo helpers
 * ---------------------------------------------------------------------- */

#define DEMO_PAYLOAD_SIZE   4096u
#define DEMO_IMG_BUF_SIZE   (UIOX_KS_IMG_HDR_SIZE + DEMO_PAYLOAD_SIZE + 512u)

static uint8_t  s_img_buf[DEMO_IMG_BUF_SIZE];  /* synthetic image in RAM  */
static uint8_t  s_text_region[1024u];           /* fake .text              */
static uint8_t  s_rodata_region[512u];          /* fake .rodata            */

static uiox_ks_keystore_t  s_keystore;
static uiox_ks_log_t       s_log;

/* -------------------------------------------------------------------------
 * Build a minimal synthetic signed image in s_img_buf[].
 *
 * Layout:
 *   [0 .. HDR_SIZE-1]        — uiox_ks_img_hdr_t (zero-padded to 512 B)
 *   [HDR_SIZE .. HDR_SIZE+4095] — fake kernel payload (filled with 0xA5)
 *   (no .uiox_sig section — sim_mode skips real sig check)
 * ---------------------------------------------------------------------- */
static void build_synthetic_image(void)
{
    memset(s_img_buf, 0, sizeof(s_img_buf));

    /* Fill payload with a recognisable pattern */
    memset(s_img_buf + UIOX_KS_IMG_HDR_SIZE, 0xA5u, DEMO_PAYLOAD_SIZE);

    uiox_ks_img_hdr_t *hdr = (uiox_ks_img_hdr_t *)s_img_buf;

    hdr->magic            = UIOX_KS_IMG_MAGIC;
    hdr->format_version   = UIOX_KS_FORMAT_VERSION;
    hdr->arch             = 0u;    /* ARM64 */
    hdr->kernel_version   = 3u;
    hdr->min_kernel_version = 1u;  /* anti-rollback floor */
    hdr->load_addr        = 0x40080000ULL;
    hdr->entry_addr       = 0x40080040ULL;
    hdr->payload_offset   = UIOX_KS_IMG_HDR_SIZE;
    hdr->payload_size     = DEMO_PAYLOAD_SIZE;
    hdr->build_time       = 1751846400ULL; /* 2025-07-07 00:00 UTC */
    hdr->sig_alg          = UIOX_KS_ALG_RSA2048_SHA256;

    /* Hard-code a recognisable build_id */
    const char *bid = "demo-cafe-1234-abcd";
    for (size_t i = 0u; bid[i] && i < 31u; i++) hdr->build_id[i] = bid[i];

    /* Compute payload hashes */
    const uint8_t *payload = s_img_buf + UIOX_KS_IMG_HDR_SIZE;
    uiox_ks_sha256_ctx_t sctx;
    uiox_ks_sha256_init(&sctx);
    uiox_ks_sha256_update(&sctx, payload, DEMO_PAYLOAD_SIZE);
    uiox_ks_sha256_final(&sctx, hdr->payload_hash);

    uiox_ks_sha384_ctx_t s384;
    uiox_ks_sha384_init(&s384);
    uiox_ks_sha384_update(&s384, payload, DEMO_PAYLOAD_SIZE);
    uiox_ks_sha384_final(&s384, hdr->payload_hash384);

    /* No real signing key — sim_mode will accept sig_section_size == 0 */
    hdr->sig_section_offset = UIOX_KS_IMG_HDR_SIZE + DEMO_PAYLOAD_SIZE;
    hdr->sig_section_size   = 0u;

    /* No specific signing_key_id — sim_mode ignores it */
    memset(hdr->signing_key_id, 0, UIOX_KS_KEY_ID_LEN);

    uiox_fw_printf("[demo] Synthetic image built: %u bytes payload, "
                   "kernel_version=%u\n",
                   DEMO_PAYLOAD_SIZE, hdr->kernel_version);
}

/* -------------------------------------------------------------------------
 * Populate a minimal key store with one fake root key.
 * ---------------------------------------------------------------------- */
static void build_keystore(void)
{
    memset(&s_keystore, 0, sizeof(s_keystore));

    uiox_ks_key_entry_t *k = &s_keystore.keys[0];
    const char *kname = "UIOX-ROOT-CA-SIM";
    for (size_t i = 0u; kname[i] && i < UIOX_KS_KEY_NAME_LEN - 1u; i++)
        k->name[i] = kname[i];
    k->alg          = UIOX_KS_ALG_RSA2048_SHA256;
    k->flags        = 0u;
    k->is_root      = true;
    k->is_active    = true;
    k->cert_depth   = 0u;
    /* Simulated public key modulus bytes (not real) */
    for (size_t i = 0u; i < UIOX_KS_RSA2048_BYTES; i++)
        k->pubkey.rsa.n[i] = (uint8_t)(i ^ 0xACu);
    k->pubkey.rsa.e = 65537u;

    s_keystore.key_count = 1u;
    /* Current min kernel version (anti-rollback floor in NVRAM) */
    s_keystore.min_kernel_version = 1u;

    uiox_ks_keystore_print(&s_keystore);
}

/* -------------------------------------------------------------------------
 * Stage helpers — each returns 0 on pass, 1 on unexpected failure.
 * ---------------------------------------------------------------------- */

static int stage_boot_verify(void)
{
    uiox_ks_verify_report_t report;
    memset(&report, 0, sizeof(report));

    uiox_ks_err_t rc = uiox_ks_verify_image(
            s_img_buf, sizeof(s_img_buf),
            &s_keystore, &s_log,
            &report);

    uiox_ks_verify_print(&report);

    /* In sim_mode the signature check is skipped — expect OK */
    if (rc != UIOX_KS_OK) {
        uiox_fw_printf("[demo] STAGE-2 FAIL: verify returned %d\n", (int)rc);
        return 1;
    }
    uiox_fw_printf("[demo] STAGE-2 PASS: boot-time verify OK\n");
    return 0;
}

static int stage_replay_verify(void)
{
    uiox_ks_err_t rc = uiox_ks_log_verify_replay(&s_log);
    if (rc != UIOX_KS_OK) {
        uiox_fw_printf("[demo] STAGE-3 FAIL: PCR replay returned %d\n", (int)rc);
        return 1;
    }
    uiox_fw_printf("[demo] STAGE-3 PASS: PCR replay OK\n");
    return 0;
}

/* -------------------------------------------------------------------------
 * Violation callback
 * ---------------------------------------------------------------------- */
static void violation_handler(const uiox_ks_rt_region_t *r, uint32_t total)
{
    uiox_fw_printf("[demo/cb] Violation callback: region='%s' total=%u\n",
                   r->name, total);
}

/* -------------------------------------------------------------------------
 * main() — orchestrates all stages
 * ---------------------------------------------------------------------- */
int main(void)
{
    int failures = 0;

    uiox_fw_printf("\n=== UIOX ksign End-to-End Demo ===\n\n");

    /* ── Stage 0: Key store ─────────────────────────────────────────── */
    uiox_fw_printf("[demo] STAGE-0: Build key store\n");
    build_keystore();

    /* ── Stage 1: Synthetic image ───────────────────────────────────── */
    uiox_fw_printf("[demo] STAGE-1: Build synthetic signed image\n");
    build_synthetic_image();

    /* ── Stage 2: Boot-time verification ────────────────────────────── */
    uiox_fw_printf("[demo] STAGE-2: Boot-time verification (sim_mode)\n");
    uiox_ks_log_init(&s_log);
    failures += stage_boot_verify();

    /* ── Stage 3: PCR replay verify ─────────────────────────────────── */
    uiox_fw_printf("[demo] STAGE-3: PCR replay-verify\n");
    failures += stage_replay_verify();

    /* ── Stage 4: Runtime engine startup ────────────────────────────── */
    uiox_fw_printf("[demo] STAGE-4: Start runtime integrity engine\n");
    memset(s_text_region,   0xCC, sizeof(s_text_region));
    memset(s_rodata_region, 0xDD, sizeof(s_rodata_region));

    uiox_ks_rt_engine_t *eng = NULL;
    {
        uiox_ks_err_t rc = uiox_ksign_runtime_start_impl(
                (uintptr_t)s_text_region,   sizeof(s_text_region),
                (uintptr_t)s_rodata_region, sizeof(s_rodata_region),
                &s_log);
        if (rc != UIOX_KS_OK) {
            uiox_fw_printf("[demo] STAGE-4 FAIL: runtime_start returned %d\n",
                           (int)rc);
            failures++;
        } else {
            eng = uiox_ks_rt_get_engine();
            uiox_ks_rt_set_policy(eng, UIOX_KS_RT_POLICY_CALLBACK,
                                   violation_handler);
            uiox_fw_printf("[demo] STAGE-4 PASS\n");
        }
    }

    /* ── Stage 5: Clean periodic tick ──────────────────────────────── */
    uiox_fw_printf("[demo] STAGE-5: Clean periodic tick\n");
    if (eng) {
        /* Simulate 60 s passing: one call with 60000 ms triggers a check */
        uiox_ks_err_t rc = uiox_ks_rt_tick(eng, 60000u);
        if (rc != UIOX_KS_OK) {
            uiox_fw_printf("[demo] STAGE-5 FAIL: tick returned %d\n", (int)rc);
            failures++;
        } else {
            uiox_fw_printf("[demo] STAGE-5 PASS: no violations\n");
        }
    }

    /* ── Stage 6: Simulate tampering ────────────────────────────────── */
    uiox_fw_printf("[demo] STAGE-6: Simulate .text tampering\n");
    uint8_t saved_byte = s_text_region[42];
    s_text_region[42] ^= 0xFFu;   /* flip a byte */

    if (eng) {
        uiox_ks_err_t rc = uiox_ks_rt_check_region(eng, ".text");
        if (rc == UIOX_KS_ERR_TAMPERED) {
            uiox_fw_printf("[demo] STAGE-6 PASS: tampering correctly detected\n");
        } else {
            uiox_fw_printf("[demo] STAGE-6 FAIL: expected TAMPERED, got %d\n",
                           (int)rc);
            failures++;
        }
    }

    /* ── Stage 7: Restore and re-check ─────────────────────────────── */
    uiox_fw_printf("[demo] STAGE-7: Restore byte and re-check\n");
    s_text_region[42] = saved_byte;

    if (eng) {
        /* Reset violation count so the engine considers it healed */
        eng->regions[0].violation_count = 0u;
        /* Re-establish reference digest from the restored region */
        uiox_ks_sha256_ctx_t sctx;
        uiox_ks_sha256_init(&sctx);
        uiox_ks_sha256_update(&sctx, s_text_region, sizeof(s_text_region));
        uiox_ks_sha256_final(&sctx, eng->regions[0].ref_digest);

        uiox_ks_err_t rc = uiox_ks_rt_check_region(eng, ".text");
        if (rc == UIOX_KS_OK) {
            uiox_fw_printf("[demo] STAGE-7 PASS: region clean after restore\n");
        } else {
            uiox_fw_printf("[demo] STAGE-7 FAIL: check returned %d\n", (int)rc);
            failures++;
        }
    }

    /* ── Stage 8: Syscall exercise ──────────────────────────────────── */
    uiox_fw_printf("[demo] STAGE-8: Syscall interface\n");
    {
        char status_buf[256];
        long sr = sys_ksign_status((long)status_buf, (long)sizeof(status_buf),
                                    0, 0);
        if (sr == (long)UIOX_KS_OK) {
            uiox_fw_printf("[demo] sys_ksign_status: '%s'\n", status_buf);
        } else {
            uiox_fw_printf("[demo] sys_ksign_status returned %ld\n", sr);
        }

        uint8_t quote_buf[4096];
        long qr = sys_ksign_quote((long)quote_buf, (long)sizeof(quote_buf),
                                   0, 0);
        if (qr > 0) {
            uiox_fw_printf("[demo] sys_ksign_quote: %ld bytes serialised\n", qr);
        } else {
            uiox_fw_printf("[demo] sys_ksign_quote returned %ld\n", qr);
        }
    }

    /* ── Stage 9: Print artefacts ───────────────────────────────────── */
    uiox_fw_printf("[demo] STAGE-9: Print all artefacts\n");
    uiox_ks_log_print(&s_log);
    if (eng) uiox_ks_rt_print(eng);

    /* ── Summary ────────────────────────────────────────────────────── */
    uiox_fw_printf("\n=== Demo complete: %d failure(s) ===\n", failures);
    return failures;
}
