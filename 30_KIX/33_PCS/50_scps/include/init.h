#ifndef INIT_H
#define INIT_H
#include "uiox_klibc.h"

#define INITTAB_PATH          "/etc/inittab"
#undef  INIT_PID
#define INIT_PID              1
#undef  SWAPPER_PID
#define SWAPPER_PID           0
#define MAX_INITTAB_ENTRIES   64
#define MAX_CMD_LEN           256

typedef enum run_level {
    RUNLEVEL_HALT      = 0,
    RUNLEVEL_SINGLE    = 1,
    RUNLEVEL_MULTI     = 2,
    RUNLEVEL_NETWORK   = 3,
    RUNLEVEL_GRAPHICAL = 5,
    RUNLEVEL_REBOOT    = 6
} run_level_t;

typedef enum init_action {
    INIT_RESPAWN  = 1,
    INIT_WAIT     = 2,
    INIT_ONCE     = 3,
    INIT_BOOT     = 4,
    INIT_BOOTWAIT = 5,
    INIT_OFF      = 6,
    INIT_SYSINIT  = 7
} init_action_t;

typedef struct inittab_entry {
    char          ie_id[8];
    run_level_t   ie_runlevel;
    init_action_t ie_action;
    char          ie_cmd[MAX_CMD_LEN];
    uint32_t      ie_child_pid;
    int           ie_active;
} inittab_entry_t;

typedef struct kern_init_state {
    int ki_buf_initialized;
    int ki_inode_initialized;
    int ki_proc_initialized;
    int ki_region_initialized;
    int ki_mount_initialized;
    int ki_pgtbl_initialized;
    int ki_intr_initialized;
} kern_init_state_t;

extern kern_init_state_t kern_init;
extern run_level_t       current_runlevel;

typedef enum proc_class {
    PCLASS_USER   = 1,
    PCLASS_DAEMON = 2,
    PCLASS_KERNEL = 3
} proc_class_t;

void kernel_start              (void);
void init_kernel_data_structures(void);
void pseudo_mount_root         (void);
void handcraft_proc0           (void);
void kernel_init_process       (void);
int  swapper_loop              (void);
void spawn_kernel_processes    (void);
int  parse_inittab             (const char *path,
                                inittab_entry_t *entries,
                                int max_entries);
void kernel_init               (void);

#endif
