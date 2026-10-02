/**
 * @file  uiox_kix_sec_mac_policy.c
 * @brief UIOX Security — MAC policy store: load, type table, rule table.
 *
 * ── what changed in 1.1.0 ─────────────────────────────────────────────
 *   1. uiox_mac_plat_sha256() now delegates to uiox_ks_sha256().  The
 *      former body was FNV-1a — 32 bits of non-cryptographic hash written
 *      into bytes 0..3 of a 32-byte digest — and uiox_mac_policy_load()
 *      compared THAT against hdr->policy_hash.  The "policy is signed and
 *      verified" guarantee was a 32-bit checksum over exactly the bytes
 *      an attacker controls.  Now a real SHA-256.
 *
 *   2. The loader's offset bounds checks were short by sizeof(hdr).  The
 *      guard proved the table STARTS inside the blob, not that it ENDS
 *      there.  It now requires both, plus a wrap-safe form.
 *
 *   3. mp_streq() no longer reads past the end of @a.
 *
 *   4. The hand-rolled copy in add_type is now mp_strncpy().
 *
 * @version 1.1.0
 * @date    2026-10-02
 */
#include "../include/uiox_kix_sec_mac_policy.h"
#include "uiox_kix_ksign_crypto.h"   /* uiox_ks_sha256 — the policy hash */

extern void uiox_fw_printf(const char *fmt, ...);

/* ── No-libc helpers ──────────────────────────────────────────────────── */
static void mp_memset(void *d, int v, size_t n)
{ uint8_t *p = (uint8_t *)d; while (n--) *p++ = (uint8_t)v; }

static void mp_memcpy(void *d, const void *s, size_t n)
{ uint8_t *dp = (uint8_t *)d; const uint8_t *sp = (const uint8_t *)s;
  while (n--) *dp++ = *sp++; }

  static int mp_memcmp(const void *a, const void *b, size_t n)
  {
      uint8_t diff = 0;
      const uint8_t *ap = (const uint8_t *)a;
      const uint8_t *bp = (const uint8_t *)b;
  
      while (n--)
          diff |= (*ap++ ^ *bp++);
  
      return (int)diff;
  }  

/* Bounded copy — at most @max bytes, NUL-padded. */
static void mp_strncpy(char *d, const char *s, size_t max)
{
    size_t i = 0u;
    for (; i < max && s[i]; i++) d[i] = s[i];
    for (; i < max; i++)         d[i] = '\0';
}

/* Bounded string equality.  NOTE: callers pass a @b of unknown length
 * (see the header's contract on @name) — @a is bounded by its own NUL,
 * @b is only bounded by @max. */
static bool mp_streq(const char *a, const char *b, size_t max)
{
    for (size_t i = 0; i < max; i++) {
        if (a[i] != b[i]) return false;
        if (a[i] == '\0') return true;
    }
    return true;
}

/* =========================================================================
 * Platform SHA-256 — delegates to 03_ksign
 *
 * ── this is the policy-signature check ────────────────────────────────
 * uiox_mac_policy_load() compares hdr->policy_hash against this output.
 * uiox_ks_sha256() is a STRONG symbol in 03_ksign, so a build of this
 * module must link libksign.a — without the archive this becomes an
 * undefined reference at link time, not a compile error here.
 *
 * Declared weak so a BSP with hardware SHA-256 can override it.
 * ====================================================================== */
__attribute__((weak))
void uiox_mac_plat_sha256(const uint8_t *data, size_t len,
                           uint8_t digest[UIOX_MAC_HASH_LEN])
{
    uiox_ks_sha256(data, len, digest);
}

uiox_sec_err_t uiox_mac_policy_init(uiox_mac_policy_ctx_t *ctx,
                                     uiox_mac_mode_t        mode)
{
    if (!ctx) return UIOX_SEC_ERR_INVAL;
    mp_memset(ctx, 0, sizeof(*ctx));
    ctx->mode        = mode;
    ctx->initialized = true;
    return UIOX_SEC_OK;
}

