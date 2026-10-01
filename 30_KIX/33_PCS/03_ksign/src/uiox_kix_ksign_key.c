/**
 * @file  uiox_kix_ksign_key.c
 * @brief UIOX Signed Kernel — key store, KRL, key lifecycle.
 *
 * ── what changed in 1.1.0 ─────────────────────────────────────────────
 * Every public name now matches uiox_kix_ksign_key.h, which is the
 * contract other modules include.  The chain walk was called
 * uiox_ks_verify_cert_chain here AND declared in uiox_kix_ksign_verify.h
 * with a different signature; it is now uiox_ks_key_walk_chain.
 *
 * uiox_ks_key_check_valid reports whether the time window was evaluated.
 * now_sec == 0 means there is no clock, and skipping then is necessary —
 * but it is now a recorded fact rather than an unintended pass.
 *
 * uiox_ks_compute_key_id covers the whole public key in format version 2,
 * with the length and exponent encoded big-endian.  Version 1 hashed the
 * modulus alone in native byte order, so key ids collided across
 * differing exponents and depended on where they were computed.
 *
 * @version 1.1.0
 * @date    2026-10-01
 */
#include "../include/uiox_kix_ksign_key.h"

extern void uiox_fw_printf(const char *fmt, ...);

/* ── No-libc helpers ──────────────────────────────────────────────────── */
static void ks_memset(void *d, int v, size_t n)
{ uint8_t *p = (uint8_t *)d; while (n--) *p++ = (uint8_t)v; }

static void ks_memcpy(void *d, const void *s, size_t n)
{ uint8_t *dp = (uint8_t *)d; const uint8_t *sp = (const uint8_t *)s;
  while (n--) *dp++ = *sp++; }

/* Constant-time compare — the answer decides whether a key is trusted, so
 * an early exit would leak which byte differed through timing. */
static int ks_memcmp(const uint8_t *a, const uint8_t *b, size_t n)
{ uint8_t diff = 0; while (n--) diff |= (*a++ ^ *b++); return (int)diff; }

/* =========================================================================
 * Key store init
 * ====================================================================== */
uiox_ks_err_t uiox_ks_keystore_init(uiox_ks_keystore_t *ks,
                                      const uint8_t rot_key_id[UIOX_KS_KEY_ID_LEN])
{
    if (!ks || !rot_key_id) return UIOX_KS_ERR_INVAL;

    ks_memset(ks, 0, sizeof(*ks));
    ks_memcpy(ks->rot_key_id, rot_key_id, UIOX_KS_KEY_ID_LEN);
    ks->initialized = true;
    return UIOX_KS_OK;
}

/* =========================================================================
 * Add a key entry
 * ====================================================================== */
uiox_ks_err_t uiox_ks_keystore_add(uiox_ks_keystore_t        *ks,
                                     const uiox_ks_key_entry_t *entry)
{
    if (!ks || !entry)                      return UIOX_KS_ERR_INVAL;
    if (!ks->initialized)                   return UIOX_KS_ERR_INVAL;
    if (ks->key_count >= UIOX_KS_MAX_KEYS)  return UIOX_KS_ERR_NOMEM;
    if (entry->magic != UIOX_KS_KEY_MAGIC)  return UIOX_KS_ERR_BADMAGIC;

    ks_memcpy(&ks->keys[ks->key_count], entry, sizeof(*entry));
    ks->key_count++;
    return UIOX_KS_OK;
}

/* =========================================================================
 * Look up a key by key_id (SHA-256 of public key bytes)
 * ====================================================================== */
const uiox_ks_key_entry_t *uiox_ks_keystore_find(
        const uiox_ks_keystore_t *ks,
        const uint8_t             key_id[UIOX_KS_KEY_ID_LEN])
{
    if (!ks || !key_id || !ks->initialized) return (const uiox_ks_key_entry_t *)0;

    for (uint32_t i = 0; i < ks->key_count; i++) {
        const uiox_ks_key_entry_t *e = &ks->keys[i];
        if (ks_memcmp(e->key_id, key_id, UIOX_KS_KEY_ID_LEN) == 0)
            return e;
    }
    return (const uiox_ks_key_entry_t *)0;
}

