/*
 * 30_KIX/33_PCS/50_scps/src/uiox_sys_call.c
 *
 * Syscall dispatch table.
 *
 * Provides sys_call_dispatch() — called by arch_sys_call_dispatch()
 * in 30_KIX/10Arch/<arch>/src/archruntime.c.
 *
 * Include fixes (v2.1)
 * ────────────────────
 *   WAS: #include "../../../../50_UIX/.../uix_types.h"   deep relative, breaks -nostdinc
 *   WAS: #include "../40_procStruct/include/uiox_task.h" wrong relative from 50_scps/src/
 *   WAS: #include "../include/uiox_sys_call_nr.h"        wrong relative from compile CWD
 *   NOW: #include "uiox_klibc.h"          resolved via -I33_PCS/include
 *   NOW: #include "uiox_task.h"            resolved via -I40_procStruct/include
 *   NOW: #include "uiox_sys_call_nr.h"     resolved via -I50_scps/include
 *
 * Convention
 * ──────────
 *  sys_call_dispatch(uint64_t nr, a0..a5) → int64_t result
 *  Negative result = errno-style error code.
 *
 * All implementations are __attribute__((weak)) stubs.
 * Real implementations (uiox_fork.c, uiox_pipe.c …) override at link.
 *
 * @version 2.1.0  @date 2026-07-23
 */

 #include "uiox_klibc.h"          /* uint8/16/32/64_t, bool, NULL       */
 #include "uiox_task.h"            /* uiox_task_t, g_current             */
 #include "uiox_sys_call_nr.h"     /* UIOX_SYS_NR_* constants            */
 
 /* ── Error codes ────────────────────────────────────────────────────── */
 #define SYS_EOK       ((int64_t)  0)
 #define SYS_EINVAL    ((int64_t) -1)
 #define SYS_ENOSYS    ((int64_t) -2)
 #define SYS_EFAULT    ((int64_t) -3)
 #define SYS_ENOMEM    ((int64_t) -4)
 #define SYS_EBADF     ((int64_t) -5)
 #define SYS_EAGAIN    ((int64_t) -6)
 #define SYS_ECHILD    ((int64_t) -7)
 
 /* ── Weak stub implementations ──────────────────────────────────────── */
 
 __attribute__((weak)) int64_t sys_exit(uint64_t exit_code)
 {
     if (g_current) {
         g_current->p_exit_code = (int)exit_code;
         g_current->p_state     = UIOX_TASK_ZOMBIE;
     }
     return SYS_EOK;
 }
 
 __attribute__((weak)) int64_t sys_fork(void)
     { return SYS_ENOMEM; }
 
 __attribute__((weak)) int64_t sys_read(uint64_t fd,
     uintptr_t buf, uint64_t count)
     { (void)fd; (void)buf; (void)count; return SYS_ENOSYS; }
 
 __attribute__((weak)) int64_t sys_write(uint64_t fd,
     uintptr_t buf, uint64_t count)
     { (void)fd; (void)buf; (void)count; return SYS_ENOSYS; }
 
 __attribute__((weak)) int64_t sys_open(uintptr_t path,
     uint64_t flags, uint64_t mode)
     { (void)path; (void)flags; (void)mode; return SYS_ENOSYS; }
 
 __attribute__((weak)) int64_t sys_close(uint64_t fd)
     { (void)fd; return SYS_ENOSYS; }
 
 __attribute__((weak)) int64_t sys_wait_pid(uint64_t pid,
     uintptr_t status_ptr, uint64_t opts)
     { (void)pid; (void)status_ptr; (void)opts; return SYS_ECHILD; }
 
 __attribute__((weak)) int64_t sys_execve(uintptr_t path,
     uintptr_t argv, uintptr_t envp)
     { (void)path; (void)argv; (void)envp; return SYS_ENOSYS; }
 
 __attribute__((weak)) int64_t sys_get_pid(void)
     { return g_current ? (int64_t)g_current->p_pid : 0; }
 
 __attribute__((weak)) int64_t sys_get_ppid(void)
     { return g_current ? (int64_t)g_current->p_ppid : 0; }
 
 __attribute__((weak)) int64_t sys_brk(uint64_t new_brk)
     { (void)new_brk; return SYS_ENOMEM; }
 
 __attribute__((weak)) int64_t sys_mmap(uint64_t addr,
     uint64_t len, uint64_t prot, uint64_t flags,
     uint64_t fd, uint64_t offset)
     { (void)addr;(void)len;(void)prot;(void)flags;(void)fd;(void)offset;
       return SYS_ENOMEM; }
 
 __attribute__((weak)) int64_t sys_munmap(uint64_t addr, uint64_t len)
     { (void)addr; (void)len; return SYS_ENOSYS; }
 
 __attribute__((weak)) int64_t sys_nano_sleep(uintptr_t req, uintptr_t rem)
     { (void)req; (void)rem; return SYS_ENOSYS; }
 
 __attribute__((weak)) int64_t sys_clock_get_time(uint64_t clk_id,
     uintptr_t tp_ptr)
     { (void)clk_id; (void)tp_ptr; return SYS_ENOSYS; }
 
 __attribute__((weak)) int64_t sys_kill(uint64_t pid, uint64_t sig)
     { (void)pid; (void)sig; return SYS_ENOSYS; }
 
 __attribute__((weak)) int64_t sys_sig_action(uint64_t sig,
     uintptr_t act, uintptr_t old_act)
     { (void)sig; (void)act; (void)old_act; return SYS_ENOSYS; }
 
 /* ── Handler type ───────────────────────────────────────────────────── */
 typedef int64_t (*uiox_sys_call_fn_t)(uint64_t a0, uint64_t a1,
                                        uint64_t a2, uint64_t a3,
                                        uint64_t a4, uint64_t a5);
 
 /* ── 6-arg shim wrappers ─────────────────────────────────────────────── */
 #define _P(x) ((uintptr_t)(x))
 
 static int64_t _wrap_exit(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5){(void)a1;(void)a2;(void)a3;(void)a4;(void)a5; return sys_exit(a0);}
 static int64_t _wrap_fork(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5){(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5; return sys_fork();}
 static int64_t _wrap_read(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5){(void)a3;(void)a4;(void)a5; return sys_read(a0,_P(a1),a2);}
 static int64_t _wrap_write(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5){(void)a3;(void)a4;(void)a5; return sys_write(a0,_P(a1),a2);}
 static int64_t _wrap_open(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5){(void)a3;(void)a4;(void)a5; return sys_open(_P(a0),a1,a2);}
 static int64_t _wrap_close(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5){(void)a1;(void)a2;(void)a3;(void)a4;(void)a5; return sys_close(a0);}
 static int64_t _wrap_wait_pid(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5){(void)a3;(void)a4;(void)a5; return sys_wait_pid(a0,_P(a1),a2);}
 static int64_t _wrap_execve(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5){(void)a3;(void)a4;(void)a5; return sys_execve(_P(a0),_P(a1),_P(a2));}
 static int64_t _wrap_get_pid(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5){(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5; return sys_get_pid();}
 static int64_t _wrap_get_ppid(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5){(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5; return sys_get_ppid();}
 static int64_t _wrap_brk(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5){(void)a1;(void)a2;(void)a3;(void)a4;(void)a5; return sys_brk(a0);}
 static int64_t _wrap_mmap(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5){return sys_mmap(a0,a1,a2,a3,a4,a5);}
 static int64_t _wrap_munmap(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5){(void)a2;(void)a3;(void)a4;(void)a5; return sys_munmap(a0,a1);}
 static int64_t _wrap_nano_sleep(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5){(void)a2;(void)a3;(void)a4;(void)a5; return sys_nano_sleep(_P(a0),_P(a1));}
 static int64_t _wrap_clock_get_time(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5){(void)a2;(void)a3;(void)a4;(void)a5; return sys_clock_get_time(a0,_P(a1));}
 static int64_t _wrap_kill(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5){(void)a2;(void)a3;(void)a4;(void)a5; return sys_kill(a0,a1);}
 static int64_t _wrap_sig_action(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5){(void)a3;(void)a4;(void)a5; return sys_sig_action(a0,_P(a1),_P(a2));}
 static int64_t _wrap_enosys(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5){(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5; return SYS_ENOSYS;}
 
 #undef _P
 
 /* ── Dispatch table ─────────────────────────────────────────────────── */
 static uiox_sys_call_fn_t s_sys_call_table[UIOX_SYS_NR_MAX];
 
 /* ────────────────────────────────────────────────────────────────────
  * uiox_sys_call_init — populate the dispatch table.
  * Called from uiox_proc_init(). Idempotent.
  * ──────────────────────────────────────────────────────────────────── */
 void uiox_sys_call_init(void)
 {
     uint32_t i;
     for (i = 0; i < UIOX_SYS_NR_MAX; i++)
         s_sys_call_table[i] = _wrap_enosys;
 
     s_sys_call_table[UIOX_SYS_NR_EXIT]           = _wrap_exit;
     s_sys_call_table[UIOX_SYS_NR_FORK]           = _wrap_fork;
     s_sys_call_table[UIOX_SYS_NR_READ]           = _wrap_read;
     s_sys_call_table[UIOX_SYS_NR_WRITE]          = _wrap_write;
     s_sys_call_table[UIOX_SYS_NR_OPEN]           = _wrap_open;
     s_sys_call_table[UIOX_SYS_NR_CLOSE]          = _wrap_close;
     s_sys_call_table[UIOX_SYS_NR_WAIT_PID]       = _wrap_wait_pid;
     s_sys_call_table[UIOX_SYS_NR_EXECVE]         = _wrap_execve;
     s_sys_call_table[UIOX_SYS_NR_GET_PID]        = _wrap_get_pid;
     s_sys_call_table[UIOX_SYS_NR_GET_PPID]       = _wrap_get_ppid;
     s_sys_call_table[UIOX_SYS_NR_BRK]            = _wrap_brk;
     s_sys_call_table[UIOX_SYS_NR_MMAP]           = _wrap_mmap;
     s_sys_call_table[UIOX_SYS_NR_MUNMAP]         = _wrap_munmap;
     s_sys_call_table[UIOX_SYS_NR_NANO_SLEEP]     = _wrap_nano_sleep;
     s_sys_call_table[UIOX_SYS_NR_CLOCK_GET_TIME] = _wrap_clock_get_time;
     s_sys_call_table[UIOX_SYS_NR_KILL]           = _wrap_kill;
     s_sys_call_table[UIOX_SYS_NR_SIG_ACTION]     = _wrap_sig_action;
 }
 
 /* ────────────────────────────────────────────────────────────────────
  * sys_call_dispatch — called by arch_sys_call_dispatch() in archruntime.c
  *
  * @nr     syscall number (UIOX_SYS_NR_* from uiox_sys_call_nr.h)
  * @a0-a5  arch register arguments
  *
  * Returns int64_t; negative = error code.
  * ──────────────────────────────────────────────────────────────────── */
 int64_t sys_call_dispatch(uint64_t nr,
                            uint64_t a0, uint64_t a1,
                            uint64_t a2, uint64_t a3,
                            uint64_t a4, uint64_t a5)
 {
     if (nr >= UIOX_SYS_NR_MAX) return SYS_ENOSYS;
     return s_sys_call_table[nr](a0, a1, a2, a3, a4, a5);
 }
 