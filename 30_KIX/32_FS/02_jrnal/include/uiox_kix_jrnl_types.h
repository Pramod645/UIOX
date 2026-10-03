/**
 * @file  uiox_kix_jrnl_types.h
 * @brief UIOX Filesystem Journaling — base types, error codes,
 *        on-disk structures, and magic numbers.
 *
 * ── RENAMED ───────────────────────────────────────────────────────────
 * Was uiox_jrnl_types.h.  The module now carries the kix prefix its
 * siblings use.
 *
 * ── CHANGED: THE LOG NOW OWNS ITS DATA ────────────────────────────────
 * uiox_jr_logged_block_t.data was a POINTER holding the caller's buffer
 * address.  bwrite() releases that buffer, so the commit read a page that
 * had already been handed to another claimant.  data_index replaces it.
 *
 * @version 1.1.0
 * @date    2026-10-03
 */
#ifndef UIOX_KIX_JRNL_TYPES_H
#define UIOX_KIX_JRNL_TYPES_H

#ifndef UIOX_BASETYPES_COMPAT
#  define UIOX_BASETYPES_COMPAT
#endif
#include "uiox_base_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/* ── 02_jrnal/include/uiox_kix_jrnl_types.h ───────────────────────── */

/* the context is opaque everywhere outside uiox_kix_jrnl.c */
typedef struct uiox_jr_ctx uiox_jr_ctx_t;

/* what recovery reports back — filled by uiox_jr_mount() and read by
 * uiox_fs_init() */
typedef struct uiox_jr_recovery_stats {
     uint32_t txns_found;
     uint32_t txns_partial;
     uint32_t blocks_replayed;
     uint32_t blocks_revoked;
     uint32_t corrupt_blocks;
     bool     clean;
} uiox_jr_recovery_stats_t;


typedef enum {
    UIOX_JR_OK               =  0,
    UIOX_JR_ERR_INVAL        = -1,
    UIOX_JR_ERR_NOMEM        = -2,
    UIOX_JR_ERR_IO           = -3,
    UIOX_JR_ERR_BADMAGIC     = -4,
    UIOX_JR_ERR_BADVERSION   = -5,
    UIOX_JR_ERR_CORRUPT      = -6,
    UIOX_JR_ERR_FULL         = -7,
    UIOX_JR_ERR_ABORT        = -8,
    UIOX_JR_ERR_NOTFOUND     = -9,
    UIOX_JR_ERR_ALREADY      = -10,
    UIOX_JR_ERR_NOTOPEN      = -11,
    UIOX_JR_ERR_OVERFLOW     = -12,
    UIOX_JR_ERR_CHECKSUM     = -13,
} uiox_jr_err_t;

#define UIOX_JR_SUPER_MAGIC      0x554A5253u
#define UIOX_JR_DESC_MAGIC       0x554A4442u
#define UIOX_JR_COMMIT_MAGIC     0x554A434Du
#define UIOX_JR_REVOKE_MAGIC     0x554A5256u
#define UIOX_JR_FORMAT_VERSION   1u

#define UIOX_JR_BLOCK_SIZE       4096u
#define UIOX_JR_SUPER_SIZE       UIOX_JR_BLOCK_SIZE
#define UIOX_JR_CRC32_POLY       0xEDB88320u

#define UIOX_JR_MAX_HANDLES      64u
#define UIOX_JR_MAX_BLOCKS_PER_TX 256u
#define UIOX_JR_MAX_REVOKE       128u
#define UIOX_JR_MIN_LOG_BLOCKS   1024u

#define UIOX_JR_LOG_POOL_BLOCKS   UIOX_JR_MAX_BLOCKS_PER_TX
#define UIOX_JR_LOG_POOL_SIZE     (UIOX_JR_LOG_POOL_BLOCKS * UIOX_JR_BLOCK_SIZE)

#define UIOX_JR_POOL_SLOT_NONE    0xFFFFu

typedef enum {
    UIOX_JR_MODE_METADATA  = 0,
    UIOX_JR_MODE_ORDERED   = 1,
    UIOX_JR_MODE_DATA      = 2,
} uiox_jr_mode_t;

typedef enum {
    UIOX_JR_TX_INACTIVE    = 0,
    UIOX_JR_TX_RUNNING     = 1,
    UIOX_JR_TX_LOCKED      = 2,
    UIOX_JR_TX_FLUSH       = 3,
    UIOX_JR_TX_COMMIT      = 4,
    UIOX_JR_TX_CHECKPOINT  = 5,
} uiox_jr_tx_state_t;

typedef struct __attribute__((packed)) {
    uint32_t  magic;
    uint32_t  format_version;
    uint32_t  block_size;
    uint32_t  log_blocks;
    uint32_t  log_first;
    uint32_t  sequence;
    uint32_t  log_start;
    uint32_t  errno;
    uint32_t  feature_compat;
    uint32_t  feature_incompat;
    uint8_t   uuid[16];
    uint32_t  nr_users;
    uint32_t  checksum;
    uint8_t   _pad[UIOX_JR_BLOCK_SIZE - 64u];
} uiox_jr_super_t;

#define UIOX_JR_FLAG_SAME_UUID   (1u << 0)
#define UIOX_JR_FLAG_LAST_TAG    (1u << 1)
#define UIOX_JR_FLAG_ESCAPED     (1u << 2)

typedef struct __attribute__((packed)) {
    uint64_t  blocknr;
    uint32_t  flags;
    uint32_t  checksum;
} uiox_jr_block_tag_t;

typedef struct __attribute__((packed)) {
    uint32_t  magic;
    uint32_t  blocktype;
    uint32_t  sequence;
    uint32_t  checksum;
} uiox_jr_desc_hdr_t;

typedef struct __attribute__((packed)) {
    uint32_t  magic;
    uint32_t  blocktype;
    uint32_t  sequence;
    uint64_t  commit_time;
    uint32_t  checksum;
    uint8_t   _pad[UIOX_JR_BLOCK_SIZE - 24u];
} uiox_jr_commit_t;

typedef struct __attribute__((packed)) {
    uint32_t  magic;
    uint32_t  blocktype;
    uint32_t  sequence;
    uint32_t  count;
    uint8_t   _pad[UIOX_JR_BLOCK_SIZE - 16u];
} uiox_jr_revoke_hdr_t;

typedef struct {
    uint64_t  fs_blocknr;
    uint16_t  data_index;
    uint16_t  _pad;
    uint32_t  checksum;
    bool      escaped;
    bool      revoked;
} uiox_jr_logged_block_t;

#ifdef __cplusplus
}
#endif
#endif /* UIOX_KIX_JRNL_TYPES_H */
