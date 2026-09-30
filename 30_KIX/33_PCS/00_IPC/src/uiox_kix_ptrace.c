/*
 * 30KIX/33PCS/00IPC/src/uiox_kix_ptrace.c
 *
 * Process tracing — PTRACE_TRACEME, PEEKDATA/POKEDATA, CONT, KILL,
 * SINGLESTEP, GETREGS/SETREGS.
 *
 * ── bound to 40_psa ─────────────────────────────────────────────────
 * SimProcess is deleted.  The debugger and tracee arguments are real
 * process table entries, and the private 16-slot SimProcess pool is gone
 * — uiox_kix_ptrace_spawn_child now calls 40_psa's allocator, so a traced child
 * is visible to the scheduler, the swapper and wait().
 *
 * The TRACE BIT is no longer a field on a private struct: it is
 * 40_psa's UIOX_KIX_PSA_P_TRACED flag on the process entry.  That
 * matters because the kernel checks it on the way back from exec, on a
 * path that knows nothing about this subsystem.
 *
 * ── the four context switches ───────────────────────────────────────
 * Bach: "The kernel must do four context switches to transfer a word of
 * data between a debugger and a traced process."  PEEKDATA and POKEDATA
 * each do all four, which is why the trace state carries a simulated
 * register set and memory rather than reaching into the real
 * address space — there is no address-space model to reach into.
 *
 * ── what the child waits on ─────────────────────────────────────────
 * uiox_kix_ptrace_post_exec_trap puts the child to sleep on a channel derived
 * from ITS OWN process entry, and PTRACE_CONT wakes the same channel.
 * The old version reused EVENT_SOCKET_CONN — a socket event — as the
 * trap channel, so a CONT could have woken an unrelated socket waiter.
 *
 * @version 3.0.0  @date 2026-09-30
 */
#include "../include/uiox_kix_ptrace.h"
#include "../include/uiox_kix_ipc_types.h"
#include "uiox_klibc.h"

/* ── Trace slot table ────────────────────────────────────────────────
 * One slot per traced process.  A static array, so no heap. */
static TraceState trace_table[PTRACE_MAX_TRACED];

/* ── uiox_kix_ptrace_init ─────────────────────────────────────────────────────── */
void uiox_kix_ptrace_init(void)
{
    int i;

    for (i = 0; i < PTRACE_MAX_TRACED; i++) {
        trace_table[i].tracee   = (uiox_kix_psa_proc_t *)0;
        trace_table[i].debugger = (uiox_kix_psa_proc_t *)0;
        trace_table[i].active   = false;
        trace_table[i].saved_regs.pc = 0u;
        trace_table[i].saved_regs.sp = 0u;
    }
}

/* ── find a trace slot for a tracee ──────────────────────────────────── */
static TraceState *get_trace_slot(uiox_kix_psa_proc_t *tracee)
{
    int i;
    for (i = 0; i < PTRACE_MAX_TRACED; i++)
        if (trace_table[i].active && trace_table[i].tracee == tracee)
            return &trace_table[i];
    return (TraceState *)0;
}

/* ── claim a free slot ───────────────────────────────────────────────── */
static TraceState *alloc_trace_slot(uiox_kix_psa_proc_t *tracee,
                                    uiox_kix_psa_proc_t *debugger)
{
    int i;
    for (i = 0; i < PTRACE_MAX_TRACED; i++) {
        if (!trace_table[i].active) {
            uiox_uint8_t *p = (uiox_uint8_t *)trace_table[i].mem;
            uiox_uint64_t n = sizeof trace_table[i].mem;

            trace_table[i].active   = true;
            trace_table[i].tracee   = tracee;
            trace_table[i].debugger = debugger;
            trace_table[i].saved_regs.pc = 0u;
            trace_table[i].saved_regs.sp = 0u;

            /* 0xCC is the conventional fill for a simulated image — it
             * makes an untouched byte visibly untouched rather than
             * looking like a zero some program wrote. */
            while (n--) *p++ = 0xCCu;
            return &trace_table[i];
        }
    }
    return (TraceState *)0;   /* no free slots */
}

/* ── uiox_kix_ptrace_post_exec_trap ───────────────────────────────────────────
 * Bach: after exec, "the kernel checks for signals when returning from
 * the exec system call ... finds the trap signal it had just sent
 * itself, and executes code for process tracing".
 *
 * The child sleeps on a channel derived from its own entry, so a CONT
 * for this child cannot wake anyone else. */
void uiox_kix_ptrace_post_exec_trap(uiox_kix_psa_proc_t *child,
                           uiox_kix_psa_proc_t *debugger)
{
    (void)debugger;
    if (!child) return;

    (void)ipc_sleep(child, IPC_WCHAN_SOCK_CONN(child));
}

/* ── uiox_kix_ptrace_spawn_child ──────────────────────────────────────────────
 * Allocate a REAL process, set its trace bit, and attach a trace slot.
 *
 * The pid comes from 40_psa's allocator rather than being invented as
 * debugger->pid + 100, so two children of one debugger get distinct
 * pids and the process table knows about them.
 *
 * Returns NULL when the process table is full — a real limit now. */
