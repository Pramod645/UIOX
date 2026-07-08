/**
 * @file  uiox_jrnl_types.h
 * @brief UIOX Filesystem Journaling — base types, error codes,
 *        on-disk structures, and magic numbers.
 *
 * Journal design: write-ahead log (WAL), ordered mode by default,
 * writeback and data-journaling modes selectable at mount time.
 *
 * On-disk layout (dedicated journal block device or file):
 *   ┌──────────────────────────────────┐
 *   │  uiox_jrnl_sb_t   (block 0)     │  ← journal superblock
 *   │  Descriptor blocks  (variable)  │  ← one per transaction
 *   │  Data / metadata blocks         │  ← journaled content
 *   │  Commit blocks                  │  ← transaction seal
 *   │  Revoke blocks                  │  ← revocation records
 *   └──────────────────────────────────┘
 *
 * Integrates with:
 *   32_FileSystem/VirtualFileSystem.h  — superblock, inode, dentry
 *   31_BufferCache                     — buffer_head I/O
 *   40_SystemCallInterface             — sys_journal_status()
 *   33_ProcessControlSubsystem        — journal_commit_thread wakeup
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
/**
 * @file  uiox_jrnl_types.h
 * @brief UIOX Filesystem Journaling — base types, error codes, magic numbers.
 *
 * Implements a write-ahead log (WAL) journal compatible with the UIOX VFS
 * layer (32_FileSystem/VirtualFileSystem.h). Modelled on ext3/ext4 journaling
 * semantics: metadata-only journaling by default, full data journaling opt-in.
 *
 * Integrates with:
 *   32_FileSystem/VirtualFileSystem.h  — superblock, inode, dentry objects
 *   31_BufferCache                     — buffer_head / block I/O
 *   33_ProcessControlSubsystem         — transaction commit on context switch
 *   40_SystemCallInterface             — sys_sync(), sys_fsync()
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
#ifndef UIOX_JRNL_TYPES_H
#define UIOX_JRNL_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Error codes
 * ====================================================================== */
typedef enum {
    UIOX_JR_OK               =  0,
    UIOX_JR_ERR_INVAL        = -1,
    UIOX_JR_ERR_NOMEM        = -2,
    UIOX_JR_ERR_IO           = -3,
    UIOX_JR_ERR_BADMAGIC     = -4,
    UIOX_JR_ERR_BADVERSION   = -5,
    UIOX_JR_ERR_CORRUPT      = -6,  /**< Journal block corrupt / bad CRC   */
    UIOX_JR_ERR_FULL         = -7,  /**< Journal log area full             */
    UIOX_JR_ERR_ABORT        = -8,  /**< Journal aborted (I/O error seen)  */
    UIOX_JR_ERR_NOTFOUND     = -9,  /**< Block not found in log            */
    UIOX_JR_ERR_ALREADY      = -10, /**< Transaction already open          */
    UIOX_JR_ERR_NOTOPEN      = -11, /**< No open transaction               */
    UIOX_JR_ERR_OVERFLOW     = -12,
    UIOX_JR_ERR_CHECKSUM     = -13,
} uiox_jr_err_t;

/* =========================================================================
 * Magic numbers and constants
 * ====================================================================== */
#define UIOX_JR_SUPER_MAGIC      0x554A5253u  /**< "UJRS" Journal Superblock  */
#define UIOX_JR_DESC_MAGIC       0x554A4442u  /**< "UJDB" Descriptor block    */
#define UIOX_JR_COMMIT_MAGIC     0x554A434Du  /**< "UJCM" Commit block        */
#define UIOX_JR_REVOKE_MAGIC     0x554A5256u  /**< "UJRV" Revoke block        */
#define UIOX_JR_FORMAT_VERSION   1u

/* Block sizes */
#define UIOX_JR_BLOCK_SIZE       4096u        /**< Journal block size (bytes) */
#define UIOX_JR_SUPER_SIZE       UIOX_JR_BLOCK_SIZE
#define UIOX_JR_CRC32_POLY       0xEDB88320u

/* Transaction / log limits */
#define UIOX_JR_MAX_HANDLES      64u   /**< Max concurrent handles/tx       */
#define UIOX_JR_MAX_BLOCKS_PER_TX 256u /**< Max blocks in one transaction   */
#define UIOX_JR_MAX_REVOKE       128u  /**< Max revoke entries per tx       */
#define UIOX_JR_MIN_LOG_BLOCKS   1024u /**< Minimum journal size in blocks  */

/* =========================================================================
 * Journaling mode
 * ====================================================================== */
typedef enum {
    UIOX_JR_MODE_METADATA  = 0,  /**< Journal metadata only (default)      */
    UIOX_JR_MODE_ORDERED   = 1,  /**< Data written before metadata commit   */
    UIOX_JR_MODE_DATA      = 2,  /**< Full data journaling                  */
} uiox_jr_mode_t;

