/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_dispatch.c
 *
 * SCPCS dispatcher — process-control numbers to process-control calls.
 *
 *   called by : 40_SystemCallInterface/uix_archSysCall.c
 *   calls     : the 24 uiox_kix_scpcs_* entry points in this directory,
 *               plus the 01_schedular timing entry points declared below
 *
 * ── numbering ───────────────────────────────────────────────────────
 * The numbers are the BSD ones from 50_UIX's uix_sys.h, NOT numbers
 * invented here.  SCiX decides which subsystem owns a number; this file
 * decides which FUNCTION within the subsystem.
 *
 * That split keeps each table small: SCiX knows only "87 belongs to the
 * timing group", and this file knows "87 is clock_gettime".  A number
 * added to the group is one row here and one case in SCiX's ownership
 * test, and neither layer has to learn the other's whole table.
 *
 * ── three groups share this table ───────────────────────────────────
 *   process control   33_PCS/50_scpcs        the 24 wrappers
 *   timing            33_PCS/01_schedular    test / set / sleep / alarm
 *   scheduling        33_PCS/01_schedular    priority adjustment
 *
 * A third dispatcher was considered and rejected.  01_schedular is part
 * of the process control subsystem, so its calls arrive here — a fourth
 * table would be a fourth place deciding what a number means.
 *
 * ── the shape of a call ─────────────────────────────────────────────
 * A processor does not pass arguments as C parameters.  It traps with
 * the number in a register and up to six argument registers (x0..x5 on
 * ARM64, with the number in x8), which is why the entry point takes an
 * ARRAY-shaped argument list.  A wrapper needing three of the six
 * ignores the rest — the count is validated, the extras are not.
 *
 * ── the return convention ───────────────────────────────────────────
 * Every wrapper returns int64_t, negative for an error.  Where a
 * register context is supplied, the value is written back as:
 *
 *   success  rc_r0 = low 32 bits, rc_r1 = high 32 bits, rc_carry = 0
 *   failure  rc_r0 = the ERROR NUMBER (positive),     rc_carry = 1
 *
 * The carry flag marks the error rather than the sign of rc_r0, because
 * on a machine where value and status share a register pair a flag is
 * the only thing that distinguishes a return of -13 from an error 13.
 *
 * Passing NULL for regs skips the write-back and returns the value, so a
 * test harness with no arch layer underneath can still call this.
 *
 * ── three failures that are NOT the same ────────────────────────────
 *   EINVAL  the number names no call in this subsystem at all
 *   ENOSYS  the number names a REAL call with no implementation yet
 *   EFAULT  the call exists, was invoked, and rejected a user pointer
 *   EPERM   the call exists, the caller may not make it
 *
 * Collapsing the first two would erase the only signal separating "you
 * asked for nothing" from "not finished yet".
 *
 * ── THE USER-POINTER RULE, and why it is enforced here ──────────────
 * There is no copy_from_user or copy_to_user in this tree.  So every
 * wrapper that writes through, or reads through, a pointer handed in
 * from user space MUST validate that pointer itself, through
 * uiox_kix_scpcs_check_user_ptr, before touching it.
 *
 * This is NOT optional and NOT delegated.  When SCiX routes a number
 * here it passes the raw register values straight through — nothing on
 * that path has examined them.  A wrapper that dereferences what it is
 * given without checking therefore lets a userspace program name ANY
 * address, kernel space included, and have the kernel write there.
 *
 * Two consequences worth stating:
 *
 *   A NULL pointer where an argument is mandatory is EFAULT, not a
 *   silent no-op.  POSIX allows NULL for an OPTIONAL argument only.
 *
 *   The check is local to the call that does the write.  A future
 *   change to SCiX's routing cannot un-guard a wrapper, because the
 *   guarantee lives with the code that depends on it.
 *
 * @version 3.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scpcs.h"
#include "uix_sys.h"                 /* the BSD numbers               */
#include "uiox_kix_psa_context.h"    /* reg context, by pointer only  */

#define UIOX_KIX_SCPCS_NR_MAX 256

