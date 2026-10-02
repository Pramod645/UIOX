/**
 * @file  uiox_kix_sec_mac_policy.h
 * @brief UIOX Security — MAC policy store: type table, rule loading,
 *        and policy binary format.
 *
 * Policy binary layout (loaded from a signed policy file):
 *   ┌─────────────────────────────────────┐
 *   │  uiox_mac_policy_hdr_t  (64 bytes) │  ← magic, version, counts
 *   │  Type name table                    │  ← null-separated strings
 *   │  uiox_mac_rule_t[]                  │  ← allow / audit rules
 *   └─────────────────────────────────────┘
 *
 * The policy binary MUST be verified before loading.
 * uiox_mac_policy_load() computes SHA-256 over the body — everything
 * after the fixed header — and compares it against hdr->policy_hash.
 * The hash function is uiox_mac_plat_sha256(), which delegates to
 * 03_ksign's uiox_ks_sha256().
 *
 * NOTE ON TABLE OFFSETS: type_table_offset and rule_table_offset are
 * ABSOLUTE byte offsets from the start of the blob, while policy_hash
 * covers only the body.  uiox_mac_policy_load() therefore checks both
 * that each table starts inside the blob and that it ENDS inside it.
 *
 * Integrates with:
 *   33_ProcessControlSubsystem/03_ksign — policy hash via uiox_ks_sha256
 *   40_SystemCallInterface              — sys_setlabel(), sys_getlabel()
 *
 * @version 1.1.0
 * @date    2026-10-02
 */
#ifndef UIOX_MAC_POLICY_H
#define UIOX_MAC_POLICY_H

#include "uiox_kix_sec_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Policy binary header
 * ====================================================================== */
#define UIOX_MAC_POLICY_MAGIC    0x554D4143u  /**< "UMAC"                  */
#define UIOX_MAC_POLICY_VERSION  1u

typedef struct __attribute__((packed)) {
    uint32_t  magic;
    uint32_t  version;
    uint32_t  type_count;         /**< Number of security types            */
    uint32_t  rule_count;         /**< Number of allow/audit rules         */
    uint32_t  type_table_offset;  /**< Byte offset to type name table      */
    uint32_t  type_table_size;    /**< Byte size of type name table        */
    uint32_t  rule_table_offset;  /**< Byte offset to rule array           */
    uint32_t  rule_table_size;
    uint8_t   policy_hash[UIOX_MAC_HASH_LEN]; /**< SHA-256 of body        */
    uint8_t   _pad[64u - 4u*8u - UIOX_MAC_HASH_LEN];
} uiox_mac_policy_hdr_t;

/* =========================================================================
 * Type table entry (in-memory after load)
 * ====================================================================== */
typedef struct {
    char      name[UIOX_MAC_LABEL_LEN];
    uint32_t  id;
    bool      active;
} uiox_mac_type_entry_t;

/* =========================================================================
 * Policy context (one per system)
 * ====================================================================== */
typedef struct {
    uiox_mac_mode_t       mode;
    uiox_mac_type_entry_t types[UIOX_MAC_MAX_TYPES];
    uint32_t              type_count;
    uiox_mac_rule_t       rules[UIOX_MAC_MAX_RULES];
    uint32_t              rule_count;
    uint8_t               loaded_hash[UIOX_MAC_HASH_LEN];
    bool                  sealed;     /**< Policy immutable after sealing  */
    bool                  initialized;

    /* Audit log */
    uiox_mac_audit_entry_t audit_log[UIOX_MAC_AUDIT_LOG_SIZE];
    uint32_t               audit_head;
    uint32_t               audit_count;
} uiox_mac_policy_ctx_t;

/* =========================================================================
 * Policy API
 * ====================================================================== */

/** Initialise an empty policy context in @mode. */
uiox_sec_err_t uiox_mac_policy_init(uiox_mac_policy_ctx_t *ctx,
                                     uiox_mac_mode_t        mode);

/**
 * @brief Load and validate a binary policy blob.
 *        Verifies magic, version, and SHA-256 hash before installing rules.
 * @param blob      Pointer to policy binary in memory.
 * @param blob_size Size of policy binary in bytes.
 */
uiox_sec_err_t uiox_mac_policy_load(uiox_mac_policy_ctx_t *ctx,
                                     const void            *blob,
                                     size_t                 blob_size);

/**
 * @brief Add a single allow rule programmatically (boot-time / test only).
 *        Rejected once the policy is sealed.
 */
uiox_sec_err_t uiox_mac_policy_add_rule(uiox_mac_policy_ctx_t *ctx,
                                         const uiox_mac_rule_t *rule);

/**
 * @brief Register a security type by name.
 *        Returns the assigned type_id via @out_id.
 *
 * CONTRACT: @name must be NUL-terminated within UIOX_MAC_LABEL_LEN bytes.
 * The comparison against existing types reads up to that bound, so a
 * caller passing a shorter unterminated buffer reads past its end.
 * Callers holding an arbitrary-length string should bound it first.
 */
uiox_sec_err_t uiox_mac_policy_add_type(uiox_mac_policy_ctx_t *ctx,
                                         const char            *name,
                                         uint32_t              *out_id);

/**
 * @brief Look up a type_id by name.
 * @return UIOX_SEC_OK on success, UIOX_SEC_ERR_NOTFOUND if unknown.
 *         Same @name contract as uiox_mac_policy_add_type().
 */
uiox_sec_err_t uiox_mac_policy_lookup_type(const uiox_mac_policy_ctx_t *ctx,
                                            const char                  *name,
                                            uint32_t                    *out_id);

/**
 * @brief Seal the policy — prevents further modifications.
 *        Called once the filesystem is mounted and the policy is verified.
 */
uiox_sec_err_t uiox_mac_policy_seal(uiox_mac_policy_ctx_t *ctx);

/** Set enforcement mode (can only decrease: ENFORCING → PERMISSIVE → DISABLED). */
uiox_sec_err_t uiox_mac_policy_set_mode(uiox_mac_policy_ctx_t *ctx,
                                          uiox_mac_mode_t        mode);

/** Print policy summary: mode, type count, rule count. */
void uiox_mac_policy_print(const uiox_mac_policy_ctx_t *ctx);

/* =========================================================================
 * Platform hash hook — implement in BSP, default delegates to 03_ksign
 * ====================================================================== */

/** Compute UIOX_MAC_HASH_LEN bytes of SHA-256 over @data.
 *  Weak: override with a hardware-accelerated implementation.  Must run
 *  only where the node will hash large buffers for performance reasons.
 *
 *  Build note: the default calls uiox_ks_sha256() from 03_ksign, so any
 *  image linking libsec.a must also link libksign.a. */
__attribute__((weak))
void uiox_mac_plat_sha256(const uint8_t *data, size_t len,
                           uint8_t digest[UIOX_MAC_HASH_LEN]);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_MAC_POLICY_H */