/* =========================================================================
 * Key validity: time window and revocation
 * ====================================================================== */
uiox_ks_err_t uiox_ks_key_check_valid(const uiox_ks_key_entry_t *key,
                                        const uiox_ks_krl_t        *krl,
                                        uint64_t                    now_sec,
                                        bool                       *expiry_checked)
{
    if (expiry_checked) *expiry_checked = false;
    if (!key) return UIOX_KS_ERR_INVAL;

    /* Time window.
     *
     * now_sec == 0 is "no clock", not "time zero".  The window is skipped
     * and expiry_checked stays false, so the caller can report that
     * expiry was not evaluated rather than treating a pass as a verdict.
     * A zero not_after means no expiry, which IS evaluated — hence the
     * flag is set on this branch. */
    if (now_sec > 0u) {
        if (expiry_checked) *expiry_checked = true;
        if (now_sec < key->not_before)  return UIOX_KS_ERR_EXPIRED;
        if (key->not_after > 0u && now_sec > key->not_after)
            return UIOX_KS_ERR_EXPIRED;
    }

    /* KRL check — unaffected by the clock question above */
    if (krl) {
        if (krl->magic != UIOX_KS_KRL_MAGIC) return UIOX_KS_ERR_BADMAGIC;
        for (uint32_t i = 0; i < krl->entry_count &&
                              i < UIOX_KS_KRL_MAX_ENTRIES; i++) {
            if (ks_memcmp(krl->revoked_key_ids[i],
                          key->key_id, UIOX_KS_KEY_ID_LEN) == 0)
                return UIOX_KS_ERR_REVOKED;
            if (krl->revoked_serials[i] == key->serial &&
                key->serial != 0u)
                return UIOX_KS_ERR_REVOKED;
        }
    }

    return UIOX_KS_OK;
}

/* =========================================================================
 * Walk certificate chain from leaf -> root
 *
 * The signing key must chain up to the Root-of-Trust key_id stored in OTP.
 * Maximum depth: UIOX_KS_MAX_KEYS (prevents infinite loops).
 * ====================================================================== */
uiox_ks_err_t uiox_ks_key_walk_chain(const uiox_ks_keystore_t *ks,
                                       const uint8_t             leaf_key_id[UIOX_KS_KEY_ID_LEN],
                                       const uiox_ks_krl_t      *krl,
                                       uint64_t                  now_sec,
                                       bool                     *expiry_checked)
{
    uint8_t current_id[UIOX_KS_KEY_ID_LEN];

    if (expiry_checked) *expiry_checked = false;
    if (!ks || !leaf_key_id || !ks->initialized) return UIOX_KS_ERR_INVAL;

    ks_memcpy(current_id, leaf_key_id, UIOX_KS_KEY_ID_LEN);

    for (uint32_t depth = 0; depth < UIOX_KS_MAX_KEYS; depth++) {
        const uiox_ks_key_entry_t *entry =
            uiox_ks_keystore_find(ks, current_id);
        bool this_checked = false;
        uiox_ks_err_t rc;

        if (!entry) return UIOX_KS_ERR_NOTFOUND;

        rc = uiox_ks_key_check_valid(entry, krl, now_sec, &this_checked);

        /* The flag is the AND across the chain: it is only meaningful if
         * EVERY link was evaluated, because an unevaluated link is an
         * unchecked link. */
        if (this_checked && expiry_checked) *expiry_checked = true;
        if (rc != UIOX_KS_OK) return rc;

        /* Reached root of trust? */
        if (entry->is_root) {
            if (ks_memcmp(entry->key_id, ks->rot_key_id,
                          UIOX_KS_KEY_ID_LEN) == 0)
                return UIOX_KS_OK;  /* chain anchors to the OTP root */
            return UIOX_KS_ERR_CERT; /* root does not match OTP */
        }

        /* Climb to issuer */
        ks_memcpy(current_id, entry->issuer_key_id, UIOX_KS_KEY_ID_LEN);
    }

    return UIOX_KS_ERR_CERT;  /* exceeded max depth */
}

