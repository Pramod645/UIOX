#include "../include/init.h"
#include "../include/fork.h"
#include "../include/exec.h"
#include "../include/exit_wait.h"
/* string.h/stdio.h/stdlib.h removed — provided via init.h -> uiox_klibc.h */

/* ── Globals ─────────────────────────────────────────────────── */
kern_init_state_t kern_init        = {0};
run_level_t       current_runlevel = RUNLEVEL_MULTI;

/* ── Simple freestanding atoi replacement ────────────────────── */
static int uiox_atoi(const char *s)
{
    int n = 0, neg = 0;
    if (!s) return 0;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') { n = n * 10 + (*s - '0'); s++; }
    return neg ? -n : n;
}

/* ── Simple freestanding strsep replacement ──────────────────── */
static char *uiox_strsep(char **sp, const char *delim)
{
    char *s, *tok;
    const char *d;
    if (!sp || !*sp) return (char *)0;
    tok = *sp;
    for (s = *sp; *s; s++) {
        for (d = delim; *d; d++) {
            if (*s == *d) {
                *s   = '\0';
                *sp  = s + 1;
                return tok;
            }
        }
    }
    *sp = (char *)0;
    return tok;
}

/* ── init_kernel_data_structures ────────────────────────────── */
void init_kernel_data_structures(void)
{
    printf("[start] initializing kernel data structures\n");
    kern_init.ki_buf_initialized    = 1;
    kern_init.ki_inode_initialized  = 1;
    kern_init.ki_proc_initialized   = 1;
    kern_init.ki_region_initialized = 1;
    kern_init.ki_mount_initialized  = 1;
    kern_init.ki_pgtbl_initialized  = 1;
    kern_init.ki_intr_initialized   = 1;
    printf("[start] all kernel structures ready\n");
}

/* ── pseudo_mount_root ───────────────────────────────────────── */
void pseudo_mount_root(void)
{
    printf("[start] pseudo-mounting root file system\n");
    printf("[start] root file system mounted\n");
}

/* ── handcraft_proc0 ─────────────────────────────────────────── */
void handcraft_proc0(void)
{
    printf("[start] hand-crafting process 0 (swapper)\n");
}

/* ── spawn_kernel_processes ──────────────────────────────────── */
void spawn_kernel_processes(void)
{
    printf("[start] spawning kernel processes\n");
}

/* ── kernel_init_process ─────────────────────────────────────── */
void kernel_init_process(void)
{
    char *argv[] = { "/etc/init", (char *)0 };
    char *envp[] = { "HOME=/", "PATH=/bin:/sbin", (char *)0 };
    printf("[init-bootstrap] process 1: exec /etc/init\n");
    kernel_exec("/etc/init", argv, envp);
}

/* ── parse_inittab ───────────────────────────────────────────────
 * File I/O (FILE/fopen/fgets/fclose) removed — no filesystem in
 * freestanding kernel at boot time.
 * Replaced with a static built-in inittab for simulation.
 * strsep/atoi replaced with freestanding equivalents above.
 * ────────────────────────────────────────────────────────────── */
