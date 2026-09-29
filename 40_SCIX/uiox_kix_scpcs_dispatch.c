/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_dispatch.c
 *
 * SCPCS dispatcher — process-control numbers to process-control calls.
 *
 *   called by : 40_SystemCallInterface/uix_archSysCall.c
 *   calls     : the 24 uiox_kix_scpcs_* entry points in this directory
 *
 * ── numbering ───────────────────────────────────────────────────────
 * The numbers are the BSD ones from 50_UIX's uix_sys.h, NOT numbers
 * invented here.  SCiX decides which subsystem owns a number; this file
 * decides which FUNCTION within the subsystem.
 *
 * That split matters: SCiX needs to know only "20 belongs to process
 * control", and this file knows "20 is getpid".  A number added to the
 * process group is one line here and one line in SCiX's ownership test,
 * and neither layer has to learn the other's whole table.
 *
 * ── the shape of a call ─────────────────────────────────────────────
 * A processor does not pass arguments as C parameters.  It traps with
 * the number in a register and up to six argument registers, which is
 * why the entry point takes an ARRAY-shaped argument list: SCiX fills
 * a0..a5 from wherever the ABI puts them.  A wrapper needing three of
 * the six ignores the rest — the count is validated, the extras are not.
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
 * Passing NULL for regs skips the write-back and returns the value, so
 * a test harness with no arch layer underneath can still call this.
 *
 * ── three failures that are NOT the same ────────────────────────────
 *   EINVAL  the number names no call in this subsystem at all
 *   ENOSYS  the number names a REAL call with no implementation yet
 *   EFAULT  the call exists, was invoked, and rejected a user pointer
 *
 * The last is returned by the wrapper itself.  The first two are this
 * file's job to distinguish, and collapsing them would erase the only
 * signal separating "you asked for nothing" from "not finished yet".
 *
 * @version 2.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scpcs.h"
#include "uix_sys.h"                 /* the BSD numbers                */
#include "uiox_kix_psa_context.h"    /* reg context, by pointer only   */

/* ── Thin adapters ───────────────────────────────────────────────────
 * The wrappers have named parameters — getpid(void), exit(code) — and a
 * table wants one uniform signature.  These adapters are that bridge,
 * and they are the ONLY place the six-slot shape is unpacked, so a
 * wrapper keeps readable parameter names and a reader can see which
 * slot each argument comes from.
 *
 * static: nothing outside this file calls them.  An exported symbol per
 * syscall would defeat the point of one table.
 * ──────────────────────────────────────────────────────────────────── */
