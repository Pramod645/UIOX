/*
 * 30_KIX/33_PCS/40_procStruct/src/syscall.c
 *
 * Freestanding fixes (v2.0)
 *
 * @version 2.0.0  @date 2026-07-24
 */
#include "../include/proc_algo.h"
#include "../include/context.h"
syscall_entry_t syscall_table[NSYSCALL];
u_area_t        u;
void syscall_register(int num, syscall_fn_t fn, int nargs, const char *name)
{
    if (num < 0 || num >= NSYSCALL) return;
    syscall_table[num].se_fn    = fn;
    syscall_table[num].se_nargs = nargs;
    syscall_table[num].se_name  = name;
}
int syscall(int callnum, uintptr_t *args)
{
    syscall_entry_t *ep;
    uintptr_t        local_args[8];
    int              nargs, ret;
    if (callnum < 0 || callnum >= NSYSCALL) {
        printf("[syscall] ERROR: invalid call number %d\n", callnum);
        u.u_error = EINVAL; return -1;
    }
    ep = &syscall_table[callnum];
    if (!ep->se_fn) {
        printf("[syscall] ERROR: no handler for syscall %d\n", callnum);
        u.u_error = EINVAL; return -1;
    }
    printf("[syscall] invoking %s (num=%d nargs=%d)\n",
           ep->se_name ? ep->se_name : "?", callnum, ep->se_nargs);
    memset(local_args, 0, sizeof local_args);
    nargs = ep->se_nargs;
    if (nargs > 8) nargs = 8;
    if (args) memcpy(local_args, args, (size_t)nargs * sizeof(uintptr_t));
    u.u_qsave.regs[0] = 0;
    u.u_error         = 0;
    u.u_rval          = 0;
    ret = ep->se_fn(&u, local_args);
    if (u.u_qsave.regs[0] != 0) {
        printf("[syscall] ERROR: aborted by signal\n");
        u.u_saved_regs.rc_r0    = (uint32_t)u.u_error;
        u.u_saved_regs.rc_carry = 1;
        return -1;
    }
    if (u.u_error) {
        u.u_saved_regs.rc_r0    = (uint32_t)u.u_error;
        u.u_saved_regs.rc_carry = 1;
        printf("[syscall] %s error=%d\n",
               ep->se_name ? ep->se_name : "?", u.u_error);
        return -1;
    }
    u.u_saved_regs.rc_r0    = (uint32_t)((uint64_t)u.u_rval & 0xFFFFFFFFULL);
    u.u_saved_regs.rc_r1    = (uint32_t)((uint64_t)u.u_rval >> 32);
    u.u_saved_regs.rc_carry = 0;
    printf("[syscall] %s ok rval=%ld\n",
           ep->se_name ? ep->se_name : "?", (long)u.u_rval);
    return ret;
}
