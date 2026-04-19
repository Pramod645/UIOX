#include "../include/proc_algo.h"
#include "../include/context.h"
#include <string.h>
#include <stdio.h>

/* ── Globals ────────────────────────────────────────────────── */
syscall_entry_t syscall_table[NSYSCALL];
u_area_t        u;

/* ── syscall_register ───────────────────────────────────────
 * Register a system call handler.
 */
void syscall_register(int num, syscall_fn_t fn,
                      int nargs, const char *name)
{
    if (num < 0 || num >= NSYSCALL) return;
    syscall_table[num].se_fn    = fn;
    syscall_table[num].se_nargs = nargs;
    syscall_table[num].se_name  = name;
}

/* ─────────────────────────────────────────────────────────────
 * 2. Algorithm syscall
 *    input : system call number, argument array
 *    output: result of system call (in u area)
 */
int syscall(int callnum, uintptr_t *args)
{
    /* Find entry in system call table */
    if (callnum < 0 || callnum >= NSYSCALL) {
        fprintf(stderr, "[syscall] invalid call number %d\n",
                callnum);
        u.u_error = EINVAL;
        return -1;
    }

    syscall_entry_t *ep = &syscall_table[callnum];
    if (!ep->se_fn) {
        fprintf(stderr, "[syscall] no handler for syscall %d\n",
                callnum);
        u.u_error = EINVAL;
        return -1;
    }

    printf("[syscall] invoking %s (num=%d, nargs=%d)\n",
           ep->se_name ? ep->se_name : "?", callnum, ep->se_nargs);

    /* Determine number of parameters; copy from address space
     * to u area (simulated: args array passed directly) */
    uintptr_t local_args[8] = {0};
    int nargs = ep->se_nargs;
    if (nargs > 8) nargs = 8;
    if (args)
        memcpy(local_args, args, (size_t)nargs * sizeof(uintptr_t));

    /* Save current context for abortive return (longjmp target) */
    if (setjmp(u.u_qsave) != 0) {
        /* Abortive return: signal interrupted the syscall */
        fprintf(stderr, "[syscall] aborted by signal\n");
        u.u_saved_regs.rc_r0    = (uint32_t)u.u_error;
        u.u_saved_regs.rc_carry = 1;   /* turn on carry bit */
        return -1;
    }

    /* Clear previous error */
    u.u_error = 0;
    u.u_rval  = 0;

    /* Invoke system call code in kernel */
    int ret = ep->se_fn(&u, local_args);

    /* Set return values in saved register context */
    if (u.u_error) {
        /* Error: set r0 to error number, turn on carry bit */
        u.u_saved_regs.rc_r0    = (uint32_t)u.u_error;
        u.u_saved_regs.rc_carry = 1;
        printf("[syscall] %s returned error=%d\n",
               ep->se_name, u.u_error);
        return -1;
    } else {
        /* Success: set r0 and r1 to return values */
        u.u_saved_regs.rc_r0    = (uint32_t)(u.u_rval & 0xFFFFFFFF);
        u.u_saved_regs.rc_r1    = (uint32_t)(u.u_rval >> 32);
        u.u_saved_regs.rc_carry = 0;
        printf("[syscall] %s returned ok rval=%ld\n",
               ep->se_name, (long)u.u_rval);
        return ret;
    }
}
