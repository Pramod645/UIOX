Now I have the full picture of the existing filesystem code style. The existing files use descriptive comment-heavy headers — but the 12_ksign and 13_fboot modules established the proper production C style for new modules. I'll follow that same pattern: uiox_ prefix, typedef enum/typedef struct, no-libc helpers, __attribute__((weak)) platform hooks, and extern void uiox_fw_printf().

The journaling module slots into 32_FileSystem/ as a new 13_journal/ subdirectory.
==========
32_FileSystem/13_journal/ — Complete File Set (9 files)
========
32_FileSystem/13_journal/
├── include/
│   ├── uiox_jrnl_types.h     # Error codes, magic numbers, on-disk structs,
│   │                         #   txn state machine, buf/handle/txn types
│   ├── uiox_jrnl_io.h        # Block I/O abstraction + platform hooks
│   ├── uiox_jrnl_txn.h       # Transaction/handle/buffer lifecycle API
│   ├── uiox_jrnl_recover.h   # Crash recovery: scan → revoke → replay
│   └── uiox_jrnl.h           # Master include, lifecycle API, syscalls
└── src/
    ├── uiox_jrnl_io.c        # CRC-32, SHA-256, block read/write stubs
    ├── uiox_jrnl_txn.c       # Begin/commit/checkpoint/abort +
========
32_FileSystem/13_jrnl/
├── include/
│   ├── uiox_jrnl_types.h      # Error codes, magic numbers, on-disk structs
│   │                          #   (superblock, descriptor, commit, revoke,
│   │                          #    block tag, logged block entry)
│   ├── uiox_jrnl_tx.h         # Handle + transaction types and API
│   ├── uiox_jrnl_recovery.h   # Replay map, recovery stats, recovery API
│   └── uiox_jrnl.h            # Master include — lifecycle, VFS hooks,
│                              #   syscalls, diagnostics
└── src/
    ├── uiox_jrnl_tx.c         # Handle pool, get_write_access, dirty_metadata,
    │                          #   revoke, stop, is_aborted
    ├── uiox_jrnl_recovery.c   # Scan (circular log walk) + replay engine
    └── uiox_jrnl.c            # Init, mount, unmount, abort, tick,
                               #   checkpoint, commit pipeline, VFS hooks,
                               #   syscall handlers, diagnostics
================================
VFS write path (32_FileSystem)
    │
    ├─ uiox_jr_vfs_get_write_access()   ← before modifying inode/dir block
    ├─ uiox_jr_vfs_dirty_metadata()     ← after modifying
    └─ uiox_jr_vfs_revoke()             ← on truncate / unlink

Scheduler tick (33_ProcessControlSubsystem)
    └─ uiox_jr_tick()                   ← commits if interval elapsed (5 s)

Mount / unmount path
    ├─ uiox_jr_mount()  → uiox_jr_recover()   ← scan + replay on dirty mount
    └─ uiox_jr_unmount() → force_commit + checkpoint + clean superblock

Syscall table (40_SystemCallInterface)
    ├─ SYS_SYNC      (162) → uiox_jr_force_commit + checkpoint
    ├─ SYS_FSYNC      (74) → uiox_jr_force_commit
    ├─ SYS_FDATASYNC  (75) → uiox_jr_force_commit (data only)
    └─ SYS_SYNCFS    (306) → same as SYS_SYNC
=========================
Transaction state machine
INACTIVE
   │  uiox_jr_start()
   ▼
RUNNING  ← get_write_access / dirty_metadata / revoke
   │  all handles stopped / log full / tick
   ▼
LOCKED
   │  descriptor block written
   ▼
FLUSH    ← data blocks written to log
   │
   ▼
COMMIT   ← commit block written → fsync guarantee met
   │
   ▼
CHECKPOINT ← blocks written back to home location
   │
   ▼
INACTIVE
