#include "../include/init.h"
#include "../include/fork.h"
#include "../include/exec.h"
#include "../include/exit_wait.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ── Globals ────────────────────────────────────────────────── */
kern_init_state_t kern_init       = {0};
run_level_t       current_runlevel = RUNLEVEL_MULTI;

/* ── init_kernel_data_structures ─────────────────────────────
 * Initialize all kernel internal data structures at boot.
 * Constructs free lists, hash queues, tables, page entries.
 */
void init_kernel_data_structures(void)
{
    printf("[start] initializing kernel data structures\n");

    /* Buffer cache free list and hash queues */
    /* buf_init(); */
    kern_init.ki_buf_initialized = 1;
    printf("[start]   buffer cache initialized\n");

    /* Inode table free list and hash queues */
    /* inode_init(); */
    kern_init.ki_inode_initialized = 1;
    printf("[start]   inode table initialized\n");

    /* Process table */
    /* memset(proc_table, 0, sizeof(proc_table)); */
    kern_init.ki_proc_initialized = 1;
    printf("[start]   process table initialized\n");

    /* Region table free list */
    /* region_init(); */
    kern_init.ki_region_initialized = 1;
    printf("[start]   region table initialized\n");

    /* Mount table */
    /* memset(mount_table, 0, sizeof(mount_table)); */
    kern_init.ki_mount_initialized = 1;
    printf("[start]   mount table initialized\n");

    /* Page table entries */
    /* pgtbl_init(); */
    kern_init.ki_pgtbl_initialized = 1;
    printf("[start]   page tables initialized\n");

    /* Interrupt vector table */
    /* intr_init(); */
    kern_init.ki_intr_initialized = 1;
    printf("[start]   interrupt vectors initialized\n");

    printf("[start] all kernel structures ready\n");
}

/* ── pseudo_mount_root ───────────────────────────────────────
 * Mount the root file system onto "/" and set up the
 * environment visible to process 0.
 */
void pseudo_mount_root(void)
{
    printf("[start] pseudo-mounting root file system\n");
    /*
     * fs_mount("/dev/root", "/", 0);
     * root_inode = iget(ROOT_DEV, ROOT_INODE_NUM);
     */
    printf("[start] root file system mounted\n");
}

/* ── handcraft_proc0 ─────────────────────────────────────────
 * Build the environment for process 0 (the swapper) by hand,
 * without a fork(), since there is no parent process yet.
 */
void handcraft_proc0(void)
{
    printf("[start] hand-crafting environment for process 0\n");

    /* Initialise process table slot 0 */
    /* proc_table[0].p_pid   = SWAPPER_PID;               */
    /* proc_table[0].p_state = PROC_KERNEL_RUNNING;        */
    /* proc_table[0].p_uid   = 0;                          */
    /* proc_table[0].p_flag  = P_LOADED;                   */

    /* Create u area for process 0 */
    /* u.u_procp = &proc_table[0];                         */

    /* Make root the current directory of process 0 */
    /* u.u_cdir = root_inode; root_inode->i_count++;       */
    /* u.u_rdir = root_inode; root_inode->i_count++;       */

    /* current_proc = &proc_table[0];                      */
    printf("[start] process 0 (swapper) environment ready\n");
}

/* ── spawn_kernel_processes ──────────────────────────────────
 * Process 0 forks kernel processes such as vhand (page reclaim).
 * These run only in kernel mode and access kernel structures
 * directly without system calls.
 */
void spawn_kernel_processes(void)
{
    printf("[start] spawning kernel processes\n");

    /* Fork vhand (page reclaimer) */
    /* kernel_fork() -> exec vhand kernel thread */
    printf("[start]   vhand (page reclaimer) started\n");

    /* Other kernel processes (bdflush, kswapd, etc.) */
    printf("[start]   kernel processes spawned\n");
}