#define SCPCS_ADAPTER(adapter, target, n)                                 \
    static int64_t adapter(uiox_uintptr_t a0, uiox_uintptr_t a1,          \
                           uiox_uintptr_t a2, uiox_uintptr_t a3,          \
                           uiox_uintptr_t a4, uiox_uintptr_t a5)          \
    {                                                                    \
        (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;       \
        return scpcs_bind_##n(target, a0, a1, a2, a3, a4, a5);            \
    }

/* One helper per arity, so the macro above stays readable and the
 * argument-slot mapping is written once per count rather than per call. */
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

/* ── getpid, getppid, getuid, geteuid, getgid, getegid, getpgrp, pause ──
 * Eight calls, one shape.  Each needs its own adapter only because a
 * table entry is a function pointer and there is no way to parameterise
 * a pointer by result. */
SCPCS_ADAPTER(scpcs_call_get_pid,   (void *)uiox_kix_scpcs_get_pid,  0)
SCPCS_ADAPTER(scpcs_call_get_ppid,  (void *)uiox_kix_scpcs_get_ppid, 0)
SCPCS_ADAPTER(scpcs_call_get_uid,   (void *)uiox_kix_scpcs_get_uid,  0)
SCPCS_ADAPTER(scpcs_call_get_euid,  (void *)uiox_kix_scpcs_get_euid, 0)
SCPCS_ADAPTER(scpcs_call_get_gid,   (void *)uiox_kix_scpcs_get_gid,  0)
SCPCS_ADAPTER(scpcs_call_get_egid,  (void *)uiox_kix_scpcs_get_egid, 0)
SCPCS_ADAPTER(scpcs_call_get_pgrp,  (void *)uiox_kix_scpcs_get_pgrp, 0)
SCPCS_ADAPTER(scpcs_call_pause,     (void *)uiox_kix_scpcs_pause,    0)
SCPCS_ADAPTER(scpcs_call_fork,      (void *)uiox_kix_scpcs_fork,     0)

/* ── one-argument ─────────────────────────────────────────────────── */
SCPCS_ADAPTER(scpcs_call_exit,     (void *)uiox_kix_scpcs_exit,     1)
SCPCS_ADAPTER(scpcs_call_brk,      (void *)uiox_kix_scpcs_brk,      1)
SCPCS_ADAPTER(scpcs_call_set_uid,  (void *)uiox_kix_scpcs_set_uid,  1)
SCPCS_ADAPTER(scpcs_call_nice,     (void *)uiox_kix_scpcs_nice,     1)
SCPCS_ADAPTER(scpcs_call_times,    (void *)uiox_kix_scpcs_times,    1)
SCPCS_ADAPTER(scpcs_call_raise,    (void *)uiox_kix_scpcs_raise,    1)
SCPCS_ADAPTER(scpcs_call_sig_pending,
              (void *)uiox_kix_scpcs_sig_pending, 1)
SCPCS_ADAPTER(scpcs_call_alarm,    (void *)uiox_kix_scpcs_alarm,    1)

/* ── two-argument ─────────────────────────────────────────────────── */
SCPCS_ADAPTER(scpcs_call_kill,       (void *)uiox_kix_scpcs_kill,       2)
SCPCS_ADAPTER(scpcs_call_nano_sleep, (void *)uiox_kix_scpcs_nano_sleep, 2)
SCPCS_ADAPTER(scpcs_call_clock_get_time,
              (void *)uiox_kix_scpcs_clock_get_time, 2)

/* ── three-argument ───────────────────────────────────────────────── */
SCPCS_ADAPTER(scpcs_call_execve,      (void *)uiox_kix_scpcs_execve,      3)
SCPCS_ADAPTER(scpcs_call_wait_pid,    (void *)uiox_kix_scpcs_wait_pid,    3)
SCPCS_ADAPTER(scpcs_call_sig_action,  (void *)uiox_kix_scpcs_sig_action,  3)
SCPCS_ADAPTER(scpcs_call_sig_procmask,
              (void *)uiox_kix_scpcs_sig_procmask, 3)

/* ── The table ───────────────────────────────────────────────────────
 * Designated initialisers, so a row's position and its BSD number are
 * the same thing by construction — the number cannot drift from its
 * slot without the compiler refusing.
 *
 * Rows are written in NUMERIC order, which is why they do not group
 * neatly: BSD spread each family across the number space, and a reader
 * looking up number 122 should find it where 122 lives. */
typedef struct uiox_kix_scpcs_entry {
    int64_t (*fn)(uiox_uintptr_t, uiox_uintptr_t, uiox_uintptr_t,
                  uiox_uintptr_t, uiox_uintptr_t, uiox_uintptr_t);
    const char *name;
} uiox_kix_scpcs_entry_t;

#define UIOX_KIX_SCPCS_NR_MAX 256

static const uiox_kix_scpcs_entry_t
    uiox_kix_scpcs_table[UIOX_KIX_SCPCS_NR_MAX] =
{
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
    [SYS_CLOCK_GETTIME]   = { scpcs_call_clock_get_time, "clock_gettime"  },
    [SYS_NANOSLEEP]       = { scpcs_call_nano_sleep,     "nanosleep"      },
    [SYS_KILL]            = { scpcs_call_kill,           "kill"           },
    [SYS_SETGID]          = { scpcs_call_set_uid,        "setgid"         },
    [SYS_SETPRIORITY]     = { scpcs_call_nice,           "setpriority"    },
    [SYS_TIMES]           = { scpcs_call_times,          "times"          },
    [SYS_SIGSUSPEND]      = { scpcs_call_pause,          "sigsuspend"     },
    [SYS_GETPGID]         = { scpcs_call_get_pgrp,       "getpgid"        },
    [SYS_GETPPID]         = { scpcs_call_get_ppid,       "getppid"        },
    [SYS_ALARM]           = { scpcs_call_alarm,          "alarm"          }
};

/* ── The number this subsystem is asked about, and what it answers ─── */

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
    if (e->fn == (void *)0)
        return SCPCS_EINVAL;    /* not ours — SCiX should not have sent it */

    rv = e->fn(a0, a1, a2, a3, a4, a5);

    if (regs) {
        uiox_kix_psa_reg_context_t *rc = (uiox_kix_psa_reg_context_t *)regs;
        if (rv < 0) {
            rc->rc_r0    = (uint32_t)(-rv);   /* positive error number */
            rc->rc_r1    = 0u;
            rc->rc_carry = 1u;                /* the flag marks it     */
        } else {
            rc->rc_r0    = (uint32_t)((uint64_t)rv & 0xFFFFFFFFu);
            rc->rc_r1    = (uint32_t)((uint64_t)rv >> 32);
            rc->rc_carry = 0u;
        }
    }
    return rv;
}

/* ── Introspection, for a diagnostic that must not read the table ──── */
const char *uiox_kix_scpcs_name(uiox_uint64_t nr)
{
    if (nr >= (uiox_uint64_t)UIOX_KIX_SCPCS_NR_MAX) return (const char *)0;
    return uiox_kix_scpcs_table[nr].name;
}
