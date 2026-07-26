/**
 * @file  uiox_ksign_key.c
 * @brief UIOX Signed Kernel — key store, KRL, key lifecycle.
 *
 * Manages:
 *   - Key store initialisation and lookup
 *   - Root-of-trust anchoring from OTP
 *   - Certificate chain walking and validation
 *   - Key Revocation List (KRL) checking
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
#include "../include/uiox_ksign_key.h"

extern void uiox_fw_printf(const char *fmt, ...);

/* ── No-libc helpers ──────────────────────────────────────────────────── */
static void ks_memset(void *d, int v, size_t n)
{ uint8_t *p = (uint8_t *)d; while (n--) *p++ = (uint8_t)v; }

static void ks_memcpy(void *d, const void *s, size_t n)
{ uint8_t *dp = (uint8_t *)d; const uint8_t *sp = (const uint8_t *)s;
  while (n--) *dp++ = *sp++; }

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
    if (!ks || !entry)                              return UIOX_KS_ERR_INVAL;
    if (!ks->initialized)                           return UIOX_KS_ERR_INVAL;
    if (ks->key_count >= UIOX_KS_MAX_KEYS)         return UIOX_KS_ERR_NOMEM;
    if (entry->magic != UIOX_KS_KEY_MAGIC)         return UIOX_KS_ERR_BADMAGIC;

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
    if (!ks || !key_id || !ks->initialized) return NULL;

    for (uint32_t i = 0; i < ks->key_count; i++) {
        const uiox_ks_key_entry_t *e = &ks->keys[i];
        if (ks_memcmp(e->key_id, key_id, UIOX_KS_KEY_ID_LEN) == 0)
            return e;
    }
    return NULL;
}

/* =========================================================================
 * Key validity: not expired, not revoked
 * ====================================================================== */
uiox_ks_err_t uiox_ks_key_check_valid(const uiox_ks_key_entry_t *key,
                                        const uiox_ks_krl_t        *krl,
                                        uint64_t                    now_sec)
{
    if (!key) return UIOX_KS_ERR_INVAL;

    /* Time window */
    if (now_sec > 0u) {
        if (now_sec < key->not_before)  return UIOX_KS_ERR_EXPIRED;
        if (key->not_after > 0u && now_sec > key->not_after)
            return UIOX_KS_ERR_EXPIRED;
    }

    /* KRL check */
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
 * Walk certificate chain from leaf → root
 *
 * The signing key must chain up to the Root-of-Trust key_id stored in OTP.
 * Maximum depth: UIOX_KS_MAX_KEYS (prevents infinite loops).
 * ====================================================================== */
uiox_ks_err_t uiox_ks_verify_cert_chain(const uiox_ks_keystore_t  *ks,
                                          const uint8_t              leaf_key_id[UIOX_KS_KEY_ID_LEN],
                                          const uiox_ks_krl_t       *krl,
                                          uint64_t                   now_sec)
{
    if (!ks || !leaf_key_id || !ks->initialized) return UIOX_KS_ERR_INVAL;

    uint8_t current_id[UIOX_KS_KEY_ID_LEN];
    ks_memcpy(current_id, leaf_key_id, UIOX_KS_KEY_ID_LEN);

    for (uint32_t depth = 0; depth < UIOX_KS_MAX_KEYS; depth++) {
        const uiox_ks_key_entry_t *entry =
            uiox_ks_keystore_find(ks, current_id);
        if (!entry) return UIOX_KS_ERR_NOTFOUND;

        /* Validity */
        uiox_ks_err_t rc = uiox_ks_key_check_valid(entry, krl, now_sec);
        if (rc != UIOX_KS_OK) return rc;

        /* Reached root of trust? */
        if (entry->is_root) {
            if (ks_memcmp(entry->key_id, ks->rot_key_id,
                          UIOX_KS_KEY_ID_LEN) == 0)
                return UIOX_KS_OK;  /* ✓ chain anchors to OTP root */
            return UIOX_KS_ERR_CERT; /* root doesn't match OTP */
        }

        /* Climb to issuer */
        ks_memcpy(current_id, entry->issuer_key_id, UIOX_KS_KEY_ID_LEN);
    }

    return UIOX_KS_ERR_CERT;  /* exceeded max depth */
}

/* =========================================================================
 * Compute key_id = SHA-256(public key bytes)
 * ====================================================================== */
void uiox_ks_compute_key_id(const uiox_ks_key_entry_t *key,
                              uint8_t                    key_id[UIOX_KS_KEY_ID_LEN])
{
    if (!key || !key_id) return;

    /* Hash the relevant public key bytes based on algorithm */
    if (key->alg == UIOX_KS_ALG_ECDSA_P256) {
        uiox_ks_sha256(key->ecdsa.pub_x, UIOX_KS_ECDSA_COORD_LEN * 2u, key_id);
    } else {
        /* RSA: hash modulus */
        uiox_ks_sha256(key->rsa.modulus, key->rsa.modulus_len, key_id);
    }
}

/* =========================================================================
 * Attach / replace KRL
 * ====================================================================== */
uiox_ks_err_t uiox_ks_keystore_set_krl(uiox_ks_keystore_t *ks,
                                         uiox_ks_krl_t      *krl)
{
    if (!ks || !krl)                        return UIOX_KS_ERR_INVAL;
    if (krl->magic != UIOX_KS_KRL_MAGIC)   return UIOX_KS_ERR_BADMAGIC;

    ks->krl = krl;
    return UIOX_KS_OK;
}

/* =========================================================================
 * Print
 * ====================================================================== */
void uiox_ks_keystore_print(const uiox_ks_keystore_t *ks)
{
    if (!ks) return;
    const char *alg_str[] = { "NONE", "RSA2048-SHA256", "RSA4096-SHA256",
                               "ECDSA-P256", "Ed25519" };
    uiox_fw_printf("[ksign] Key store (%u entries):\n", ks->key_count);
    for (uint32_t i = 0; i < ks->key_count; i++) {
        const uiox_ks_key_entry_t *e = &ks->keys[i];
        uiox_fw_printf("  [%u] %-48s  alg=%-16s  root=%s  active=%s\n",
                       i, e->name,
                       e->alg < 5u ? alg_str[e->alg] : "?",
                       e->is_root  ? "YES" : "NO",
                       e->active   ? "YES" : "NO");
    }
    if (ks->krl)
        uiox_fw_printf("  KRL: version=%u  revoked=%u\n",
                       ks->krl->version, ks->krl->entry_count);
}