/* ── kernel_init_process ─────────────────────────────────────
 * Code that runs as process 1 (init) after process 0 forks it.
 * This code runs in user mode and execs /etc/init.
 */
void kernel_init_process(void)
{
    printf("[init-bootstrap] process 1 starting\n");

    /* Allocate a region for init's address space */
    /* rp = allocreg(NULL, REG_DATA);              */

    /* Attach region to init's address space */
    /* prp = attachreg(rp, current_proc,
     *                 INIT_LOAD_VADDR, REG_DATA); */

    /* Grow region to accommodate init bootstrap code */
    /* growreg(prp, INIT_CODE_SIZE);                  */

    /* Copy bootstrap code from kernel space into init's
     * user address space                              */
    /* memcpy((void*)INIT_LOAD_VADDR,
     *         kernel_init_code, INIT_CODE_SIZE);      */

    printf("[init-bootstrap] changing mode: "
           "kernel -> user (exec /etc/init)\n");

    /*
     * Change mode: return from kernel to user mode.
     * As a result, init execs /etc/init and becomes a normal
     * user process with respect to system call invocation.
     * Init never returns to this point.
     */
    char *argv[] = { "/etc/init", NULL };
    char *envp[] = { "HOME=/", "PATH=/bin:/sbin", NULL };
    kernel_exec("/etc/init", argv, envp);
}

/* ── parse_inittab ───────────────────────────────────────────
 * Read and parse /etc/inittab into the entries array.
 * Returns the number of entries read.
 */
int parse_inittab(const char *path,
                  inittab_entry_t *entries,
                  int max_entries)
{
    FILE *fd = fopen(path, "r");
    if (!fd) {
        fprintf(stderr, "[init] cannot open %s\n", path);
        return 0;
    }

    char buf[512];
    int  count = 0;

    while (fgets(buf, sizeof(buf), fd) && count < max_entries) {
        /* Skip comments and blank lines */
        if (buf[0] == '#' || buf[0] == '\n') continue;

        inittab_entry_t *e = &entries[count];
        memset(e, 0, sizeof(*e));

        /* Format: id:runlevel:action:command */
        char *p = buf;
        char *tok;

        tok = strsep(&p, ":");
        if (tok) strncpy(e->ie_id, tok, sizeof(e->ie_id) - 1);

        tok = strsep(&p, ":");
        if (tok) e->ie_runlevel = (run_level_t)atoi(tok);

        tok = strsep(&p, ":");
        if (tok) e->ie_action = (init_action_t)atoi(tok);

        tok = strsep(&p, "\n");
        if (tok) strncpy(e->ie_cmd, tok, sizeof(e->ie_cmd) - 1);

        e->ie_active    = 1;
        e->ie_child_pid = 0;
        count++;
    }

    fclose(fd);
    printf("[init] parsed %d entries from %s\n", count, path);
    return count;
}

/* ─────────────────────────────────────────────────────────────
 * 12. Algorithm init
 *     Process 1 — the init process.
 *     Reads /etc/inittab, spawns processes for the current
 *     run level, and monitors them.
 */
