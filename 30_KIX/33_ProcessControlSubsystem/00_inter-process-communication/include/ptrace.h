#ifndef UIOX_PTRACE_H
#define UIOX_PTRACE_H

#include "ipc_types.h"

/* ─────────────────────────────────────────────────────────────
 * ptrace request codes
 * ───────────────────────────────────────────────────────────── */
typedef enum {
    PTRACE_TRACEME    = 0,  /* child: consent to be traced        */
    PTRACE_PEEKDATA   = 1,  /* debugger: read word from tracee VA */
    PTRACE_POKEDATA   = 2,  /* debugger: write word to tracee VA  */
    PTRACE_CONT       = 3,  /* debugger: resume tracee            */
    PTRACE_KILL       = 4,  /* debugger: kill tracee              */
    PTRACE_SINGLESTEP = 5,  /* debugger: single-step tracee       */
    PTRACE_GETREGS    = 6,  /* debugger: read register set        */
    PTRACE_SETREGS    = 7   /* debugger: write register set       */
} PtraceRequest;

/* ─────────────────────────────────────────────────────────────
 * Simulated register set
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    uint64_t pc;    /* program counter                            */
    uint64_t sp;    /* stack pointer                              */
    uint64_t regs[8];
} RegSet;

/* ─────────────────────────────────────────────────────────────
 * Trace state of a single tracee
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    SimProcess *tracee;
    SimProcess *debugger;
    bool        active;
    RegSet      saved_regs;
    uint8_t     mem[256];   /* simulated virtual address space    */
} TraceState;

/* ─────────────────────────────────────────────────────────────
 * ptrace API
 * ───────────────────────────────────────────────────────────── */
void ptrace_init(void);

/*
 * ptrace()
 *
 * PTRACE_TRACEME:
 *   Child sets its own trace bit; kernel will send SIGTRAP after exec.
 *
 * PTRACE_PEEKDATA / PTRACE_POKEDATA:
 *   4 context switches to transfer one word (debugger→kernel→tracee
 *   →kernel→debugger).
 *
 * PTRACE_CONT:
 *   Resume tracee from its current PC.
 *
 * Returns 0 on success, -1 on error.
 */
int ptrace(PtraceRequest req, SimProcess *debugger,
           SimProcess *tracee, uint64_t addr, uint64_t *data);

/* Spawn a child, set its trace bit, exec its image */
SimProcess *ptrace_spawn_child(SimProcess *debugger, const char *image);

/* Called by kernel after exec when trace bit is set */
void ptrace_post_exec_trap(SimProcess *child, SimProcess *debugger);

#endif /* UIOX_PTRACE_H */