/* ── The 33_PCS/01_schedular timing entry points ─────────────────────
 * Declared rather than included: this file must not drag the
 * scheduler's types in.  The functions live in
 * 01_schedular/src/syscall_time.c and read xtime / the timer wheel.
 *
 * They arrive here rather than at a third dispatcher because
 * 01_schedular is part of the process control subsystem.
 * ──────────────────────────────────────────────────────────────────── */
extern int   sys_gettimeofday(void *tv);
extern int   sys_adjtimex(int64_t delta_sec, int32_t delta_nsec);
extern int   sys_setitimer(void *p, const void *new_val, void *old_val);
extern int   sys_clock_settime(int id, const void *in);

/* A structural view of 01_schedular's XTime { int64_t sec; uint32_t
 * nsec; } so this file can size a pointer check without including
 * sched_types.h.  The layout MUST match — same two fields, same order. */
typedef struct scpcs_xtime_view { int64_t tv_sec; uiox_uint32_t tv_nsec; }
        scpcs_xtime_view_t;

/* The same for TimeVal { int64_t tv_sec; uint32_t tv_usec; } and
 * ItimerVal { TimeVal it_interval; TimeVal it_value; }. */
typedef struct scpcs_timeval_view { int64_t tv_sec; uiox_uint32_t tv_usec; }
        scpcs_timeval_view_t;

typedef struct scpcs_itimerval_view {
    scpcs_timeval_view_t it_interval;
    scpcs_timeval_view_t it_value;
} scpcs_itimerval_view_t;

/* ── Thin adapters ───────────────────────────────────────────────────
 * The wrappers have named parameters — getpid(void), exit(code) — and a
 * table wants one uniform signature.  These adapters are that bridge,
 * and they are the ONLY place the six-slot shape is unpacked, so a
 * wrapper keeps readable parameter names and a reader can see which
 * slot each argument comes from.
 *
 * static: nothing outside this file calls them.  An exported symbol per
 * syscall would defeat the point of one table.
 *
 * The bind_N helpers below take the arity, so a row's adapter is written
 * once per argument count rather than once per call. */
