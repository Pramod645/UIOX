/**
 * @file  uiox_mac_policy.c
 * @brief UIOX Security — MAC policy store: load, type table, rule table.
 * @date  2026-07-08
 */
#include "../include/uiox_mac_policy.h"

extern void uiox_fw_printf(const char *fmt, ...);

/* ── No-libc helpers ──────────────────────────────────────────────────── */
static void mp_memset(void *d, int v, size_t n)
{ uint8_t *p = (uint8_t *)d; while (n--) *p++ = (uint8_t)v; }

static void mp_memcpy(void *d, const void *s, size_t n)
{ uint8_t *dp = (uint8_t *)d; const uint8_t *sp = (const uint8_t *)s;
  while (n--) *dp++ = *sp++; }

static int mp_memcmp(const void *a, const void *b, size_t n)
{ uint8_t diff = 0; const uint8_t *ap = (const uint8_t *)a;
  const uint8_t *bp = (const uint8_t *)b;
  while (n--) diff |= (*ap++ ^ *bp++); return (int)diff; }

static bool mp_streq(const char *a, const char *b, size_t max)
{
    for (size_t i = 0; i < max; i++) {
        if (a[i] != b[i]) return false;
        if (a[i] == '\0') return true;
    }
    return true;
}

/* Platform SHA-256 — delegates to 12_ksign when linked */
__attribute__((weak))
void uiox_mac_plat_sha256(const uint8_t *data, size_t len,
                           uint8_t digest[UIOX_MAC_HASH_LEN])
{
    /* Minimal FNV-1a stub — replace with uiox_ks_sha256() */
    uint32_t h = 0x811c9dc5u;
    for (size_t i = 0; i < len; i++) {
        h ^= data[i]; h *= 0x01000193u;
    }
    mp_memset(digest, 0, UIOX_MAC_HASH_LEN);
    digest[0] = (uint8_t)(h >> 24); digest[1] = (uint8_t)(h >> 16);
    digest[2] = (uint8_t)(h >>  8); digest[3] = (uint8_t)(h);
}

/* =========================================================================
 * Init
 * ====================================================================== */
uiox_sec_err_t uiox_mac_policy_init(uiox_mac_policy_ctx_t *ctx,
                                     uiox_mac_mode_t        mode)
{
    if (!ctx) return UIOX_SEC_ERR_INVAL;
    mp_memset(ctx, 0, sizeof(*ctx));
    ctx->mode        = mode;
    ctx->initialized = true;
    return UIOX_SEC_OK;
}

/* =========================================================================
 * Add type
 * ====================================================================== */
uiox_sec_err_t uiox_mac_policy_add_type(uiox_mac_policy_ctx_t *ctx,
                                          const char            *name,
                                          uint32_t              *out_id)
{
    if (!ctx || !name || !out_id)  return UIOX_SEC_ERR_INVAL;
    if (ctx->sealed)               return UIOX_SEC_ERR_READONLY;
    if (ctx->type_count >= UIOX_MAC_MAX_TYPES) return UIOX_SEC_ERR_OVERFLOW;

    /* Idempotent — return existing id */
    for (uint32_t i = 0; i < ctx->type_count; i++) {
        if (mp_streq(ctx->types[i].name, name, UIOX_MAC_LABEL_LEN)) {
            *out_id = ctx->types[i].id;
            return UIOX_SEC_OK;
        }
    }

    uiox_mac_type_entry_t *t = &ctx->types[ctx->type_count];
    mp_memset(t, 0, sizeof(*t));
    /* strncpy without libc */
    for (uint32_t i = 0; i < UIOX_MAC_LABEL_LEN - 1u && name[i]; i++)
        t->name[i] = name[i];
    t->id     = ctx->type_count;
    t->active = true;
    *out_id   = t->id;
    ctx->type_count++;
    return UIOX_SEC_OK;
}

/* =========================================================================
 * Lookup type
 * ====================================================================== */
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

