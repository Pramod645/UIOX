#include "../include/exec.h"
#include "../include/signal.h"
/* stdio.h/stdlib.h/string.h removed — provided via exec.h -> uiox_klibc.h */

/* ── Static exec_args pool (replaces calloc/free) ────────────── */
#define EXEC_ARGS_POOL_SZ  4
static exec_args_t s_ea_pool[EXEC_ARGS_POOL_SZ];
static uint8_t     s_ea_used[EXEC_ARGS_POOL_SZ];

static exec_args_t *ea_alloc(void)
{
    uint32_t i;
    for (i = 0; i < EXEC_ARGS_POOL_SZ; i++) {
        if (!s_ea_used[i]) {
            s_ea_used[i] = 1;
            memset(&s_ea_pool[i], 0, sizeof(exec_args_t));
            return &s_ea_pool[i];
        }
    }
    return (exec_args_t *)0;
}

static void ea_free(exec_args_t *ea)
{
    uint32_t i;
    if (!ea) return;
    for (i = 0; i < EXEC_ARGS_POOL_SZ; i++) {
        if (&s_ea_pool[i] == ea) {
            memset(ea, 0, sizeof(exec_args_t));
            s_ea_used[i] = 0;
            return;
        }
    }
}

/* ── exec_verify_file ────────────────────────────────────────── */
int exec_verify_file(struct inode *ip, exec_hdr_t *hdr)
{
    if (!ip || !hdr) return -1;
    hdr->eh_magic = EXEC_MAGIC;
    hdr->eh_entry = 0x1000;
    hdr->eh_nseg  = 3;
    if (hdr->eh_magic != EXEC_MAGIC &&
        hdr->eh_magic != EXEC_MAGIC_AOUT) {
        printf("[exec] ERROR: bad magic number\n");
        return -1;
    }
    printf("[exec] file verified: %u segments entry=0x%x\n",
           hdr->eh_nseg, hdr->eh_entry);
    return 0;
}

/* ── exec_detach_all ─────────────────────────────────────────── */
void exec_detach_all(struct u_area *u, struct proc *p)
{
    (void)u; (void)p;
    printf("[exec] detaching all regions\n");
}

/* ── exec_load_segments ──────────────────────────────────────── */
int exec_load_segments(struct u_area *u, struct proc *p,
                       struct inode *ip, exec_hdr_t *hdr)
{
    uint32_t i;
    /* Suppress unused-parameter warnings — real impl uses all four */
    (void)u; (void)p; (void)ip;

    if (!hdr) return -1;
    for (i = 0; i < hdr->eh_nseg && i < MAX_SEGMENTS; i++) {
        seg_desc_t *sd = &hdr->eh_segs[i];
        printf("[exec] loading segment %u type=%d vaddr=0x%x sz=%u\n",
               i, (int)sd->sd_type, sd->sd_vaddr, sd->sd_mem_sz);
    }
    return 0;
}

/* ── exec_copy_args ──────────────────────────────────────────── */
void exec_copy_args(struct u_area *u, exec_args_t *ea)
{
    (void)u;
    if (!ea) return;
    printf("[exec] copying %d args to new user stack\n", ea->ea_argc);
}

/* ── exec_handle_setuid ──────────────────────────────────────── */
void exec_handle_setuid(struct proc *p, exec_hdr_t *hdr)
{
    (void)p;
    if (!hdr) return;
    if (hdr->eh_setuid)
        printf("[exec] setuid bit set\n");
}

/* ── exec_init_registers ─────────────────────────────────────── */
void exec_init_registers(struct u_area *u, exec_hdr_t *hdr)
{
    (void)u;
    if (!hdr) return;
    printf("[exec] initialising registers entry=0x%x\n", hdr->eh_entry);
}

/* ── Algorithm kernel_exec (Section 6) ──────────────────────── */
int kernel_exec(const char *filename,
                char *const argv[], char *const envp[])
{
    exec_hdr_t    hdr;
    exec_args_t  *ea;
    struct proc  *proc  = (struct proc  *)0;
    struct u_area *u    = (struct u_area *)0;
    struct inode  *ip   = (struct inode  *)0;
    int            i;

    (void)envp;

    printf("[exec] kernel_exec('%s')\n", filename ? filename : "?");

    /* Simulate inode lookup */
    if (!filename) return -1;

    if (exec_verify_file(ip, &hdr) != 0)
        return -1;

    /* Copy args to kernel space — static pool replaces calloc */
    ea = ea_alloc();
    if (!ea) return -1;

    strncpy(ea->ea_filename, filename, MAX_ARG_LEN - 1);
    ea->ea_argc = 0;
    if (argv) {
        for (i = 0; argv[i] && i < MAX_ARGS; i++) {
            strncpy(ea->ea_argv[i], argv[i], MAX_ARG_LEN - 1);
            ea->ea_argc++;
        }
    }

    exec_detach_all(u, proc);

    if (exec_load_segments(u, proc, ip, &hdr) != 0) {
        ea_free(ea);
        return -1;
    }

    exec_copy_args(u, ea);
    ea_free(ea);                               /* was: free(ea) */

    exec_handle_setuid(proc, &hdr);
    exec_init_registers(u, &hdr);

    printf("[exec] exec complete, entry=0x%x\n", hdr.eh_entry);
    return 0;
}

/* ── Algorithm xalloc (Section 7) ───────────────────────────── */
void xalloc(struct inode *ip, struct proc *p,
            struct u_area *u, exec_hdr_t *hdr)
{
    /* Suppress unused-parameter warnings */
    (void)p; (void)u;

    if (!ip || !hdr) return;
    printf("[xalloc] allocating text region entry=0x%x nseg=%u\n",
           hdr->eh_entry, hdr->eh_nseg);
}