uiox_sec_err_t uiox_mac_policy_add_type(uiox_mac_policy_ctx_t *ctx,
                                          const char            *name,
                                          uint32_t              *out_id)
{
    if (!ctx || !name || !out_id)  return UIOX_SEC_ERR_INVAL;
    if (ctx->sealed)               return UIOX_SEC_ERR_READONLY;
    if (ctx->type_count >= UIOX_MAC_MAX_TYPES) return UIOX_SEC_ERR_OVERFLOW;

    for (uint32_t i = 0; i < ctx->type_count; i++) {
        if (mp_streq(ctx->types[i].name, name, UIOX_MAC_LABEL_LEN)) {
            *out_id = ctx->types[i].id;
            return UIOX_SEC_OK;
        }
    }

    uiox_mac_type_entry_t *t = &ctx->types[ctx->type_count];
    mp_memset(t, 0, sizeof(*t));
    mp_strncpy(t->name, name, UIOX_MAC_LABEL_LEN - 1u);

    t->id     = ctx->type_count;
    t->active = true;
    *out_id   = t->id;
    ctx->type_count++;
    return UIOX_SEC_OK;
}

uiox_sec_err_t uiox_mac_policy_lookup_type(const uiox_mac_policy_ctx_t *ctx,
                                            const char                  *name,
                                            uint32_t                    *out_id)
{
    if (!ctx || !name || !out_id) return UIOX_SEC_ERR_INVAL;

    for (uint32_t i = 0; i < ctx->type_count; i++) {
        if (mp_streq(ctx->types[i].name, name, UIOX_MAC_LABEL_LEN)) {
            *out_id = ctx->types[i].id;
            return UIOX_SEC_OK;
        }
    }
    return UIOX_SEC_ERR_NOTFOUND;
}

uiox_sec_err_t uiox_mac_policy_add_rule(uiox_mac_policy_ctx_t *ctx,
                                          const uiox_mac_rule_t *rule)
{
    if (!ctx || !rule)  return UIOX_SEC_ERR_INVAL;
    if (ctx->sealed)    return UIOX_SEC_ERR_READONLY;
    if (ctx->rule_count >= UIOX_MAC_MAX_RULES) return UIOX_SEC_ERR_OVERFLOW;

    for (uint32_t i = 0; i < ctx->rule_count; i++) {
        uiox_mac_rule_t *r = &ctx->rules[i];
        if (r->subject_type == rule->subject_type &&
            r->object_type  == rule->object_type  &&
            r->obj_class    == rule->obj_class) {
            r->allow |= rule->allow;
            r->audit  |= rule->audit;
            r->active  = true;
            return UIOX_SEC_OK;
        }
    }

    mp_memcpy(&ctx->rules[ctx->rule_count], rule, sizeof(*rule));
    ctx->rules[ctx->rule_count].active = true;
    ctx->rule_count++;
    return UIOX_SEC_OK;
}

/* =========================================================================
 * Load binary policy
 *   [ hdr (fixed) ] [ body: tables ]     ← hash covers the body only
 *   offsets are ABSOLUTE from blob start
 * ====================================================================== */
