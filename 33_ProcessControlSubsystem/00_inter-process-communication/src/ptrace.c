#include "ptrace.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ─────────────────────────────────────────────────────────────
 * Trace-state table — one slot per active traced child
 * ───────────────────────────────────────────────────────────── */
#define MAX_TRACED 8
static TraceState trace_table[MAX_TRACED];

void ptrace_init(void)
{
    memset(trace_table, 0, sizeof trace_table);
    printf("[ptrace] init: max_traced=%d\n", MAX_TRACED);
}

/* ─────────────────────────────────────────────────────────────
 * Internal: find or allocate a trace slot for a tracee
 * ───────────────────────────────────────────────────────────── */
static TraceState *get_trace_slot(SimProcess *tracee)
{
    for (int i = 0; i < MAX_TRACED; i++)
        if (trace_table[i].active &&
            trace_table[i].tracee == tracee)
            return &trace_table[i];
    return NULL;
}

static TraceState *alloc_trace_slot(SimProcess *tracee,
                                    SimProcess *debugger)
{
    for (int i = 0; i < MAX_TRACED; i++) {
        if (!trace_table[i].active) {
            trace_table[i].active   = true;
            trace_table[i].tracee   = tracee;
            trace_table[i].debugger = debugger;
            memset(trace_table[i].mem, 0xCC, sizeof trace_table[i].mem);
            return &trace_table[i];
        }
    }
    return NULL;
}

/* ─────────────────────────────────────────────────────────────
 * ptrace_post_exec_trap
 *
 * After the tracee calls exec, the kernel:
 *   1. Finishes exec normally.
 *   2. Notes the trace bit — sends SIGTRAP to child.
 *   3. Child wakes parent from wait(), enters trace-sleep state,
 *      does a context switch.
 * ───────────────────────────────────────────────────────────── */
void ptrace_post_exec_trap(SimProcess *child, SimProcess *debugger)
{
    printf("[ptrace] exec complete for pid=%d — sending SIGTRAP\n",
           child->pid);
    printf("[ptrace] child pid=%d enters trace-sleep state\n", child->pid);

    /* Context switch sequence (4 switches to transfer one word):
     *   1. debugger → kernel (ptrace call)
     *   2. kernel   → tracee (wake tracee to reply)
     *   3. tracee   → kernel (tracee replies)
     *   4. kernel   → debugger (return answer)
     */
    printf("[ptrace] context switches: "
           "debugger→kernel→tracee→kernel→debugger (4 switches)\n");

    sim_sleep(child, EVENT_SOCKET_CONN);  /* trace-sleep */
    sim_wakeup(debugger, EVENT_SOCKET_CONN); /* wake waiting parent */
}

/* ─────────────────────────────────────────────────────────────
 * ptrace_spawn_child
 * ───────────────────────────────────────────────────────────── */
SimProcess *ptrace_spawn_child(SimProcess *debugger, const char *image)
{
    SimProcess *child = calloc(1, sizeof *child);
    if (!child) { perror("calloc"); return NULL; }

    child->pid    = debugger->pid + 100;  /* simplified child PID */
    child->uid    = debugger->uid;
    child->gid    = debugger->gid;
    child->traced = false;

    printf("[ptrace] debugger pid=%d spawned child pid=%d for image '%s'\n",
           debugger->pid, child->pid, image);

    /* Child calls PTRACE_TRACEME before exec */
    ptrace(PTRACE_TRACEME, debugger, child, 0, NULL);

    /* Simulate exec of the image */
    printf("[ptrace] child pid=%d exec('%s')\n", child->pid, image);
    ptrace_post_exec_trap(child, debugger);

    return child;
}

/* ─────────────────────────────────────────────────────────────
 * ptrace  — main system call handler
 * ───────────────────────────────────────────────────────────── */
int ptrace(PtraceRequest req, SimProcess *debugger,
           SimProcess *tracee, uint64_t addr, uint64_t *data)
{
    switch (req) {

    /* ── TRACEME: child consents to be traced ─────────────── */
    case PTRACE_TRACEME:
        tracee->traced = true;
        alloc_trace_slot(tracee, debugger);
        printf("[ptrace] TRACEME: pid=%d trace-bit set\n", tracee->pid);
        return 0;

    /* ── PEEKDATA: read one word from tracee address space ── */
    case PTRACE_PEEKDATA: {
        TraceState *ts = get_trace_slot(tracee);
        if (!ts || !tracee->traced) { fprintf(stderr,"[ptrace] PEEKDATA: not traced\n"); return -1; }
        if (addr >= sizeof ts->mem) { fprintf(stderr,"[ptrace] PEEKDATA: bad addr\n"); return -1; }

        /* 4 context switches */
        printf("[ptrace] PEEKDATA: 4 context switches "
               "(debugger→kernel→tracee→kernel→debugger)\n");
        if (data) *data = ts->mem[addr];
        printf("[ptrace] PEEKDATA: addr=0x%llx  val=0x%llx\n",
               (unsigned long long)addr,
               (unsigned long long)(data ? *data : 0));
        return 0;
    }

    /* ── POKEDATA: write one word to tracee address space ─── */
    case PTRACE_POKEDATA: {
        TraceState *ts = get_trace_slot(tracee);
        if (!ts || !tracee->traced) { fprintf(stderr,"[ptrace] POKEDATA: not traced\n"); return -1; }
        if (addr >= sizeof ts->mem) { fprintf(stderr,"[ptrace] POKEDATA: bad addr\n"); return -1; }

        printf("[ptrace] POKEDATA: 4 context switches\n");
        if (data) ts->mem[addr] = (uint8_t)*data;
        printf("[ptrace] POKEDATA: addr=0x%llx  val=0x%llx\n",
               (unsigned long long)addr,
               (unsigned long long)(data ? *data : 0));
        return 0;
    }

    /* ── CONT: resume tracee ──────────────────────────────── */
    case PTRACE_CONT:
        printf("[ptrace] CONT: pid=%d resumed\n", tracee->pid);
        sim_wakeup(tracee, EVENT_SOCKET_CONN);
        return 0;

    /* ── KILL: terminate tracee ───────────────────────────── */
    case PTRACE_KILL: {
        TraceState *ts = get_trace_slot(tracee);
        printf("[ptrace] KILL: pid=%d\n", tracee->pid);
        tracee->traced = false;
        if (ts) ts->active = false;
        return 0;
    }

    /* ── GETREGS ──────────────────────────────────────────── */
    case PTRACE_GETREGS: {
        TraceState *ts = get_trace_slot(tracee);
        if (!ts) return -1;
        if (data) {
            /* Pack PC into data[0] as a simplification */
            *data = ts->saved_regs.pc;
        }
        printf("[ptrace] GETREGS: pid=%d  pc=0x%llx\n",
               tracee->pid, (unsigned long long)ts->saved_regs.pc);
        return 0;
    }

    /* ── SETREGS ──────────────────────────────────────────── */
    case PTRACE_SETREGS: {
        TraceState *ts = get_trace_slot(tracee);
        if (!ts || !data) return -1;
        ts->saved_regs.pc = *data;
        printf("[ptrace] SETREGS: pid=%d  new_pc=0x%llx\n",
               tracee->pid, (unsigned long long)*data);
        return 0;
    }

    default:
        fprintf(stderr, "[ptrace] unknown request %d\n", req);
        return -1;
    }
}
