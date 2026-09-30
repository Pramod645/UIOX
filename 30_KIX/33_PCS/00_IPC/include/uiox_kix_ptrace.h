/*
 * 30KIX/33PCS/00IPC/include/uiox_kix_ptrace.h
 *
 * Process tracing — Bach's process-tracing primitive.
 *
 * ── version 3.0.0: the sleep/wakeup binding ─────────────────────────
 * SimProcess is deleted.  debugger and tracee are uiox_kix_psa_proc_t *
 * — the ONE process type — and the private SimProcess pool that
 * uiox_kix_ptrace_spawn_child drew from is gone with it.
 *
 * ── the four context switches ───────────────────────────────────────
 * Bach: "The kernel must do four context switches to transfer a word of
 * data between a debugger and a traced process: The kernel switches
 * context in the debugger in the uiox_kix_ptrace call until the traced process
 * replies to a query, switches context to the traced process to ask it
 * to perform the request, switches back to the kernel when the traced
 * process replies, and switches context to the debugger when it returns
 * from the uiox_kix_ptrace call."
 *
 * PEEKDATA and POKEDATA each do all four.  That is why they are separate
 * requests rather than a general read/write — the cost is the interface.
 *
 * ── what uiox_kix_ptrace_spawn_child now does ────────────────────────────────
 * It used to allocate from a private 16-slot SimProcess pool and invent
 * a pid (debugger->pid + 100).  It now calls 40_psa's process allocator,
 * so a traced child is a real process table entry that the scheduler,
 * the swapper and wait() can all see.
 *
 * It stays in this file rather than moving to 40_psa: uiox_kix_ptrace is what
 * invokes it, and the debugger-facing API is IPC's.  What it no longer
 * does is keep a pool of its own.
 *
 * @version 3.0.0  @date 2026-09-30
 */
#ifndef UIOX_PTRACE_H
#define UIOX_PTRACE_H

#include "uiox_kix_ipc_types.h"
#include "uiox_kix_psa_process.h"

/* ── Request codes ─────────────────────────────────────────────────── */
typedef enum {
    PTRACE_TRACEME    = 0,  /* child: consent to being traced     */
    PTRACE_PEEKDATA   = 1,  /* debugger: read a word from tracee  */
    PTRACE_POKEDATA   = 2,  /* debugger: write a word into tracee */
    PTRACE_CONT       = 3,  /* debugger: resume the tracee        */
    PTRACE_KILL       = 4,  /* debugger: terminate the tracee     */
    PTRACE_SINGLESTEP = 5,  /* debugger: advance one instruction  */
    PTRACE_GETREGS    = 6,  /* debugger: read the register set    */
    PTRACE_SETREGS    = 7   /* debugger: write the register set   */
} PtraceRequest;

/* ── Register set ──────────────────────────────────────────────────── */
typedef struct {
    uiox_uint64_t pc;       /* program counter                    */
    uiox_uint64_t sp;       /* stack pointer                      */
    uiox_uint64_t regs[8];  /* general-purpose registers          */
} RegSet;

/* ── Trace state ─────────────────────────────────────────────────────
 * One slot per traced process.  mem[] is the simulated address space —
 * 256 bytes, addressed a word at a time by PEEKDATA/POKEDATA, which is
 * why an out-of-range address is rejected rather than faulting.
 *
 * The trace bit itself is NOT here: it is 40_psa's P_TRACED flag on the
 * process entry, because the kernel checks it when returning from exec,
 * on a path that knows nothing about this subsystem. */
typedef struct {
    uiox_kix_psa_proc_t *tracee;
    uiox_kix_psa_proc_t *debugger;
    bool                 active;
    RegSet               saved_regs;
    uiox_uint8_t         mem[256];
} TraceState;

#define PTRACE_MAX_TRACED  8

/* ── API ───────────────────────────────────────────────────────────── */

/* Zero the trace slot table. */
void uiox_kix_ptrace_init(void);

/* The main request handler.  Returns 0 on success, -1 on error:
 *   - a request on a process whose trace bit is not set
 *   - an address outside the simulated address space
 *   - no trace slot available */
int uiox_kix_ptrace(PtraceRequest req, uiox_kix_psa_proc_t *debugger,
           uiox_kix_psa_proc_t *tracee, uiox_uint64_t addr,
           uiox_uint64_t *data);

/* Allocate a real process from 40_psa's table, set its trace bit, and
 * attach it to a trace slot.  Returns NULL when the process table is
 * full — which is a real limit now, not a 16-slot pool. */
uiox_kix_psa_proc_t *uiox_kix_ptrace_spawn_child(uiox_kix_psa_proc_t *debugger,
                                        const char *image);

/* Called by the kernel after exec when the trace bit is set — Bach:
 * "at the end notes that the trace bit is set and sends the child a trap
 * signal".  The child sleeps until the debugger resumes it. */
void uiox_kix_ptrace_post_exec_trap(uiox_kix_psa_proc_t *child,
                           uiox_kix_psa_proc_t *debugger);

/* Detach a trace slot and release the child's entry. */
void uiox_kix_ptrace_free_child(uiox_kix_psa_proc_t *child);

#endif /* UIOX_PTRACE_H */