void kernel_init(void)
{
    inittab_entry_t entries[MAX_INITTAB_ENTRIES];
    int n_entries = 0;

    printf("[init] init process (pid=1) running\n");

    /* Open /etc/inittab for reading */
    n_entries = parse_inittab(INITTAB_PATH, entries,
                              MAX_INITTAB_ENTRIES);

    /* Read every line of the file */
    for (int i = 0; i < n_entries; i++) {
        inittab_entry_t *e = &entries[i];

        /* If invoked run level doesn't match entry — skip */
        if (e->ie_runlevel != current_runlevel &&
            e->ie_runlevel != 0)
            continue;

        /* Run level matched — fork to run the command */
        printf("[init] spawning: %s\n", e->ie_cmd);

        int pid = kernel_fork();
        if (pid == 0) {
            /* ── Child process ──────────────────────────── */
            char *argv[] = { e->ie_cmd, NULL };
            kernel_exec(e->ie_cmd, argv, NULL);
            kernel_exit(1);   /* exec failed */
        } else if (pid > 0) {
            /* ── Parent (init) — store child PID ────────── */
            e->ie_child_pid = (uint32_t)pid;

            if (e->ie_action == INIT_WAIT ||
                e->ie_action == INIT_BOOTWAIT) {
                /* Wait for this child before continuing */
                int status;
                kernel_wait(&status);
            }
            /* RESPAWN and ONCE: init continues without waiting */
        }
    }

    /* ── Monitor loop: reap dead children, respawn if needed ─ */
    for (;;) {
        int status;
        int id = kernel_wait(&status);

        if (id == -1) {
            /* No children — should not happen for init */
            break;
        }

        /* Find which inittab entry owned the dead child */
        for (int i = 0; i < n_entries; i++) {
            if (entries[i].ie_child_pid == (uint32_t)id) {
                printf("[init] child pid=%d (%s) died "
                       "status=%d\n",
                       id, entries[i].ie_cmd, status);

                if (entries[i].ie_action == INIT_RESPAWN) {
                    /* Respawn the process */
                    printf("[init] respawning %s\n",
                           entries[i].ie_cmd);
                    int new_pid = kernel_fork();
                    if (new_pid == 0) {
                        char *argv[] = {
                            entries[i].ie_cmd, NULL
                        };
                        kernel_exec(entries[i].ie_cmd,
                                    argv, NULL);
                        kernel_exit(1);
                    }
                    entries[i].ie_child_pid = (uint32_t)new_pid;
                }
                break;
            }
        }
    }
}

/* ── swapper_loop ────────────────────────────────────────────
 * Process 0 becomes the swapper after forking process 1.
 * It manages the allocation of process address space between
 * main memory and swap devices. Runs as an infinite loop,
 * sleeping when there is nothing to do.
 */
int swapper_loop(void)
{
    printf("[swapper] process 0 entering swapper loop\n");

    for (;;) {
        /* Check if any process needs to be swapped in */
        int work_to_do = 0;

        /* Scan for PROC_READY_SWAPPED processes */
        /* Scan for memory pressure requiring swap out      */

        if (!work_to_do) {
            printf("[swapper] no work, sleeping\n");
            /* proc_sleep(SWAPPER_EVENT, PSWP, 0); */
            break;  /* break simulates sleep for demo         */
        }

        /* Swap processes in/out as needed */
        /* swapout(candidate_proc); */
        /* swapin(ready_swapped_proc); */
    }

    return 0;
}

/* ─────────────────────────────────────────────────────────────
 * 11. Algorithm start
 *     System startup procedure — runs on bootstrap.
 */
void kernel_start(void)
{
    printf("========================================\n");
    printf("  UNIX Kernel Starting\n");
    printf("========================================\n");

    /* ── Initialize all kernel data structures ─────────────── */
    init_kernel_data_structures();

    /* ── Pseudo-mount of root file system ──────────────────── */
    pseudo_mount_root();

    /* ── Hand-craft environment of process 0 (swapper) ─────── */
    handcraft_proc0();

    /* ── Fork process 1 (init) ──────────────────────────────── */
    printf("[start] forking process 1 (init)\n");
    int pid = kernel_fork();

    if (pid == 0) {
        /* ── Process 1 code ───────────────────────────────── */
        kernel_init_process();
        /* Never reached — exec'd /etc/init above             */
        kernel_exit(1);

    } else {
        /* ── Process 0 continues here ────────────────────── */
        printf("[start] process 0: init pid=%d forked\n", pid);

        /* Fork additional kernel processes (vhand, etc.) */
        spawn_kernel_processes();

        /* Process 0 becomes the swapper */
        printf("[start] process 0 becoming swapper\n");
        swapper_loop();
    }
}
