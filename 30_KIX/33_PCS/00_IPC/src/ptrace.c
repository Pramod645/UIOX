#include "ptrace.h"
#include "../include/uiox_klibc.h"

/*
 * 30_KIX/33_PCS/00_IPC/src/ptrace.c
 *
 * Freestanding fixes (v2.0)
 * ─────────────────────────
 *   REMOVED: #include <stdio.h>  <stdlib.h>  <string.h>
 *            All provided through ptrace.h → ipc_types.h → uiox_klibc.h
 *
 *   FIXED: fprintf(stderr, ...) → printf(...)
 *   FIXED: calloc(1, sizeof *child) → static SimProcess pool
 *   FIXED: perror("calloc")       → printf("[ptrace] ERROR: ...")
 *
 * No algorithm changes — all ptrace logic identical to original.
 *
 * @version 2.0.0  @date 2026-07-23
 */

 /* ── Static SimProcess pool — replaces calloc/free ──────────────────── */
 #define SIM_PROC_POOL_SIZE  16
 
 static SimProcess s_proc_pool[SIM_PROC_POOL_SIZE];
 static uint8_t    s_proc_used[SIM_PROC_POOL_SIZE];
 static uint8_t    s_proc_pool_ready = 0;
 
 static void proc_pool_init(void)
 {
     if (!s_proc_pool_ready) {
         memset(s_proc_pool, 0, sizeof s_proc_pool);
         memset(s_proc_used, 0, sizeof s_proc_used);
         s_proc_pool_ready = 1;
     }
 }
 
 static SimProcess *proc_alloc(void)
 {
     uint32_t i;
     proc_pool_init();
     for (i = 0; i < SIM_PROC_POOL_SIZE; i++) {
         if (!s_proc_used[i]) {
             s_proc_used[i] = 1;
             memset(&s_proc_pool[i], 0, sizeof s_proc_pool[i]);
             return &s_proc_pool[i];
         }
     }
     return (SimProcess *)0;
 }
 
 static void proc_free(SimProcess *p)
 {
     uint32_t i;
     if (!p) return;
     for (i = 0; i < SIM_PROC_POOL_SIZE; i++) {
         if (&s_proc_pool[i] == p) {
             memset(p, 0, sizeof *p);
             s_proc_used[i] = 0;
             return;
         }
     }
 }
 
 /* ── Trace slot table ────────────────────────────────────────────────── */
 #define MAX_TRACED  8
 
 static TraceState trace_table[MAX_TRACED];
 static uint8_t    trace_init_done = 0;
 
 /* ── ptrace_init ─────────────────────────────────────────────────────── */
 void ptrace_init(void)
 {
     proc_pool_init();
     memset(trace_table, 0, sizeof trace_table);
     trace_init_done = 1;
     printf("[ptrace] init: max_traced=%d\n", MAX_TRACED);
 }
 
 /* ── Internal: find existing trace slot ──────────────────────────────── */
 static TraceState *get_trace_slot(SimProcess *tracee)
 {
     int i;
     for (i = 0; i < MAX_TRACED; i++)
         if (trace_table[i].active && trace_table[i].tracee == tracee)
             return &trace_table[i];
     return (TraceState *)0;
 }
 
 /* ── Internal: allocate a new trace slot ─────────────────────────────── */
 static TraceState *alloc_trace_slot(SimProcess *tracee, SimProcess *debugger)
 {
     int i;
     for (i = 0; i < MAX_TRACED; i++) {
         if (!trace_table[i].active) {
             trace_table[i].active   = true;
             trace_table[i].tracee   = tracee;
             trace_table[i].debugger = debugger;
             memset(trace_table[i].mem, 0xCC, sizeof trace_table[i].mem);
             return &trace_table[i];
         }
     }
     printf("[ptrace] ERROR: no free trace slots\n");
     return (TraceState *)0;
 }
 
 /* ── ptrace_post_exec_trap ───────────────────────────────────────────── */
 void ptrace_post_exec_trap(SimProcess *child, SimProcess *debugger)
 {
     /* Simulate: after exec(), kernel sends SIGTRAP to stop child so
        debugger can attach before the new image runs.                  */
     printf("[ptrace] SIGTRAP → pid=%d after exec (debugger pid=%d)\n",
            child->pid, debugger ? debugger->pid : -1);
     sim_sleep(child, EVENT_SOCKET_CONN);   /* reuse an event as "trap" */
 }
 
 /* ── ptrace_spawn_child ──────────────────────────────────────────────── */
 SimProcess *ptrace_spawn_child(SimProcess *debugger, const char *image)
 {
     /* was: calloc(1, sizeof *child) */
     SimProcess *child = proc_alloc();
     if (!child) {
         printf("[ptrace] ERROR: SimProcess pool exhausted\n"); /* was: perror */
         return (SimProcess *)0;
     }
     child->pid    = debugger->pid + 100;   /* simplified child PID */
     child->uid    = debugger->uid;
     child->gid    = debugger->gid;
     child->traced = true;
 
     alloc_trace_slot(child, debugger);
 
     printf("[ptrace] child pid=%d exec('%s')\n", child->pid, image);
     ptrace_post_exec_trap(child, debugger);
     return child;
 }
 
 /* ── ptrace_free_child ───────────────────────────────────────────────── */
 void ptrace_free_child(SimProcess *child)
 {
     if (!child) return;
     TraceState *ts = get_trace_slot(child);
     if (ts) ts->active = false;
     proc_free(child);   /* return slot to pool */
 }
 
 /* ── ptrace — main system call handler ───────────────────────────────── */
 int ptrace(PtraceRequest req, SimProcess *debugger,
            SimProcess *tracee, uint64_t addr, uint64_t *data)
 {
     switch (req) {
 
     /* ── TRACEME: child consents to being traced ─────────────────── */
     case PTRACE_TRACEME:
         tracee->traced = true;
         alloc_trace_slot(tracee, debugger);
         printf("[ptrace] TRACEME: pid=%d trace-bit set\n", tracee->pid);
         return 0;
 
     /* ── PEEKDATA: read one word from tracee address space ────────── */
     case PTRACE_PEEKDATA: {
         TraceState *ts = get_trace_slot(tracee);
         if (!ts || !tracee->traced) {
             printf("[ptrace] ERROR: PEEKDATA: not traced\n"); /* was: fprintf(stderr,...) */
             return -1;
         }
         if (addr >= sizeof ts->mem) {
             printf("[ptrace] ERROR: PEEKDATA: bad addr 0x%llx\n",
                    (unsigned long long)addr);
             return -1;
         }
         /* 4 context switches: debugger→kernel→tracee→kernel→debugger */
         printf("[ptrace] PEEKDATA: 4 context switches "
                "(debugger→kernel→tracee→kernel→debugger)\n");
         if (data) *data = ts->mem[addr];
         printf("[ptrace] PEEKDATA: addr=0x%llx  val=0x%llx\n",
                (unsigned long long)addr,
                (unsigned long long)(data ? *data : 0));
         return 0;
     }
 
     /* ── POKEDATA: write one word to tracee address space ──────────── */
     case PTRACE_POKEDATA: {
         TraceState *ts = get_trace_slot(tracee);
         if (!ts || !tracee->traced) {
             printf("[ptrace] ERROR: POKEDATA: not traced\n");
             return -1;
         }
         if (addr >= sizeof ts->mem) {
             printf("[ptrace] ERROR: POKEDATA: bad addr 0x%llx\n",
                    (unsigned long long)addr);
             return -1;
         }
         printf("[ptrace] POKEDATA: 4 context switches\n");
         if (data) ts->mem[addr] = (uint8_t)*data;
         printf("[ptrace] POKEDATA: addr=0x%llx  val=0x%llx\n",
                (unsigned long long)addr,
                (unsigned long long)(data ? *data : 0));
         return 0;
     }
 
     /* ── CONT: resume tracee ────────────────────────────────────────── */
     case PTRACE_CONT:
         printf("[ptrace] CONT: pid=%d resumed\n", tracee->pid);
         sim_wakeup(tracee, EVENT_SOCKET_CONN);
         return 0;
 
     /* ── KILL: terminate tracee ─────────────────────────────────────── */
     case PTRACE_KILL: {
         TraceState *ts = get_trace_slot(tracee);
         printf("[ptrace] KILL: pid=%d\n", tracee->pid);
         tracee->traced = false;
         if (ts) ts->active = false;
         return 0;
     }
 
     /* ── SINGLESTEP ─────────────────────────────────────────────────── */
     case PTRACE_SINGLESTEP: {
         TraceState *ts = get_trace_slot(tracee);
         if (!ts) return -1;
         ts->saved_regs.pc++;   /* simulate advancing one instruction */
         printf("[ptrace] SINGLESTEP: pid=%d  new_pc=0x%llx\n",
                tracee->pid, (unsigned long long)ts->saved_regs.pc);
         return 0;
     }
 
     /* ── GETREGS ────────────────────────────────────────────────────── */
     case PTRACE_GETREGS: {
         TraceState *ts = get_trace_slot(tracee);
         if (!ts) return -1;
         if (data) *data = ts->saved_regs.pc;
         printf("[ptrace] GETREGS: pid=%d  pc=0x%llx\n",
                tracee->pid, (unsigned long long)ts->saved_regs.pc);
         return 0;
     }
 
     /* ── SETREGS ────────────────────────────────────────────────────── */
     case PTRACE_SETREGS: {
         TraceState *ts = get_trace_slot(tracee);
         if (!ts || !data) return -1;
         ts->saved_regs.pc = *data;
         printf("[ptrace] SETREGS: pid=%d  new_pc=0x%llx\n",
                tracee->pid, (unsigned long long)*data);
         return 0;
     }
 
     default:
         printf("[ptrace] ERROR: unknown request %d\n", (int)req); /* was: fprintf(stderr,...) */
         return -1;
     }
 }
 