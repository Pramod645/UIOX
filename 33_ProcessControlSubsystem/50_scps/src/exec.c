#include "../include/exec.h"
#include "../include/signal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ── exec_verify_file ────────────────────────────────────────
 * Check that the file is executable and is a valid load module.
 * Reads the exec header from the inode.
 */
int exec_verify_file(struct inode *ip, exec_hdr_t *hdr)
{
    if (!ip || !hdr) return -1;

    /* Read file header (simulated) */
    hdr->eh_magic = EXEC_MAGIC;
    hdr->eh_entry = 0x1000;
    hdr->eh_nseg  = 3;

    /* Simulate three segments: text, data, stack */
    hdr->eh_segs[0] = (seg_desc_t){
        SEG_TEXT, 0x1000, 64, 4096, 4096,
        SEG_READ | SEG_EXEC
    };
    hdr->eh_segs[1] = (seg_desc_t){
        SEG_DATA, 0x401000, 4160, 8192, 8192,
        SEG_READ | SEG_WRITE
    };
    hdr->eh_segs[2] = (seg_desc_t){
        SEG_STACK, 0x7FFF0000, 0, 65536, 65536,
        SEG_READ | SEG_WRITE
    };

    if (hdr->eh_magic != EXEC_MAGIC &&
        hdr->eh_magic != EXEC_MAGIC_AOUT) {
        fprintf(stderr, "[exec] bad magic number\n");
        return -1;
    }

    printf("[exec] file verified: %u segments, "
           "entry=0x%x\n", hdr->eh_nseg, hdr->eh_entry);
    return 0;
}

/* ── exec_detach_all ─────────────────────────────────────────
 * Detach all regions currently attached to the process.
 * Called after exec parameters are saved in kernel space.
 */
void exec_detach_all(struct u_area *u, struct proc *p)
{
    (void)p;
    if (!u) return;
    printf("[exec] detaching all old regions\n");
    /*
     * for each entry in u->u_pregs:
     *     if (entry valid) detachreg(&u->u_pregs[i]);
     */
}

/* ── exec_load_segments ──────────────────────────────────────
 * For each segment in the exec header:
 *   1. allocate a new region  (allocreg)
 *   2. attach it              (attachreg)
 *   3. load it from the file  (loadreg, if not BSS)
 */
int exec_load_segments(struct u_area *u, struct proc *p,
                       struct inode *ip, exec_hdr_t *hdr)
{
    if (!u || !hdr) return -1;

    for (uint32_t i = 0; i < hdr->eh_nseg; i++) {
        seg_desc_t *sd = &hdr->eh_segs[i];

        /* Map segment type to region type */
        int reg_type;
        switch (sd->sd_type) {
        case SEG_TEXT:  reg_type = 1; break;  /* REG_TEXT  */
        case SEG_DATA:
        case SEG_BSS:   reg_type = 2; break;  /* REG_DATA  */
        case SEG_STACK: reg_type = 3; break;  /* REG_STACK */
        default:        reg_type = 2; break;
        }

        printf("[exec] segment %u: type=%d vaddr=0x%x "
               "filesz=%u memsz=%u\n",
               i, sd->sd_type, sd->sd_vaddr,
               sd->sd_file_sz, sd->sd_mem_sz);

        /* allocreg(ip, reg_type)      — allocate region     */
        /* attachreg(rp, p, sd_vaddr, reg_type) — attach it  */

        if (sd->sd_type != SEG_BSS && sd->sd_file_sz > 0) {
            /* loadreg(prp, sd_vaddr, ip,
             *         sd_file_off, sd_file_sz)               */
            printf("[exec] loading segment %u from file\n", i);
        } else {
            /* BSS: growreg to mem_sz, zero memory */
            printf("[exec] zero-filling BSS segment %u\n", i);
        }
    }
    return 0;
}

/* ── exec_copy_args ──────────────────────────────────────────
 * Copy exec arguments and environment into the new user stack.
 */
void exec_copy_args(struct u_area *u, exec_args_t *ea)
{
    if (!u || !ea) return;
    printf("[exec] copying %d args and %d env vars "
           "into new stack\n", ea->ea_argc, ea->ea_envc);
    /*
     * In real kernel:
     *   - calculate total byte count needed on stack
     *   - growreg stack region to accommodate
     *   - copy strings into new user address space
     *   - build argv[] and envp[] pointer arrays on stack
     */
}

/* ── exec_handle_setuid ──────────────────────────────────────
 * If the executable has setuid/setgid bits, change the
 * effective UID/GID of the process accordingly.
 */
void exec_handle_setuid(struct proc *p, exec_hdr_t *hdr)
{
    if (!p || !hdr) return;
    if (hdr->eh_setuid) {
        printf("[exec] setuid: euid changed to file owner\n");
        /* current_proc->p_euid = ip->i_uid; */
    }
    if (hdr->eh_setgid) {
        printf("[exec] setgid: egid changed to file group\n");
        /* current_proc->p_egid = ip->i_gid; */
    }
}

