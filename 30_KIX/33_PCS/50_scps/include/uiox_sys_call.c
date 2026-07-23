/*
 * 30_KIX/33_PCS/50_scps/src/uiox_sys_call.c
 *
 * Syscall dispatch table.
 *
 * Provides sys_call_dispatch() — called by arch_sys_call_dispatch()
 * in 30_KIX/10Arch/<arch>/src/archruntime.c.
 *
 * Convention
 * ──────────
 *  sys_call_dispatch(uix_uint64_t nr, a0..a5) → uix_int64_t result
 *  Negative result = errno-style error code.
 *  Syscall numbers: uiox_sys_call_nr.h (single source of truth).
 *
 * All implementations are __attribute__((weak)) stubs.
 * Real implementations in uiox_fork.c, uiox_pipe.c etc. override at link.
 *
 * @version 2.0.0  @date 2026-07-23
 */

#include "../../../../50_UIX/00_libs/00_uixlibs/sys/uix_types.h"
#include "../40_procStruct/include/uiox_task.h"
#include "../include/uiox_sys_call_nr.h"

/* ── Error codes ────────────────────────────────────────────────────── */
#define SYS_EOK       ((uix_int64_t)  0)
#define SYS_EINVAL    ((uix_int64_t) -1)
#define SYS_ENOSYS    ((uix_int64_t) -2)
#define SYS_EFAULT    ((uix_int64_t) -3)
#define SYS_ENOMEM    ((uix_int64_t) -4)
#define SYS_EBADF     ((uix_int64_t) -5)
#define SYS_EAGAIN    ((uix_int64_t) -6)
#define SYS_ECHILD    ((uix_int64_t) -7)

/* ── Weak stub implementations ──────────────────────────────────────── */

__attribute__((weak)) uix_int64_t sys_exit(uix_uint64_t exit_code)
{
    if (g_current) {
        g_current->p_exit_code = (int)exit_code;
        g_current->p_state     = UIOX_TASK_ZOMBIE;
    }
    return SYS_EOK;
}

__attribute__((weak)) uix_int64_t sys_fork(void)
    { return SYS_ENOMEM; }

__attribute__((weak)) uix_int64_t sys_read(uix_uint64_t fd,
    uix_uintptr_t buf, uix_uint64_t count)
    { (void)fd; (void)buf; (void)count; return SYS_ENOSYS; }

__attribute__((weak)) uix_int64_t sys_write(uix_uint64_t fd,
    uix_uintptr_t buf, uix_uint64_t count)
    { (void)fd; (void)buf; (void)count; return SYS_ENOSYS; }

__attribute__((weak)) uix_int64_t sys_open(uix_uintptr_t path,
    uix_uint64_t flags, uix_uint64_t mode)
    { (void)path; (void)flags; (void)mode; return SYS_ENOSYS; }

__attribute__((weak)) uix_int64_t sys_close(uix_uint64_t fd)
    { (void)fd; return SYS_ENOSYS; }

__attribute__((weak)) uix_int64_t sys_wait_pid(uix_uint64_t pid,
    uix_uintptr_t status_ptr, uix_uint64_t opts)
    { (void)pid; (void)status_ptr; (void)opts; return SYS_ECHILD; }

__attribute__((weak)) uix_int64_t sys_execve(uix_uintptr_t path,
    uix_uintptr_t argv, uix_uintptr_t envp)
    { (void)path; (void)argv; (void)envp; return SYS_ENOSYS; }

__attribute__((weak)) uix_int64_t sys_get_pid(void)
    { return g_current ? (uix_int64_t)g_current->p_pid : 0; }

__attribute__((weak)) uix_int64_t sys_get_ppid(void)
    { return g_current ? (uix_int64_t)g_current->p_ppid : 0; }

__attribute__((weak)) uix_int64_t sys_brk(uix_uint64_t new_brk)
    { (void)new_brk; return SYS_ENOMEM; }

__attribute__((weak)) uix_int64_t sys_mmap(uix_uint64_t addr,
    uix_uint64_t len, uix_uint64_t prot, uix_uint64_t flags,
    uix_uint64_t fd, uix_uint64_t offset)
    { (void)addr;(void)len;(void)prot;(void)flags;(void)fd;(void)offset;
      return SYS_ENOMEM; }

__attribute__((weak)) uix_int64_t sys_munmap(uix_uint64_t addr,
    uix_uint64_t len)
    { (void)addr; (void)len; return SYS_ENOSYS; }

__attribute__((weak)) uix_int64_t sys_nano_sleep(uix_uintptr_t req,
    uix_uintptr_t rem)
    { (void)req; (void)rem; return SYS_ENOSYS; }

