/*
 * 30_KIX/32_FS/02_journal/uiox_journal.h(New)
 *
 * UIOX Filesystem Journal — header.
 *
 * Transaction state machine:
 *   INACTIVE → RUNNING → COMMIT → CHECKPOINT → INACTIVE
 *
 * Syscall integration:
 *   SYS_SYNC    (162) → uiox_jr_force_commit() + checkpoint
 *   SYS_FSYNC    (74) → uiox_jr_force_commit()
 *   SYS_FDATASYNC(75) → uiox_jr_force_commit() (data only)
 *   SYS_SYNCFS  (306) → same as SYS_SYNC
 *
 * @version 1.0.0  @date 2026-07-29
 */

 #ifndef UIOX_JOURNAL_H
 #define UIOX_JOURNAL_H
 
 #include "uiox_base_types.h"
 
 /* ── Limits ────────────────────────────────────────────────────────── */
 #define UIOX_JR_BLOCK_SIZE    4096u
 #define UIOX_JR_MAX_BLOCKS    256u    /* journal log size in blocks    */
 #define UIOX_JR_MAX_HANDLES   32u     /* concurrent transaction handles*/
 
 /* ── Magic numbers ─────────────────────────────────────────────────── */
 #define UIOX_JR_MAGIC_SB      0x6A626434UL   /* "jbd4" superblock     */
 #define UIOX_JR_MAGIC_DESC    0x6A626435UL   /* descriptor block       */
 #define UIOX_JR_MAGIC_COMMIT  0x6A626436UL   /* commit block           */
 
 /* ── Transaction states ────────────────────────────────────────────── */
 typedef enum {
     UIOX_JR_TXN_INACTIVE   = 0,
     UIOX_JR_TXN_RUNNING    = 1,
     UIOX_JR_TXN_LOCKED     = 2,
     UIOX_JR_TXN_FLUSH      = 3,
     UIOX_JR_TXN_COMMIT     = 4,
     UIOX_JR_TXN_CHECKPOINT = 5,
     UIOX_JR_TXN_FINISHED   = 6,
 } uiox_jr_txn_state_t;
 
 /* ── Error codes ───────────────────────────────────────────────────── */
 #define UIOX_JR_OK        0
 #define UIOX_JR_ENOBUFS  -12
 #define UIOX_JR_EINVAL   -22
 #define UIOX_JR_EIO       -5
 #define UIOX_JR_ENOSPC   -28
 
 /* ── On-disk journal superblock ────────────────────────────────────── */
 typedef struct {
     uint32_t  magic;
     uint32_t  block_size;
     uint32_t  max_blocks;
     uint32_t  first_block;      /* first journal log block             */
     uint32_t  sequence;         /* next transaction sequence number    */
     uint32_t  head;             /* oldest committed txn block          */
     uint32_t  tail;             /* next free journal block             */
     uint8_t   clean;            /* 1 = cleanly unmounted               */
     uint8_t   _pad[UIOX_JR_BLOCK_SIZE - 33u];
 } __attribute__((packed)) uiox_jr_sb_disk_t;
 
 /* ── Journal block descriptor entry ───────────────────────────────── */
 typedef struct {
     uint32_t  fs_block;         /* which FS block this journal copy is */
     uint32_t  flags;
 } uiox_jr_block_tag_t;
 
 /* ── In-memory journal handle (one per active transaction) ─────────── */
 typedef struct {
     uint32_t             sequence;
     uiox_jr_txn_state_t  state;
     uint32_t             nr_buffers;    /* buffers dirtied this txn    */
     uint8_t              inuse;
 } uiox_jr_handle_t;
 
 /* ── In-memory journal context ─────────────────────────────────────── */
 typedef struct {
     uiox_jr_sb_disk_t    sb;
     uiox_jr_handle_t     handles[UIOX_JR_MAX_HANDLES];
     uint32_t             head;
     uint32_t             tail;
     uint32_t             free_blocks;
     uint8_t              dirty;         /* uncommitted changes exist   */
     uint8_t              initialised;
 } uiox_jr_ctx_t;
 
 /* ── Public API ────────────────────────────────────────────────────── */
 void  uiox_jr_init(void);
 void  uiox_jr_mount(void);
 void  uiox_jr_unmount(void);
 
 /* Transaction lifecycle */
 uiox_jr_handle_t *uiox_jr_start(uint32_t nr_blocks);
 int               uiox_jr_get_write_access(uiox_jr_handle_t *h,
                                             uint32_t fs_block);
 int               uiox_jr_dirty_metadata  (uiox_jr_handle_t *h,
                                             uint32_t fs_block);
 int               uiox_jr_stop           (uiox_jr_handle_t *h);
 
 /* Commit / sync — called from SYS_SYNC / SYS_FSYNC */
 int  uiox_jr_force_commit(void);
 int  uiox_jr_checkpoint  (void);
 
 /* Scheduler tick — commits if interval elapsed */
 void uiox_jr_tick(void);
 
 /* Recovery — called on dirty mount */
 int  uiox_jr_recover(void);
 
 #endif /* UIOX_JOURNAL_H */
 