int parse_inittab(const char *path,
                  inittab_entry_t *entries,
                  int max_entries)
{
    /* Static built-in inittab — format: id, runlevel, action, cmd */
    static const struct {
        const char    *id;
        run_level_t    rl;
        init_action_t  action;
        const char    *cmd;
    } builtin[] = {
        { "si",   RUNLEVEL_MULTI, INIT_SYSINIT,  "/etc/init.d/rcS"    },
        { "rc",   RUNLEVEL_MULTI, INIT_WAIT,      "/etc/init.d/rc 2"  },
        { "1",    RUNLEVEL_MULTI, INIT_RESPAWN,   "/sbin/getty tty1"  },
        { "2",    RUNLEVEL_MULTI, INIT_RESPAWN,   "/sbin/getty tty2"  },
    };
    int i, count = 0;
    int n = (int)(sizeof builtin / sizeof builtin[0]);

    (void)path;  /* no filesystem in freestanding boot */

    if (!entries || max_entries <= 0) return 0;

    for (i = 0; i < n && count < max_entries; i++) {
        inittab_entry_t *e = &entries[count];
        memset(e, 0, sizeof *e);
        strncpy(e->ie_id,  builtin[i].id,  sizeof(e->ie_id)  - 1);
        strncpy(e->ie_cmd, builtin[i].cmd, sizeof(e->ie_cmd) - 1);
        e->ie_runlevel = builtin[i].rl;
        e->ie_action   = builtin[i].action;
        e->ie_active   = 1;
        count++;
    }

    /* Keep uiox_atoi and uiox_strsep in the translation unit to
       avoid unused-function warnings — they are available for any
       future dynamic parsing path.                               */
    (void)uiox_atoi;
    (void)uiox_strsep;

    printf("[init] loaded %d built-in inittab entries\n", count);
    return count;
}

/* ── kernel_init ─────────────────────────────────────────────── */
void kernel_init(void)
{
    inittab_entry_t entries[MAX_INITTAB_ENTRIES];
    int n_entries, i;

    printf("[init] init process (pid=1) running\n");
    n_entries = parse_inittab(INITTAB_PATH, entries, MAX_INITTAB_ENTRIES);

    for (i = 0; i < n_entries; i++) {
        inittab_entry_t *e = &entries[i];
        if (e->ie_runlevel != current_runlevel &&
            e->ie_runlevel != 0) continue;

        printf("[init] spawning: %s\n", e->ie_cmd);
        {
            int pid = kernel_fork();
            if (pid == 0) {
                char *argv[] = { e->ie_cmd, (char *)0 };
                kernel_exec(e->ie_cmd, argv, (char *const *)0);
                kernel_exit(1);
            } else if (pid > 0) {
                e->ie_child_pid = (uint32_t)pid;
                if (e->ie_action == INIT_WAIT ||
                    e->ie_action == INIT_BOOTWAIT) {
                    int status;
                    kernel_wait(&status);
                }
            }
        }
    }

    /* Monitor loop */
    for (;;) {
        int status;
        int id = kernel_wait(&status);
        if (id == -1) break;
        for (i = 0; i < n_entries; i++) {
            if (entries[i].ie_child_pid == (uint32_t)id) {
                printf("[init] child pid=%d (%s) died status=%d\n",
                       id, entries[i].ie_cmd, status);
                if (entries[i].ie_action == INIT_RESPAWN) {
                    char *argv[] = { entries[i].ie_cmd, (char *)0 };
                    int npid = kernel_fork();
                    if (npid == 0) {
                        kernel_exec(entries[i].ie_cmd, argv, (char *const *)0);
                        kernel_exit(1);
                    } else if (npid > 0) {
                        entries[i].ie_child_pid = (uint32_t)npid;
                    }
                }
            }
        }
    }
}

/* ── swapper_loop ────────────────────────────────────────────── */
int swapper_loop(void)
{
    printf("[swapper] process 0 entering swapper loop\n");
    for (;;) {
        printf("[swapper] checking swap queues\n");
        /* proc_sleep(SWAP_IN_EVENT, PSWP, 0); */
        return 0;  /* simulation: return after one iteration */
    }
}

/* ── kernel_start ────────────────────────────────────────────── */
void kernel_start(void)
{
    int pid;
    printf("[start] UIOX kernel starting\n");
    init_kernel_data_structures();
    pseudo_mount_root();
    handcraft_proc0();

    printf("[start] forking process 1 (init)\n");
    pid = kernel_fork();
    if (pid == 0) {
        kernel_init_process();
        kernel_exit(1);
    } else {
        printf("[start] process 0: init pid=%d forked\n", pid);
        spawn_kernel_processes();
        printf("[start] process 0 becoming swapper\n");
        swapper_loop();
    }
}