/* =========================================================================
 * Compute key_id = SHA-256(public key bytes)
 *
 * ── what the hash covers, and why it changed ────────────────────────────
 * Version 1 hashed rsa.modulus alone.  Two keys sharing a modulus and
 * differing in exponent — a legitimate, if unusual, pair — therefore
 * collided to one key_id, and a lookup by id could return the wrong key.
 *
 * Version 2 hashes the modulus, its length, and the exponent — every
 * field that separates one RSA key from another.  For ECDSA the
 * uncompressed point xy IS the whole public key, so one update suffices.
 *
 * ── byte order is EXPLICIT ──────────────────────────────────────────────
 * modulus_len and exponent are encoded big-endian by hand rather than
 * hashed as native integers.  The signing tool is a HOST binary and the
 * verifier runs on the TARGET: hashing native byte order would make a
 * key's id depend on where it was computed, so an id generated on a
 * little-endian build host would not match the same key checked on a
 * big-endian target.
 * ====================================================================== */
void uiox_ks_compute_key_id(const uiox_ks_key_entry_t *key,
                              uint8_t                    key_id[UIOX_KS_KEY_ID_LEN])
{
    uiox_ks_sha256_ctx_t h;

    if (!key || !key_id) return;

    uiox_ks_sha256_init(&h);

    if (key->alg == UIOX_KS_ALG_ECDSA_P256) {
        /* The whole public key: uncompressed x||y, one array. */
        uiox_ks_sha256_update(&h, key->ecdsa.xy,
                              UIOX_KS_ECDSA_P256_KEY_LEN);
    } else {
        /* RSA: modulus, then length and exponent big-endian.  The length
         * goes in so a short modulus followed by exponent bytes cannot be
         * read as a longer modulus. */
        uint8_t be[8];

        be[0] = (uint8_t)(key->rsa.modulus_len >> 24);
        be[1] = (uint8_t)(key->rsa.modulus_len >> 16);
        be[2] = (uint8_t)(key->rsa.modulus_len >>  8);
        be[3] = (uint8_t)(key->rsa.modulus_len      );
        be[4] = (uint8_t)(key->rsa.exponent      >> 24);
        be[5] = (uint8_t)(key->rsa.exponent      >> 16);
        be[6] = (uint8_t)(key->rsa.exponent      >>  8);
        be[7] = (uint8_t)(key->rsa.exponent          );

        uiox_ks_sha256_update(&h, key->rsa.modulus, key->rsa.modulus_len);
        uiox_ks_sha256_update(&h, be, sizeof be);
    }

    uiox_ks_sha256_final(&h, key_id);
}

/* =========================================================================
 * Attach / replace KRL
 * ====================================================================== */
uiox_ks_err_t uiox_ks_keystore_set_krl(uiox_ks_keystore_t *ks,
                                         uiox_ks_krl_t      *krl)
{
    if (!ks || !krl)                      return UIOX_KS_ERR_INVAL;
    if (krl->magic != UIOX_KS_KRL_MAGIC)  return UIOX_KS_ERR_BADMAGIC;

    ks->krl = krl;
    return UIOX_KS_OK;
}

/* =========================================================================
 * Print
 * ====================================================================== */
void uiox_ks_keystore_print(const uiox_ks_keystore_t *ks)
{
    static const char *alg_str[] = { "NONE", "RSA2048-SHA256", "RSA4096-SHA256",
                                     "ECDSA-P256", "Ed25519" };
    if (!ks) return;

    uiox_fw_printf("[ksign] Key store (%u entries):\n", ks->key_count);
    for (uint32_t i = 0; i < ks->key_count; i++) {
        const uiox_ks_key_entry_t *e = &ks->keys[i];
        uiox_fw_printf("  [%u] %-48s  alg=%-16s  root=%s  active=%s\n",
                       i, e->name,
                       e->alg < 5u ? alg_str[e->alg] : "?",
                       e->is_root ? "YES" : "NO",
                       e->active  ? "YES" : "NO");
    }
    if (ks->krl)
        uiox_fw_printf("  KRL: version=%u  revoked=%u\n",
                       ks->krl->version, ks->krl->entry_count);
}