/* =========================================================================
 * Transaction state machine
 *
 *  INACTIVE → RUNNING → LOCKED → FLUSH → COMMIT → CHECKPOINT → INACTIVE
 * ====================================================================== */
typedef enum {
    UIOX_JR_TX_INACTIVE    = 0,  /**< No transaction open                   */
    UIOX_JR_TX_RUNNING     = 1,  /**< Accepting new block writes            */
    UIOX_JR_TX_LOCKED      = 2,  /**< No new writes; preparing commit       */
    UIOX_JR_TX_FLUSH       = 3,  /**< Writing data blocks to disk           */
    UIOX_JR_TX_COMMIT      = 4,  /**< Writing commit record                 */
    UIOX_JR_TX_CHECKPOINT  = 5,  /**< Checkpointing committed blocks        */
} uiox_jr_tx_state_t;

/* =========================================================================
 * On-disk journal superblock (one per journal, at block 0)
 * ====================================================================== */
typedef struct __attribute__((packed)) {
    uint32_t  magic;              /**< UIOX_JR_SUPER_MAGIC                  */
    uint32_t  format_version;     /**< UIOX_JR_FORMAT_VERSION               */
    uint32_t  block_size;         /**< Journal block size                   */
    uint32_t  log_blocks;         /**< Total blocks in log area             */
    uint32_t  log_first;          /**< First block of circular log          */
    uint32_t  sequence;           /**< Next transaction sequence number     */
    uint32_t  log_start;          /**< First valid commit block in log      */
    uint32_t  errno;              /**< Last recorded I/O error              */
    uint32_t  feature_compat;     /**< Compatible feature flags             */
    uint32_t  feature_incompat;   /**< Incompatible feature flags           */
    uint8_t   uuid[16];           /**< Journal UUID                         */
    uint32_t  nr_users;           /**< Number of filesystems using journal  */
    uint32_t  checksum;           /**< CRC32 of this superblock             */
    uint8_t   _pad[UIOX_JR_BLOCK_SIZE - 64u];
} uiox_jr_super_t;

/* =========================================================================
 * On-disk block tag (inside descriptor block — one per logged block)
 * ====================================================================== */
#define UIOX_JR_FLAG_SAME_UUID   (1u << 0)  /**< UUID same as previous tag  */
#define UIOX_JR_FLAG_LAST_TAG    (1u << 1)  /**< Last tag in descriptor     */
#define UIOX_JR_FLAG_ESCAPED     (1u << 2)  /**< Data block starts w/ magic */

typedef struct __attribute__((packed)) {
    uint64_t  blocknr;     /**< Filesystem block number this log block maps */
    uint32_t  flags;       /**< UIOX_JR_FLAG_* bitmask                      */
    uint32_t  checksum;    /**< CRC32 of the corresponding data block       */
} uiox_jr_block_tag_t;

/* =========================================================================
 * On-disk descriptor block header (precedes a run of data blocks in log)
 * ====================================================================== */
typedef struct __attribute__((packed)) {
    uint32_t  magic;       /**< UIOX_JR_DESC_MAGIC                          */
    uint32_t  blocktype;   /**< 1=descriptor, 2=commit, 3=superblock        */
    uint32_t  sequence;    /**< Transaction sequence number                 */
    uint32_t  checksum;
    /* Followed by uiox_jr_block_tag_t[n] up to UIOX_JR_BLOCK_SIZE */
} uiox_jr_desc_hdr_t;

/* =========================================================================
 * On-disk commit block
 * ====================================================================== */
typedef struct __attribute__((packed)) {
    uint32_t  magic;
    uint32_t  blocktype;   /**< 2 = commit                                  */
    uint32_t  sequence;
    uint64_t  commit_time; /**< Unix timestamp of commit                    */
    uint32_t  checksum;    /**< CRC32 over all data blocks in transaction   */
    uint8_t   _pad[UIOX_JR_BLOCK_SIZE - 20u];
} uiox_jr_commit_t;

/* =========================================================================
 * On-disk revoke block (tells recovery to skip re-applying a block)
 * ====================================================================== */
typedef struct __attribute__((packed)) {
    uint32_t  magic;
    uint32_t  blocktype;   /**< 3 = revoke                                  */
    uint32_t  sequence;
    uint32_t  count;       /**< Number of revoked blocknrs that follow      */
    /* Followed by count × uint64_t blocknr */
    uint8_t   _pad[UIOX_JR_BLOCK_SIZE - 16u];
} uiox_jr_revoke_hdr_t;

/* =========================================================================
 * In-memory logged block entry
 * ====================================================================== */
typedef struct {
    uint64_t  fs_blocknr;      /**< Filesystem block number                */
    uint8_t  *data;            /**< Copy of block data at time of log       */
    uint32_t  checksum;
    bool      escaped;         /**< Block data starts with journal magic    */
    bool      revoked;
} uiox_jr_logged_block_t;

#ifdef __cplusplus
}
#endif
#endif /* UIOX_JRNL_TYPES_H */