/* =========================================================================
 * Add rule
 * ====================================================================== */
uiox_sec_err_t uiox_mac_policy_add_rule(uiox_mac_policy_ctx_t *ctx,
                                          const uiox_mac_rule_t *rule)
{
    if (!ctx || !rule)  return UIOX_SEC_ERR_INVAL;
    if (ctx->sealed)    return UIOX_SEC_ERR_READONLY;
    if (ctx->rule_count >= UIOX_MAC_MAX_RULES) return UIOX_SEC_ERR_OVERFLOW;

    /* Merge into existing matching rule if present */
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

    /* Verify hash covers everything after the header */
    const uint8_t *body = (const uint8_t *)blob + sizeof(*hdr);
    size_t body_len     = blob_size - sizeof(*hdr);
    uint8_t actual_hash[UIOX_MAC_HASH_LEN];
    uiox_mac_plat_sha256(body, body_len, actual_hash);

    if (mp_memcmp(actual_hash, hdr->policy_hash, UIOX_MAC_HASH_LEN) != 0) {
        uiox_fw_printf("[mac-policy] Hash mismatch — policy rejected\n");
        return UIOX_SEC_ERR_BADLABEL;
    }

    /* Parse type name table */
    if (hdr->type_table_offset + hdr->type_table_size > blob_size)
        return UIOX_SEC_ERR_BADLABEL;

    const char *names = (const char *)blob + hdr->type_table_offset;
    size_t      npos  = 0u;
    for (uint32_t t = 0; t < hdr->type_count && t < UIOX_MAC_MAX_TYPES; t++) {
        uint32_t dummy_id;
        uiox_mac_policy_add_type(ctx, names + npos, &dummy_id);
        while (npos < hdr->type_table_size && names[npos] != '\0') npos++;
        npos++; /* skip null terminator */
    }

    /* Parse rule array */
    if (hdr->rule_table_offset + hdr->rule_table_size > blob_size)
        return UIOX_SEC_ERR_BADLABEL;

    const uiox_mac_rule_t *rules =
        (const uiox_mac_rule_t *)((const uint8_t *)blob
                                   + hdr->rule_table_offset);
    for (uint32_t r = 0; r < hdr->rule_count &&
                          r < UIOX_MAC_MAX_RULES; r++) {
        uiox_mac_policy_add_rule(ctx, &rules[r]);
    }

    mp_memcpy(ctx->loaded_hash, actual_hash, UIOX_MAC_HASH_LEN);
    uiox_fw_printf("[mac-policy] Loaded: types=%u  rules=%u\n",
                   ctx->type_count, ctx->rule_count);
    return UIOX_SEC_OK;
}

/* =========================================================================
 * Seal
 * ====================================================================== */
uiox_sec_err_t uiox_mac_policy_seal(uiox_mac_policy_ctx_t *ctx)
{
    if (!ctx || !ctx->initialized) return UIOX_SEC_ERR_INVAL;
    ctx->sealed = true;
    uiox_fw_printf("[mac-policy] Sealed: %u types  %u rules  mode=%s\n",
                   ctx->type_count, ctx->rule_count,
                   uiox_mac_mode_str(ctx->mode));
    return UIOX_SEC_OK;
}

/* =========================================================================
 * Set mode
 * ====================================================================== */
uiox_sec_err_t uiox_mac_policy_set_mode(uiox_mac_policy_ctx_t *ctx,
                                          uiox_mac_mode_t        mode)
{
    if (!ctx) return UIOX_SEC_ERR_INVAL;
    /* Only allow decreasing the mode (more permissive) when sealed */
    if (ctx->sealed && mode > ctx->mode) return UIOX_SEC_ERR_READONLY;
    ctx->mode = mode;
    uiox_fw_printf("[mac-policy] Mode changed to %s\n",
                   uiox_mac_mode_str(mode));
    return UIOX_SEC_OK;
}

/* =========================================================================
 * Print
 * ====================================================================== */
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