__attribute__((weak)) uix_int64_t sys_clock_get_time(uix_uint64_t clk_id,
    uix_uintptr_t tp_ptr)
    { (void)clk_id; (void)tp_ptr; return SYS_ENOSYS; }

__attribute__((weak)) uix_int64_t sys_kill(uix_uint64_t pid,
    uix_uint64_t sig)
    { (void)pid; (void)sig; return SYS_ENOSYS; }

__attribute__((weak)) uix_int64_t sys_sig_action(uix_uint64_t sig,
    uix_uintptr_t act, uix_uintptr_t old_act)
    { (void)sig; (void)act; (void)old_act; return SYS_ENOSYS; }

/* ── Handler function type ──────────────────────────────────────────── */
typedef uix_int64_t (*uiox_sys_call_fn_t)(uix_uint64_t a0, uix_uint64_t a1,
                                           uix_uint64_t a2, uix_uint64_t a3,
                                           uix_uint64_t a4, uix_uint64_t a5);

/* ── 6-arg shim wrappers ─────────────────────────────────────────────── */
#define _U(x) ((uix_uint64_t)(x))
#define _P(x) ((uix_uintptr_t)(x))

static uix_int64_t _wrap_exit(uix_uint64_t a0,uix_uint64_t a1,uix_uint64_t a2,uix_uint64_t a3,uix_uint64_t a4,uix_uint64_t a5){(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;return sys_exit(a0);}
static uix_int64_t _wrap_fork(uix_uint64_t a0,uix_uint64_t a1,uix_uint64_t a2,uix_uint64_t a3,uix_uint64_t a4,uix_uint64_t a5){(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;return sys_fork();}
static uix_int64_t _wrap_read(uix_uint64_t a0,uix_uint64_t a1,uix_uint64_t a2,uix_uint64_t a3,uix_uint64_t a4,uix_uint64_t a5){(void)a3;(void)a4;(void)a5;return sys_read(a0,_P(a1),a2);}
static uix_int64_t _wrap_write(uix_uint64_t a0,uix_uint64_t a1,uix_uint64_t a2,uix_uint64_t a3,uix_uint64_t a4,uix_uint64_t a5){(void)a3;(void)a4;(void)a5;return sys_write(a0,_P(a1),a2);}
static uix_int64_t _wrap_open(uix_uint64_t a0,uix_uint64_t a1,uix_uint64_t a2,uix_uint64_t a3,uix_uint64_t a4,uix_uint64_t a5){(void)a3;(void)a4;(void)a5;return sys_open(_P(a0),a1,a2);}
static uix_int64_t _wrap_close(uix_uint64_t a0,uix_uint64_t a1,uix_uint64_t a2,uix_uint64_t a3,uix_uint64_t a4,uix_uint64_t a5){(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;return sys_close(a0);}
static uix_int64_t _wrap_wait_pid(uix_uint64_t a0,uix_uint64_t a1,uix_uint64_t a2,uix_uint64_t a3,uix_uint64_t a4,uix_uint64_t a5){(void)a3;(void)a4;(void)a5;return sys_wait_pid(a0,_P(a1),a2);}
static uix_int64_t _wrap_execve(uix_uint64_t a0,uix_uint64_t a1,uix_uint64_t a2,uix_uint64_t a3,uix_uint64_t a4,uix_uint64_t a5){(void)a3;(void)a4;(void)a5;return sys_execve(_P(a0),_P(a1),_P(a2));}
static uix_int64_t _wrap_get_pid(uix_uint64_t a0,uix_uint64_t a1,uix_uint64_t a2,uix_uint64_t a3,uix_uint64_t a4,uix_uint64_t a5){(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;return sys_get_pid();}
static uix_int64_t _wrap_get_ppid(uix_uint64_t a0,uix_uint64_t a1,uix_uint64_t a2,uix_uint64_t a3,uix_uint64_t a4,uix_uint64_t a5){(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;return sys_get_ppid();}
static uix_int64_t _wrap_brk(uix_uint64_t a0,uix_uint64_t a1,uix_uint64_t a2,uix_uint64_t a3,uix_uint64_t a4,uix_uint64_t a5){(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;return sys_brk(a0);}
static uix_int64_t _wrap_mmap(uix_uint64_t a0,uix_uint64_t a1,uix_uint64_t a2,uix_uint64_t a3,uix_uint64_t a4,uix_uint64_t a5){return sys_mmap(a0,a1,a2,a3,a4,a5);}
static uix_int64_t _wrap_munmap(uix_uint64_t a0,uix_uint64_t a1,uix_uint64_t a2,uix_uint64_t a3,uix_uint64_t a4,uix_uint64_t a5){(void)a2;(void)a3;(void)a4;(void)a5;return sys_munmap(a0,a1);}
static uix_int64_t _wrap_nano_sleep(uix_uint64_t a0,uix_uint64_t a1,uix_uint64_t a2,uix_uint64_t a3,uix_uint64_t a4,uix_uint64_t a5){(void)a2;(void)a3;(void)a4;(void)a5;return sys_nano_sleep(_P(a0),_P(a1));}
static uix_int64_t _wrap_clock_get_time(uix_uint64_t a0,uix_uint64_t a1,uix_uint64_t a2,uix_uint64_t a3,uix_uint64_t a4,uix_uint64_t a5){(void)a2;(void)a3;(void)a4;(void)a5;return sys_clock_get_time(a0,_P(a1));}
static uix_int64_t _wrap_kill(uix_uint64_t a0,uix_uint64_t a1,uix_uint64_t a2,uix_uint64_t a3,uix_uint64_t a4,uix_uint64_t a5){(void)a2;(void)a3;(void)a4;(void)a5;return sys_kill(a0,a1);}
static uix_int64_t _wrap_sig_action(uix_uint64_t a0,uix_uint64_t a1,uix_uint64_t a2,uix_uint64_t a3,uix_uint64_t a4,uix_uint64_t a5){(void)a3;(void)a4;(void)a5;return sys_sig_action(a0,_P(a1),_P(a2));}
static uix_int64_t _wrap_enosys(uix_uint64_t a0,uix_uint64_t a1,uix_uint64_t a2,uix_uint64_t a3,uix_uint64_t a4,uix_uint64_t a5){(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;return SYS_ENOSYS;}

#undef _U
#undef _P

/* ── Dispatch table ─────────────────────────────────────────────────── */
static uiox_sys_call_fn_t s_sys_call_table[UIOX_SYS_NR_MAX];

/* ────────────────────────────────────────────────────────────────────
 * uiox_sys_call_init — populate the dispatch table.
 * Called from uiox_proc_init(). Idempotent.
 * ──────────────────────────────────────────────────────────────────── */
void uiox_sys_call_init(void)
{
    uix_uint32_t i;
    for (i = 0u; i < UIOX_SYS_NR_MAX; i++)
        s_sys_call_table[i] = _wrap_enosys;

    s_sys_call_table[UIOX_SYS_NR_EXIT]          = _wrap_exit;
    s_sys_call_table[UIOX_SYS_NR_FORK]          = _wrap_fork;
    s_sys_call_table[UIOX_SYS_NR_READ]          = _wrap_read;
    s_sys_call_table[UIOX_SYS_NR_WRITE]         = _wrap_write;
    s_sys_call_table[UIOX_SYS_NR_OPEN]          = _wrap_open;
    s_sys_call_table[UIOX_SYS_NR_CLOSE]         = _wrap_close;
    s_sys_call_table[UIOX_SYS_NR_WAIT_PID]      = _wrap_wait_pid;
    s_sys_call_table[UIOX_SYS_NR_EXECVE]        = _wrap_execve;
    s_sys_call_table[UIOX_SYS_NR_GET_PID]       = _wrap_get_pid;
    s_sys_call_table[UIOX_SYS_NR_GET_PPID]      = _wrap_get_ppid;
    s_sys_call_table[UIOX_SYS_NR_BRK]           = _wrap_brk;
    s_sys_call_table[UIOX_SYS_NR_MMAP]          = _wrap_mmap;
    s_sys_call_table[UIOX_SYS_NR_MUNMAP]        = _wrap_munmap;
    s_sys_call_table[UIOX_SYS_NR_NANO_SLEEP]    = _wrap_nano_sleep;
    s_sys_call_table[UIOX_SYS_NR_CLOCK_GET_TIME]= _wrap_clock_get_time;
    s_sys_call_table[UIOX_SYS_NR_KILL]          = _wrap_kill;
    s_sys_call_table[UIOX_SYS_NR_SIG_ACTION]    = _wrap_sig_action;
}

/* ────────────────────────────────────────────────────────────────────
 * sys_call_dispatch — called by arch_sys_call_dispatch() in archruntime.c
 *
 * @nr     syscall number (from UIOX_SYS_NR_* constants)
 * @a0-a5  arch register arguments
 *
 * Returns uix_int64_t; negative = error code.
 * ──────────────────────────────────────────────────────────────────── */
uix_int64_t sys_call_dispatch(uix_uint64_t nr,
                               uix_uint64_t a0, uix_uint64_t a1,
                               uix_uint64_t a2, uix_uint64_t a3,
                               uix_uint64_t a4, uix_uint64_t a5)
{
    if (nr >= UIOX_SYS_NR_MAX) return SYS_ENOSYS;
    return s_sys_call_table[nr](a0, a1, a2, a3, a4, a5);
}
