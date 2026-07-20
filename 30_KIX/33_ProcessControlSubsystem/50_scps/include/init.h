#ifndef INIT_H
#define INIT_H

#include <stdint.h>

/* ── Init process constants ─────────────────────────────────── */
#define INITTAB_PATH    "/etc/inittab"
#define INIT_PID        1
#define SWAPPER_PID     0

/* ── Run levels ─────────────────────────────────────────────── */
typedef enum run_level {
    RUNLEVEL_HALT    = 0,   /* system halted                    */
    RUNLEVEL_SINGLE  = 1,   /* single-user mode                 */
    RUNLEVEL_MULTI   = 2,   /* multi-user mode                  */
    RUNLEVEL_NETWORK = 3,   /* multi-user with network          */
    RUNLEVEL_GRAPHICAL = 5, /* graphical                        */
    RUNLEVEL_REBOOT  = 6    /* reboot                           */
} run_level_t;

/* ── Inittab entry ──────────────────────────────────────────── */
#define MAX_INITTAB_ENTRIES 64
#define MAX_CMD_LEN         256

typedef enum init_action {
    INIT_RESPAWN    = 1,    /* respawn if process dies          */
    INIT_WAIT       = 2,    /* run once and wait                */
    INIT_ONCE       = 3,    /* run once, don't wait             */
    INIT_BOOT       = 4,    /* run during boot                  */
    INIT_BOOTWAIT   = 5,    /* run during boot, wait            */
    INIT_OFF        = 6,    /* do nothing                       */
    INIT_SYSINIT    = 7     /* run before everything else       */
} init_action_t;

typedef struct inittab_entry {
    char          ie_id[8];         /* entry identifier         */
    run_level_t   ie_runlevel;      /* applicable run level     */
    init_action_t ie_action;        /* what to do               */
    char          ie_cmd[MAX_CMD_LEN]; /* command to execute     */
    uint32_t      ie_child_pid;     /* PID if spawned           */
    int           ie_active;        /* entry is in use          */
} inittab_entry_t;

/* ── Kernel data structures initialized at boot ─────────────── */
typedef struct kern_init_state {
    int ki_buf_initialized;     /* buffer cache ready           */
    int ki_inode_initialized;   /* inode table ready            */
    int ki_proc_initialized;    /* process table ready          */
    int ki_region_initialized;  /* region table ready           */
    int ki_mount_initialized;   /* mount table ready            */
    int ki_pgtbl_initialized;   /* page tables ready            */
    int ki_intr_initialized;    /* interrupt vectors ready      */
} kern_init_state_t;

extern kern_init_state_t kern_init;
extern run_level_t       current_runlevel;

/* ── Process type classification ────────────────────────────── */
typedef enum proc_class {
    PCLASS_USER     = 1,    /* user process (terminal assoc.)  */
    PCLASS_DAEMON   = 2,    /* daemon (no terminal, user mode) */
    PCLASS_KERNEL   = 3     /* kernel process (kernel mode)    */
} proc_class_t;

/* ── Function prototypes ────────────────────────────────────── */
void kernel_start(void);
void init_kernel_data_structures(void);
void pseudo_mount_root(void);
void handcraft_proc0(void);
void kernel_init_process(void);
int  swapper_loop(void);
void spawn_kernel_processes(void);
int  parse_inittab(const char *path,
                   inittab_entry_t *entries,
                   int max_entries);

#endif /* INIT_H */