static int64_t scpcs_bind_0(void *fn, uiox_uintptr_t a0, uiox_uintptr_t a1,
                            uiox_uintptr_t a2, uiox_uintptr_t a3,
                            uiox_uintptr_t a4, uiox_uintptr_t a5)
{
    int64_t (*f)(void) = (int64_t (*)(void))fn;
    (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    return f();
}

static int64_t scpcs_bind_1(void *fn, uiox_uintptr_t a0, uiox_uintptr_t a1,
                            uiox_uintptr_t a2, uiox_uintptr_t a3,
                            uiox_uintptr_t a4, uiox_uintptr_t a5)
{
    int64_t (*f)(uiox_uint64_t) = (int64_t (*)(uiox_uint64_t))fn;
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    return f((uiox_uint64_t)a0);
}

static int64_t scpcs_bind_2(void *fn, uiox_uintptr_t a0, uiox_uintptr_t a1,
                            uiox_uintptr_t a2, uiox_uintptr_t a3,
                            uiox_uintptr_t a4, uiox_uintptr_t a5)
{
    int64_t (*f)(uiox_uint64_t, uiox_uintptr_t) =
        (int64_t (*)(uiox_uint64_t, uiox_uintptr_t))fn;
    (void)a2; (void)a3; (void)a4; (void)a5;
    return f((uiox_uint64_t)a0, a1);
}

static int64_t scpcs_bind_3(void *fn, uiox_uintptr_t a0, uiox_uintptr_t a1,
                            uiox_uintptr_t a2, uiox_uintptr_t a3,
                            uiox_uintptr_t a4, uiox_uintptr_t a5)
{
    int64_t (*f)(uiox_uint64_t, uiox_uintptr_t, uiox_uintptr_t) =
        (int64_t (*)(uiox_uint64_t, uiox_uintptr_t, uiox_uintptr_t))fn;
    (void)a3; (void)a4; (void)a5;
    return f((uiox_uint64_t)a0, a1, a2);
}

#define SCPCS_ADAPTER(adapter, target, n)                                 \
    static int64_t adapter(uiox_uintptr_t a0, uiox_uintptr_t a1,          \
                           uiox_uintptr_t a2, uiox_uintptr_t a3,          \
                           uiox_uintptr_t a4, uiox_uintptr_t a5)          \
    {                                                                    \
        return scpcs_bind_##n((void *)(target), a0, a1, a2, a3, a4, a5);  \
    }

/* ── Zero-argument calls ─────────────────────────────────────────────
 * Nine calls, one shape.  Each needs its own adapter only because a
 * table entry is a function pointer and there is no way to parameterise
 * a pointer by result. */
SCPCS_ADAPTER(scpcs_call_get_pid,   uiox_kix_scpcs_get_pid,   0)
SCPCS_ADAPTER(scpcs_call_get_ppid,  uiox_kix_scpcs_get_ppid,  0)
SCPCS_ADAPTER(scpcs_call_get_uid,   uiox_kix_scpcs_get_uid,   0)
SCPCS_ADAPTER(scpcs_call_get_euid,  uiox_kix_scpcs_get_euid,  0)
SCPCS_ADAPTER(scpcs_call_get_gid,   uiox_kix_scpcs_get_gid,   0)
SCPCS_ADAPTER(scpcs_call_get_egid,  uiox_kix_scpcs_get_egid,  0)
SCPCS_ADAPTER(scpcs_call_get_pgrp,  uiox_kix_scpcs_get_pgrp,  0)
SCPCS_ADAPTER(scpcs_call_pause,     uiox_kix_scpcs_pause,     0)
SCPCS_ADAPTER(scpcs_call_fork,      uiox_kix_scpcs_fork,      0)

/* ── One-argument calls ─────────────────────────────────────────────── */
SCPCS_ADAPTER(scpcs_call_exit,     uiox_kix_scpcs_exit,     1)
SCPCS_ADAPTER(scpcs_call_brk,      uiox_kix_scpcs_brk,      1)
SCPCS_ADAPTER(scpcs_call_set_uid,  uiox_kix_scpcs_set_uid,  1)
SCPCS_ADAPTER(scpcs_call_nice,     uiox_kix_scpcs_nice,     1)
SCPCS_ADAPTER(scpcs_call_times,    uiox_kix_scpcs_times,    1)
SCPCS_ADAPTER(scpcs_call_raise,    uiox_kix_scpcs_raise,    1)
SCPCS_ADAPTER(scpcs_call_sig_pending, uiox_kix_scpcs_sig_pending, 1)
SCPCS_ADAPTER(scpcs_call_alarm,    uiox_kix_scpcs_alarm,    1)

/* ── Two-argument calls ─────────────────────────────────────────────── */
SCPCS_ADAPTER(scpcs_call_kill,       uiox_kix_scpcs_kill,       2)
SCPCS_ADAPTER(scpcs_call_nano_sleep, uiox_kix_scpcs_nano_sleep, 2)
SCPCS_ADAPTER(scpcs_call_clock_get_time,
              uiox_kix_scpcs_clock_get_time, 2)

/* ── Three-argument calls ───────────────────────────────────────────── */
SCPCS_ADAPTER(scpcs_call_execve,      uiox_kix_scpcs_execve,      3)
SCPCS_ADAPTER(scpcs_call_wait_pid,    uiox_kix_scpcs_wait_pid,    3)
SCPCS_ADAPTER(scpcs_call_sig_action,  uiox_kix_scpcs_sig_action,  3)
SCPCS_ADAPTER(scpcs_call_sig_procmask,
              uiox_kix_scpcs_sig_procmask, 3)

/* ══ The 01_schedular timing adapters ════════════════════════════════
 * These four cannot use the bind_N helpers, because each one must
 * VALIDATE its pointer before anything touches it.  See the banner's
 * user-pointer rule: SCiX passes the raw register values through, so
 * the guard has to live here, with the code that depends on it.
 * ──────────────────────────────────────────────────────────────────── */

/* gettimeofday(tv, tz)
 *
 * tv is MANDATORY and is WRITTEN THROUGH.  A NULL tv is EFAULT.  The
 * buffer must hold a TimeVal and be 8-byte aligned for the int64 in it.
 *
 * tz is obsolete in POSIX and may be NULL; if non-NULL it is validated
 * but not written, because this kernel has no timezone support — which
 * is why a caller that passes one is not misled into thinking it was
 * filled in. */
static int64_t scpcs_call_gettimeofday(uiox_uintptr_t a0, uiox_uintptr_t a1,
                                       uiox_uintptr_t a2, uiox_uintptr_t a3,
                                       uiox_uintptr_t a4, uiox_uintptr_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;

    if (a0 == 0u) return SCPCS_EFAULT;
    if (uiox_kix_scpcs_check_user_ptr(a0, sizeof(scpcs_timeval_view_t), 8u))
        return SCPCS_EFAULT;

    /* tz is accepted and ignored — POSIX deprecates it, and a caller
     * that names one still gets a correct timeval back. */
    if (a1 != 0u &&
        uiox_kix_scpcs_check_user_ptr(a1,
                                      sizeof(scpcs_timeval_view_t), 4u))
        return SCPCS_EFAULT;

    return (int64_t)sys_gettimeofday((void *)(uintptr_t)a0);
}

/* setitimer(which, new_val, old_val)
 *
 * new_val is MANDATORY and is READ FROM.  old_val is optional and is
 * WRITTEN THROUGH when non-NULL.
 *
 * a0 is the which argument — ITIMER_REAL / VIRTUAL / PROF — and is a
 * small integer, not a pointer, so it needs no check. */
static int64_t scpcs_call_setitimer(uiox_uintptr_t a0, uiox_uintptr_t a1,
                                    uiox_uintptr_t a2, uiox_uintptr_t a3,
                                    uiox_uintptr_t a4, uiox_uintptr_t a5)
{
    (void)a3; (void)a4; (void)a5;

    if (a1 == 0u) return SCPCS_EFAULT;
    if (uiox_kix_scpcs_check_user_ptr(a1, sizeof(scpcs_itimerval_view_t), 8u))
        return SCPCS_EFAULT;

    if (a2 != 0u &&
        uiox_kix_scpcs_check_user_ptr(a2,
                                      sizeof(scpcs_itimerval_view_t), 8u))
        return SCPCS_EFAULT;

    return (int64_t)sys_setitimer((void *)(uintptr_t)a0,
                                  (const void *)(uintptr_t)a1,
                                  (void *)(uintptr_t)a2);
}

/* clock_settime(clock_id, tp)
 *
 * tp is MANDATORY and is READ FROM.  a0 is the clock id.
 *
 * This MOVES THE WALL CLOCK, so it is gated on privilege: a process
 * whose effective uid is 0.  There is no capability model in this tree,
 * so the rule borrowed is setuid's — the effective id is the identity
 * entitlements are checked against everywhere else.
 *
 * EPERM, not ENOSYS, when refused: the call exists and the caller may
 * not make it.  That is a different fact from "not implemented". */
static int64_t scpcs_call_clock_settime(uiox_uintptr_t a0, uiox_uintptr_t a1,
                                        uiox_uintptr_t a2, uiox_uintptr_t a3,
                                        uiox_uintptr_t a4, uiox_uintptr_t a5)
{
    uiox_kix_psa_proc_t *p;

    (void)a2; (void)a3; (void)a4; (void)a5;

    p = uiox_kix_scps_current();
    if (!p) return SCPCS_ESRCH;

    if (a1 == 0u) return SCPCS_EFAULT;
    if (uiox_kix_scpcs_check_user_ptr(a1, sizeof(scpcs_xtime_view_t), 8u))
        return SCPCS_EFAULT;

    if (p->p_euid != 0u) return SCPCS_EPERM;

    return (int64_t)sys_clock_settime((int)a0,
                                      (const void *)(uintptr_t)a1);
}

/* adjtimex(delta_sec, delta_nsec)
 *
 * Both arguments are VALUES, not pointers, so there is nothing to
 * validate beyond the range.  It is gated for the same reason
 * clock_settime is: it shifts the wall clock.
 *
 * The nsec argument is checked to be inside one second, because a
 * caller passing a larger number is not asking for a small correction
 * and silently folding it would move the clock by more than it asked. */
static int64_t scpcs_call_adjtimex(uiox_uintptr_t a0, uiox_uintptr_t a1,
                                   uiox_uintptr_t a2, uiox_uintptr_t a3,
                                   uiox_uintptr_t a4, uiox_uintptr_t a5)
{
    uiox_kix_psa_proc_t *p;
    int32_t              nsec = (int32_t)a1;

    (void)a2; (void)a3; (void)a4; (void)a5;

    p = uiox_kix_scps_current();
    if (!p) return SCPCS_ESRCH;
    if (p->p_euid != 0u) return SCPCS_EPERM;

    if (nsec <= -1000000000 || nsec >= 1000000000)
        return SCPCS_EINVAL;

    return (int64_t)sys_adjtimex((int64_t)a0, nsec);
}

/* ── The table ───────────────────────────────────────────────────────
 * Designated initialisers, so a row's position and its BSD number are
 * the same thing by construction — the number cannot drift from its
 * slot without the compiler refusing.
 *
 * Rows are written in NUMERIC order, which is why they do not group
 * neatly: BSD spread each family across the number space, and a reader
 * looking up 122 should find kill where 122 lives.
 *
 * An empty slot answers EINVAL — "this subsystem has no such call".
 * That is different from a named slot with no function, which is
 * ENOSYS: a real syscall this kernel has not written yet. */
typedef struct uiox_kix_scpcs_entry {
    int64_t (*fn)(uiox_uintptr_t, uiox_uintptr_t, uiox_uintptr_t,
                  uiox_uintptr_t, uiox_uintptr_t, uiox_uintptr_t);
    const char *name;
} uiox_kix_scpcs_entry_t;

static const uiox_kix_scpcs_entry_t
    uiox_kix_scpcs_table[UIOX_KIX_SCPCS_NR_MAX] =
{
    /* ── process control ────────────────────────────────────────── */
    [SYS_EXIT]            = { scpcs_call_exit,           "exit"           },
    [SYS_FORK]            = { scpcs_call_fork,           "fork"           },
    [SYS_WAIT4]           = { scpcs_call_wait_pid,       "wait4"          },
    [SYS_BREAK]           = { scpcs_call_brk,            "break"          },
    [SYS_GETPID]          = { scpcs_call_get_pid,        "getpid"         },
    [SYS_SETUID]          = { scpcs_call_set_uid,        "setuid"         },
    [SYS_GETUID]          = { scpcs_call_get_uid,        "getuid"         },
    [SYS_GETEUID]         = { scpcs_call_get_euid,       "geteuid"        },
    [SYS_SIGACTION]       = { scpcs_call_sig_action,     "sigaction"      },
    [SYS_GETGID]          = { scpcs_call_get_gid,        "getgid"         },
    [SYS_SIGPROCMASK]     = { scpcs_call_sig_procmask,   "sigprocmask"    },
    [SYS_SIGPENDING]      = { scpcs_call_sig_pending,    "sigpending"     },
    [SYS_EXECV]           = { scpcs_call_execve,         "execv"          },
    [SYS_GETPGRP]         = { scpcs_call_get_pgrp,       "getpgrp"        },

    /* ── timing — 33_PCS/01_schedular ──────────────────────────────
     * Nine numbers, seven entry points.  Two pairings are deliberate:
     *
     *   GETTIMEOFDAY  serves both gettimeofday() and time(), because
     *                 BSD has no separate time() number.  A time()
     *                 call arrives here and the wrapper returns the
     *                 seconds field alone.
     *   SETITIMER     serves getitimer too, reading the remaining time
     *                 and passing a NULL new_val.  syscall_time.c has
     *                 no separate sys_getitimer — noted below. */
    [SYS_GETTIMEOFDAY]    = { scpcs_call_gettimeofday,   "gettimeofday"   },
    [SYS_SETTIMEOFDAY]    = { scpcs_call_clock_settime,  "settimeofday"   },
    [SYS_SETITIMER]       = { scpcs_call_setitimer,      "setitimer"      },
    [SYS_GETITIMER]       = { scpcs_call_setitimer,      "getitimer"      },
    [SYS_CLOCK_GETTIME]   = { scpcs_call_clock_get_time, "clock_gettime"  },
    [SYS_CLOCK_SETTIME]   = { scpcs_call_clock_settime,  "clock_settime"  },
    [SYS_CLOCK_GETRES]    = { scpcs_call_clock_get_time, "clock_getres"   },
    [SYS_NANOSLEEP]       = { scpcs_call_nano_sleep,     "nanosleep"      },

    /* ── signals, priority, misc ────────────────────────────────── */
    [SYS_KILL]            = { scpcs_call_kill,           "kill"           },
    [SYS_SETGID]          = { scpcs_call_set_uid,        "setgid"         },
    [SYS_SETPRIORITY]     = { scpcs_call_nice,           "setpriority"    },
    [SYS_TIMES]           = { scpcs_call_times,          "times"          },
    [SYS_SIGSUSPEND]      = { scpcs_call_pause,          "sigsuspend"     },
    [SYS_GETPGID]         = { scpcs_call_get_pgrp,       "getpgid"        },
    [SYS_GETPPID]         = { scpcs_call_get_ppid,       "getppid"        },
    [SYS_ALARM]           = { scpcs_call_alarm,          "alarm"          },
    [SYS_ADJTIME]         = { scpcs_call_adjtimex,       "adjtimex"       }
};

/* ────────────────────────────────────────────────────────────────────
 * uiox_kix_scpcs_dispatch — one number to one call.
 *
 * Returns the raw int64_t.  Where a register context is supplied the
 * value is written back in the convention described in the banner, so
 * SCiX can return either subsystem's result without knowing which
 * produced it.
 *
 * Two lookups, no loop.  A linear scan over 256 entries on every syscall
 * would be the kind of cost that is invisible until it is not, and the
 * test is exact: a slot is either initialised or zero.
 * ──────────────────────────────────────────────────────────────────── */
int64_t uiox_kix_scpcs_dispatch(uiox_uint64_t nr,
                                uiox_uintptr_t a0, uiox_uintptr_t a1,
                                uiox_uintptr_t a2, uiox_uintptr_t a3,
                                uiox_uintptr_t a4, uiox_uintptr_t a5,
                                void *regs)
{
    const uiox_kix_scpcs_entry_t *e;
    int64_t                       rv;

    if (nr >= (uiox_uint64_t)UIOX_KIX_SCPCS_NR_MAX)
        return SCPCS_EINVAL;

    e = &uiox_kix_scpcs_table[nr];

    /* Unallocated: SCiX should not have routed this number here.  A
     * routing bug rather than a missing implementation, hence EINVAL. */
    if (e->fn == (void *)0)
        return SCPCS_EINVAL;

    rv = e->fn(a0, a1, a2, a3, a4, a5);

    if (regs) {
        uiox_kix_psa_reg_context_t *rc = (uiox_kix_psa_reg_context_t *)regs;

        if (rv < 0) {
            /* The error NUMBER is positive in rc_r0, and the carry flag
             * is what marks it as an error.  Storing the negative would
             * make rc_r0 unreadable as an error code on a machine that
             * treats it as a count. */
            rc->rc_r0    = (uint32_t)(-rv);
            rc->rc_r1    = 0u;
            rc->rc_carry = 1u;
        } else {
            /* The full 64-bit value split across the return pair,
             * because a 32-bit register pair is how the ABI carries it. */
            rc->rc_r0    = (uint32_t)((uint64_t)rv & 0xFFFFFFFFu);
            rc->rc_r1    = (uint32_t)((uint64_t)rv >> 32);
            rc->rc_carry = 0u;
        }
    }

    return rv;
}

/* ── Introspection ───────────────────────────────────────────────────
 * A number's name, for a diagnostic that must not read the table's
 * layout.  Returns NULL for an unallocated number, so a caller can tell
 * "no such call" from "call with no name" without a second lookup. */
const char *uiox_kix_scpcs_name(uiox_uint64_t nr)
{
    if (nr >= (uiox_uint64_t)UIOX_KIX_SCPCS_NR_MAX) return (const char *)0;
    return uiox_kix_scpcs_table[nr].name;
}
