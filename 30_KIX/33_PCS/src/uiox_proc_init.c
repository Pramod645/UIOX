/*
 * 30_KIX/33_PCS/src/uiox_proc_init.c
 *
 * uiox_proc_init() — single entry point called by uiox_kernel_main()
 * to bring the process-control subsystem online.
 *
 * Sequence
 * ────────
 *  1. Physical memory     (uiox_mm_init)
 *  2. Idle task  PID 0    (uiox_task_init + g_idle_task)
 *  3. Scheduler           (uiox_sched_init)
 *  4. Timer / jiffies     (uiox_timer_init)
 *  5. Syscall table       (uiox_sys_call_init)
 *
 * @version 2.0.0  @date 2026-07-23
 */

#include "../../../../50_UIX/00_libs/00_uixlibs/sys/uix_types.h"
#include "../40_procStruct/include/uiox_task.h"

/* ── Symbols from sibling translation units ─────────────────────────── */
extern void uiox_mm_init(uix_uint64_t dram_base, uix_uint64_t dram_size);
extern void uiox_sched_init(void);
extern void uiox_timer_init(void);
extern void uiox_sys_call_init(void);

/* ── Weak platform hooks — override from BSP / platform layer ────────── */
__attribute__((weak)) uix_uint64_t uiox_plat_dram_base(void)
{
    /* Default: ARM QEMU virt 256 MB base. Override with real platform value. */
    return 0x40000000ULL;
}

__attribute__((weak)) uix_uint64_t uiox_plat_dram_size(void)
{
    /* Default: 64 MB — conservative, avoids touching DTB/stack regions. */
    return 0x04000000ULL;
}

/* ── Idle task kernel stack ──────────────────────────────────────────── */
#define UIOX_IDLE_STACK_SZ  4096u
static uix_uint8_t s_idle_stack[UIOX_IDLE_STACK_SZ]
    __attribute__((aligned(16)));

/* ────────────────────────────────────────────────────────────────────
 * uiox_proc_init — bring up the PCS subsystem.
 *
 * Called once from uiox_kernel_main() after BSP init completes.
 * Returns 0 on success, -1 on fatal error.
 * ──────────────────────────────────────────────────────────────────── */
int uiox_proc_init(void)
{
    /* ── 1. Physical memory manager ────────────────────────────────── */
    uiox_mm_init(uiox_plat_dram_base(), uiox_plat_dram_size());

    /* ── 2. Idle task (PID 0) ──────────────────────────────────────── */
    uiox_task_init(&g_idle_task,
                   (uix_pid_t)UIOX_PID_IDLE,
                   (uiox_task_t *)0,
                   UIOX_PRIO_MAX);
    g_idle_task.p_state  = UIOX_TASK_RUNNING;
    g_idle_task.p_kstack = (uix_uintptr_t)s_idle_stack;
    g_idle_task.p_ksp    = (uix_uintptr_t)(s_idle_stack + UIOX_IDLE_STACK_SZ);
    g_current            = &g_idle_task;

    /* ── 3. Scheduler ──────────────────────────────────────────────── */
    uiox_sched_init();

    /* ── 4. Timer / jiffies ────────────────────────────────────────── */
    uiox_timer_init();

    /* ── 5. Syscall dispatch table ─────────────────────────────────── */
    uiox_sys_call_init();

    return 0;
}