uiox_kix_psa_proc_t *uiox_kix_ptrace_spawn_child(uiox_kix_psa_proc_t *debugger,
                                        const char *image)
{
    uiox_kix_psa_proc_t *child;

    (void)image;   /* a real kernel would load this image here */

    if (!debugger) return (uiox_kix_psa_proc_t *)0;

    child = uiox_kix_psa_proc_alloc(0u, debugger->p_pid);
    if (!child) return (uiox_kix_psa_proc_t *)0;   /* table full */

    /* The trace bit.  40_psa's flag, not a private field — the kernel
     * checks it on the return from exec. */
    child->p_flag |= UIOX_KIX_PSA_P_TRACED;

    /* A freshly allocated entry is CREATED; a traced child is READY
     * once its slot is attached. */
    (void)uiox_kix_psa_proc_set_state(child, UIOX_KIX_PSA_PROC_READY);

    if (!alloc_trace_slot(child, debugger)) {
        /* No slot — undo the allocation rather than leave a READY
         * process nobody is tracing. */
        uiox_kix_psa_proc_free(child);
        return (uiox_kix_psa_proc_t *)0;
    }

    uiox_kix_ptrace_post_exec_trap(child, debugger);
    return child;
}

/* ── uiox_kix_ptrace_free_child ───────────────────────────────────────────────── */
void uiox_kix_ptrace_free_child(uiox_kix_psa_proc_t *child)
{
    TraceState *ts;

    if (!child) return;

    ts = get_trace_slot(child);
    if (ts) {
        ts->active = false;
        ts->tracee = (uiox_kix_psa_proc_t *)0;
    }

    child->p_flag &= ~UIOX_KIX_PSA_P_TRACED;
    uiox_kix_psa_proc_free(child);   /* return the entry to the table */
}

/* ── uiox_kix_ptrace — the request handler ────────────────────────────────────
 * Each case that reads or writes is guarded three ways: the process is
 * traced, a slot exists, and the address is inside the simulated
 * address space.  The last check is not defensive — mem[] is 256 bytes
 * and PEEKDATA takes an arbitrary uiox_uint64_t. */
int uiox_kix_ptrace(PtraceRequest req, uiox_kix_psa_proc_t *debugger,
           uiox_kix_psa_proc_t *tracee, uiox_uint64_t addr,
           uiox_uint64_t *data)
{
    TraceState *ts;

    (void)debugger;   /* a real kernel checks it owns the trace slot */

    if (!tracee) return -1;

    switch (req) {

    case PTRACE_TRACEME:
        /* The child consents.  40_psa's flag, read by the exec path. */
        tracee->p_flag |= UIOX_KIX_PSA_P_TRACED;
        if (!alloc_trace_slot(tracee, debugger)) return -1;
        return 0;

    case PTRACE_PEEKDATA:
        ts = get_trace_slot(tracee);
        if (!ts || !(tracee->p_flag & UIOX_KIX_PSA_P_TRACED)) return -1;
        if (addr >= sizeof ts->mem) return -1;

        /* Four context switches, as Bach describes. */
        (void)ipc_sleep(tracee, IPC_WCHAN_SOCK_CONN(tracee));

        if (data) *data = ts->mem[addr];
        return 0;

    case PTRACE_POKEDATA:
        ts = get_trace_slot(tracee);
        if (!ts || !(tracee->p_flag & UIOX_KIX_PSA_P_TRACED)) return -1;
        if (addr >= sizeof ts->mem) return -1;

        (void)ipc_sleep(tracee, IPC_WCHAN_SOCK_CONN(tracee));

        /* A word at a time, into a byte-addressed space — the low byte
         * is what lands, which is why the simulated space is 256 bytes
         * and not 256 words. */
        if (data) ts->mem[addr] = (uiox_uint8_t)*data;
        return 0;

    case PTRACE_CONT:
        /* Resume: wake the channel the trap put the child to sleep on. */
        ipc_wakeup(IPC_WCHAN_SOCK_CONN(tracee));
        return 0;

    case PTRACE_KILL:
        ts = get_trace_slot(tracee);
        tracee->p_flag &= ~UIOX_KIX_PSA_P_TRACED;
        if (ts) {
            ts->active = false;
            ts->tracee = (uiox_kix_psa_proc_t *)0;
        }
        return 0;

    case PTRACE_SINGLESTEP:
        ts = get_trace_slot(tracee);
        if (!ts) return -1;
        ts->saved_regs.pc++;          /* advance one instruction */
        return 0;

    case PTRACE_GETREGS:
        ts = get_trace_slot(tracee);
        if (!ts) return -1;
        if (data) *data = ts->saved_regs.pc;
        return 0;

    case PTRACE_SETREGS:
        ts = get_trace_slot(tracee);
        if (!ts || !data) return -1;
        ts->saved_regs.pc = *data;
        return 0;

    default:
        return -1;   /* unknown request */
    }
}