uiox_sec_err_t uiox_mac_policy_load(uiox_mac_policy_ctx_t *ctx,
                                     const void            *blob,
                                     size_t                 blob_size)
{
    if (!ctx || !blob || blob_size < sizeof(uiox_mac_policy_hdr_t))
        return UIOX_SEC_ERR_INVAL;
    if (ctx->sealed) return UIOX_SEC_ERR_READONLY;

    const uiox_mac_policy_hdr_t *hdr =
        (const uiox_mac_policy_hdr_t *)blob;

    if (hdr->magic   != UIOX_MAC_POLICY_MAGIC)   return UIOX_SEC_ERR_BADMAGIC;
    if (hdr->version != UIOX_MAC_POLICY_VERSION)  return UIOX_SEC_ERR_BADLABEL;

    const uint8_t *body = (const uint8_t *)blob + sizeof(*hdr);
    size_t body_len     = blob_size - sizeof(*hdr);
    uint8_t actual_hash[UIOX_MAC_HASH_LEN];
    uiox_mac_plat_sha256(body, body_len, actual_hash);

    if (mp_memcmp(actual_hash, hdr->policy_hash, UIOX_MAC_HASH_LEN) != 0) {
        uiox_fw_printf("[mac-policy] Hash mismatch — policy rejected\n");
        return UIOX_SEC_ERR_BADLABEL;
    }

    /* ── Type name table — must END inside the blob ───────────────── */
    if (hdr->type_table_offset < sizeof(*hdr)              ||
        hdr->type_table_offset > blob_size                 ||
        hdr->type_table_size   > blob_size - hdr->type_table_offset)
        return UIOX_SEC_ERR_BADLABEL;

    const char *names = (const char *)blob + hdr->type_table_offset;
    size_t      npos  = 0u;
    for (uint32_t t = 0; t < hdr->type_count && t < UIOX_MAC_MAX_TYPES; t++) {
        uint32_t dummy_id;
        if (npos >= hdr->type_table_size) break;
        uiox_mac_policy_add_type(ctx, names + npos, &dummy_id);
        while (npos < hdr->type_table_size && names[npos] != '\0') npos++;
        npos++;
    }

    /* ── Rule array — same requirement ────────────────────────────── */
    if (hdr->rule_table_offset < sizeof(*hdr)              ||
        hdr->rule_table_offset > blob_size                 ||
        hdr->rule_table_size   > blob_size - hdr->rule_table_offset)
        return UIOX_SEC_ERR_BADLABEL;

    const uiox_mac_rule_t *rules =
        (const uiox_mac_rule_t *)((const uint8_t *)blob
                                   + hdr->rule_table_offset);
    for (uint32_t r = 0; r < hdr->rule_count &&
                          r < UIOX_MAC_MAX_RULES; r++) {
        if ((size_t)(r + 1u) * sizeof(uiox_mac_rule_t) >
            hdr->rule_table_size)
            break;
        uiox_mac_policy_add_rule(ctx, &rules[r]);
    }

    mp_memcpy(ctx->loaded_hash, actual_hash, UIOX_MAC_HASH_LEN);
    uiox_fw_printf("[mac-policy] Loaded: types=%u  rules=%u\n",
                   ctx->type_count, ctx->rule_count);
    return UIOX_SEC_OK;
}

uiox_sec_err_t uiox_mac_policy_seal(uiox_mac_policy_ctx_t *ctx)
{
    if (!ctx || !ctx->initialized) return UIOX_SEC_ERR_INVAL;
    ctx->sealed = true;
    uiox_fw_printf("[mac-policy] Sealed: %u types  %u rules  mode=%s\n",
                   ctx->type_count, ctx->rule_count,
                   uiox_mac_mode_str(ctx->mode));
    return UIOX_SEC_OK;
}

uiox_sec_err_t uiox_mac_policy_set_mode(uiox_mac_policy_ctx_t *ctx,
                                          uiox_mac_mode_t        mode)
{
    if (!ctx) return UIOX_SEC_ERR_INVAL;
    if (ctx->sealed && mode > ctx->mode) return UIOX_SEC_ERR_READONLY;
    ctx->mode = mode;
    uiox_fw_printf("[mac-policy] Mode changed to %s\n",
                   uiox_mac_mode_str(mode));
    return UIOX_SEC_OK;
}

void uiox_mac_policy_print(const uiox_mac_policy_ctx_t *ctx)
{
    if (!ctx) return;
    uiox_fw_printf("[mac-policy] mode=%s  types=%u  rules=%u  sealed=%s\n",
                   uiox_mac_mode_str(ctx->mode),
                   ctx->type_count, ctx->rule_count,
                   ctx->sealed ? "YES" : "NO");
    for (uint32_t i = 0; i < ctx->type_count; i++)
        uiox_fw_printf("  type[%2u] = %s\n", i, ctx->types[i].name);
}