/* ── exec_init_registers ─────────────────────────────────────
 * Set up the user saved register context so that when the
 * kernel returns to user mode, execution begins at the
 * program entry point with a fresh stack.
 */
void exec_init_registers(struct u_area *u, exec_hdr_t *hdr)
{
    if (!u || !hdr) return;
    printf("[exec] setting PC=0x%x for return to user\n",
           hdr->eh_entry);
    /* u->u_saved_regs.rc_pc = hdr->eh_entry; */
    /* u->u_saved_regs.rc_sp = new_stack_top;  */
    /* u->u_saved_regs.rc_sr = 0;              */
}

/* ─────────────────────────────────────────────────────────────
 * 6. Algorithm exec
 *    input : filename, argv[], envp[]
 *    output: none (process image is replaced)
 */
int kernel_exec(const char *filename,
                char *const argv[],
                char *const envp[])
{
    if (!filename) return -1;

    printf("[exec] exec(\"%s\")\n", filename);

    /* ── Get file inode (algorithm namei) ─────────────────── */
    struct inode *ip = NULL;
    /* ip = namei(filename); */
    printf("[exec] looked up inode for \"%s\"\n", filename);

    /* ── Verify file is executable and has permission ─────── */
    exec_hdr_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    if (exec_verify_file(ip, &hdr) != 0) {
        /* iput(ip); */
        return -1;
    }

    /* ── Copy exec parameters from old address space to
     *    kernel space (before old space is destroyed) ─────── */
    exec_args_t *ea = (exec_args_t *)calloc(1, sizeof(exec_args_t));
    if (!ea) return -1;

    strncpy(ea->ea_filename, filename, MAX_ARG_LEN - 1);
    ea->ea_argc = 0;
    if (argv) {
        for (int i = 0; argv[i] && i < MAX_ARGS; i++) {
            strncpy(ea->ea_argv[i], argv[i], MAX_ARG_LEN - 1);
            ea->ea_argc++;
        }
    }
    ea->ea_envc = 0;
    if (envp) {
        for (int i = 0; envp[i] && i < MAX_ENV; i++) {
            strncpy(ea->ea_envp[i], envp[i], MAX_ARG_LEN - 1);
            ea->ea_envc++;
        }
    }
    printf("[exec] copied %d args, %d env vars to kernel\n",
           ea->ea_argc, ea->ea_envc);

    /* ── Detach all old regions (algorithm detachreg) ─────── */
    struct u_area *u    = NULL;
    struct proc   *proc = NULL;
    exec_detach_all(u, proc);

    /* ── Allocate, attach, load new regions ───────────────── */
    if (exec_load_segments(u, proc, ip, &hdr) != 0) {
        free(ea);
        /* iput(ip); */
        return -1;
    }

    /* ── Copy exec parameters into new user stack region ──── */
    exec_copy_args(u, ea);
    free(ea);

    /* ── Special processing: setuid, tracing ──────────────── */
    exec_handle_setuid(proc, &hdr);

    /* ── Initialize user register save area for return ────── */
    exec_init_registers(u, &hdr);

    /* ── Release inode (algorithm iput) ───────────────────── */
    /* iput(ip); */
    printf("[exec] exec complete, returning to user at "
           "entry 0x%x\n", hdr.eh_entry);
    return 0;
}

/* ─────────────────────────────────────────────────────────────
 * 7. Algorithm xalloc
 *    input : inode of executable, process, u area, exec header
 *    output: none
 *
 *    Allocate and initialise the text region for exec.
 */
void xalloc(struct inode *ip, struct proc *p,
            struct u_area *u, exec_hdr_t *hdr)
{
    if (!ip || !hdr) return;

    /* If executable has no separate text region (combined I&D) */
    if (!(hdr->eh_segs[0].sd_flags & SEG_EXEC)) {
        printf("[xalloc] no separate text region\n");
        return;
    }

    /* Check whether a text region for this inode already exists
     * (another process may have exec'd the same file)          */
    int region_exists = 0;  /* search active_region_list in real*/

    if (region_exists) {
        /* ── Text region already exists — attach to it ────── */
        printf("[xalloc] text region exists, attaching\n");

        /* lock region */
        /* while region contents not ready: */
        /*   increment region reference count                  */
        /*   unlock region                                     */
        /*   sleep (event: contents ready)                     */
        /*   lock region                                       */
        /*   decrement region reference count                  */

        /* attachreg(rp, p, text_vaddr, REG_TEXT) */
        /* unlock region */

    } else {
        /* ── No such text region — create one ─────────────── */
        printf("[xalloc] creating new text region\n");

        /* allocreg(ip, REG_TEXT)  — region is returned locked */

        /* If inode mode has sticky bit — turn on REG_STICKY   */
        /* attachreg(rp, p, text_vaddr, REG_TEXT)              */

        /* loadreg(prp, text_vaddr, ip,
         *         text_file_offset, text_file_size)           */
        printf("[xalloc] loaded text from file\n");

        /* Change region protection to read-only in per-process
         * region table entry                                   */

        /* unlock region */
    }
}
