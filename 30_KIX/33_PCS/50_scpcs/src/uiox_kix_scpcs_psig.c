/* ── psig ────────────────────────────────────────────────────────────
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_psig.c
 * Deliver one signal to the current process.
 *
 * A handler is read from the u area's u_signal[] through
 * uiox_kix_psa_uarea_for(slot) — no cast, the accessor brk.c already
 * uses.  The array is in .bss, so on a fresh boot every entry is 0 and
 * SIG_DFL is 0 by construction.
 *
 * ── what this can and cannot do yet ─────────────────────────────────
 * sigaction() still returns ENOSYS, so nothing can INSTALL a handler and
 * every disposition is the default.  The default branch is therefore the
 * only reachable one, and that part is complete: a fatal uncaught signal
 * takes the exit path, which sets ZOMBIE so wait() collects it.
 *
 * The handler branch is written and unreachable.  It is left visible so
 * the missing piece is a call, not a search. */

 #include "../include/uiox_kix_scpcs.h"
 #include "uiox_kix_psa_proc_algo.h"
 
/* ── signal dispositions ─────────────────────────────────────────────
 * u_signal[] holds one int per signal.  These are the two sentinel
 * values; anything else is a handler address.
 *
 * 0 - SIG_DFL — the default.  Fatal signals terminate the process;
 *               non-fatal ones are ignored.
 * -1 - SIG_IGN — the process asked not to be disturbed.
 *
 * 0 as the default is not arbitrary: the u area lives in .bss, which is
 * zeroed at boot, so every disposition starts as SIG_DFL by
 * construction rather than by an initialiser that could be forgotten. */
#define SIG_DFL   ((int)0)
#define SIG_IGN   ((int)(-1))

void uiox_kix_scpcs_psig(uiox_kix_psa_proc_t *p, int sig)
{
    uiox_kix_psa_u_area_t *u;
    int                    disp;
    uint32_t               slot;

    if (!p || sig <= 0 || sig >= (int)UIOX_KIX_PSA_NSIG) return;

    /* clear the pending bit: delivered once, not on every return */
    p->p_sig &= ~(uiox_uint32_t)(1u << (uiox_uint32_t)sig);

    slot = (uint32_t)(p - uiox_kix_psa_proc_table);
    u    = uiox_kix_psa_uarea_for(slot);
    disp = u ? u->u_signal[sig] : 0;      /* 0 == SIG_DFL */

        /* ── SIG_IGN ─────────────────────────────────────────────────────
     * -1: the process asked not to be disturbed by this signal.  The
     * pending bit is already cleared above, so nothing re-tests it. */
    if (disp == SIG_IGN)
        return;

    /* ── a handler is installed ──────────────────────────────────────
     * A non-zero, non-SIG_IGN disposition is a handler ADDRESS.  Calling
     * it needs a frame set up — the handler's pc, a stack for it, and a
     * path back through sigreturn — which is the remaining piece.
     *
     * Until that exists, this returns rather than falling through to the
     * SIG_DFL branch below: a process that installed a handler has NOT
     * asked to be killed, and treating its disposition as default would
     * terminate it.  Refusing is honest; terminating on a caught signal
     * is not. */
    if (disp != SIG_DFL)
        return;

    /* SIG_DFL: fatal signals terminate.  The exit path sets ZOMBIE, so
     * a parent's wait() collects the status. */
    if (sig == (int)UIOX_KIX_PSA_SIGKILL ||
        sig == (int)UIOX_KIX_PSA_SIGTERM ||
        sig == (int)UIOX_KIX_PSA_SIGINT  ||
        sig == (int)UIOX_KIX_PSA_SIGQUIT ||
        sig == (int)UIOX_KIX_PSA_SIGABRT) {
        (void)uiox_kix_scpcs_exit((uiox_uint64_t)(128 + sig));  /* shell convention */
    }
    /* SIG_DFL for a non-fatal signal is "ignore" — nothing to do. */
}

/* ── signal delivery on the way back to EL0 ──────────────────────────
 * Called from each arch's SVC return, after the syscall's value has been
 * written into the saved frame and before the registers are restored.
 *
 * The early-out matters: this runs on EVERY syscall return, and the
 * common case is nothing pending.  One comparison keeps it off the hot
 * path rather than walking the table. */
static void signal_run(void)
{
    uiox_kix_psa_proc_t *p = uiox_kix_scpcs_current();
    int                  sig;

    if (!p) return;
    if ((p->p_sig & ~p->p_sigmask) == 0u) return;   /* nothing deliverable */

    sig = uiox_kix_scpcs_issig(p);
    if (sig != 0)
        uiox_kix_scpcs_psig(p, sig);               /* may not return */
}

void arm64_signal_check(void)   { signal_run(); }
void arm32_signal_check(void)   { signal_run(); }
void riscv64_signal_check(void) { signal_run(); }
void x86_64_signal_check(void)  { signal_run(); }
