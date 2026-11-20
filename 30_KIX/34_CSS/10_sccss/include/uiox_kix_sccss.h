/*
 * 34_CSS/10_sccss/include/uiox_kix_sccss.h
 *
 * SCCSS — System Call Communication and Sensors System.
 * The CSS-side syscall BOUNDARY.
 *
 *   above : 10Arch's arch_sys_call_dispatch()
 *   below : 34_CSS's modules — every class under 01_Com, 02_Sensors
 *           and 03_NonSensors, each in its own directory
 *
 * ── naming ──────────────────────────────────────────────────────────
 *   layer    SCCSS        (folder 10_sccss)
 *   functions uiox_kix_css_<verb>()
 *   macros   SCCSS_<NAME>
 *   guard    UIOX_KIX_SCCCS_H
 *
 * The function prefix tracks the FOLDER's meaning, the same way
 * 33_PCS/40psa's files are uiox_kix_psa_*.  One rule, two layers.
 *
 * ── how this sits beside FS, and where it differs ───────────────────
 * 32_FS and 34_CSS are the same shape: one syscall arm over a set of
 * layers that sit BEHIND it.
 *
 *   32_FS/10_scfs      the file arm     over 00_bcache · 01_fsa · 10_unfs
 *   34_CSS/10_sccss    the CSS arm      over 01_Com · 02_Sensors · 03_NonSensors
 *
 * The difference is what sits behind them.
 *
 * FS's layers are SHARED.  Every file call goes through the same inode
 * cache, the same buffer cache, the same block map.  So uiox_kix_scfs.h
 * declares ~90 prototypes: it is the one place where those shared layers
 * meet the syscall numbers.
 *
 * CSS's families are SEPARATE.  Ethernet's socket state has nothing to do
 * with Bluetooth's ACL queue, which has nothing to do with the camera's
 * frame ring.  There is no shared callee layer under this arm, so there
 * is no shared callee surface to declare — each family declares its own
 * entry points and defines them in its own directory.
 *
 * That is why this header has no prototypes in it.
 *
 * ── why the user-address gate is NOT here ───────────────────────────
 * uiox_kix_scpcs.h declares check_user_ptr, copy_to_user and
 * copy_from_user.  uiox_kix_scfs.h declares none of them.  The
 * difference is what a call's parameters ARE:
 *
 *   SCPCS  a call carries a user ADDRESS as a parameter —
 *          clock_settime(id, tp), gettimeofday(tv, tz),
 *          sigprocmask(how, set, oldset).  The kernel must move data
 *          across the boundary, so the gate is needed, and it is shared
 *          across all 25 process calls.
 *
 *   SCFS   a call's parameters map onto a function's parameters
 *          directly — open(path, flags, perm), read(fd, buf, count).
 *          The boundary work happens inside the call.  Nothing to gate
 *          at the arm.
 *
 * A CSS call is the SCPCS case or the SCFS case depending on the FAMILY,
 * not on the arm.  eth's bind(fd, addr) reads a caller's sockaddr;
 * mic's read copies PCM into a caller buffer; camera's frames are
 * physical addresses with no user pointer at all.
 *
 * So a family that dereferences a user pointer declares its OWN gate, in
 * its own header, beside the call that uses it — and that gate's window
 * bounds must be character-for-character 50_scpcs's, or two subsystems
 * will disagree about which addresses are acceptable.
 *
 * ── what this header does NOT declare ───────────────────────────────
 * uiox_kix_css_dispatch.  SCiX reaches it with a hand-written extern in
 * 30_KIX/common/uiox_kix_router.c, the same way it declares
 * uiox_kix_scfs_dispatch and uiox_kix_scpcs_dispatch — neither of those
 * is declared in its own layer's header either.  The reg-context mirror
 * is likewise local to the .c.
 *
 * @version 1.0.0  @date 2026-11-20
 */
#ifndef UIOX_KIX_SCCCS_H
#define UIOX_KIX_SCCCS_H

#include "uiox_klibc.h"
#include "uiox_base_types.h"     /* uiox_int64_t, uiox_uint64_t,
                                  * uiox_uintptr_t                      */

/* ── Result codes ────────────────────────────────────────────────────
 * Bach's kernel returns -1 and sets u.u_error; a freestanding kernel
 * with no u area returns the code directly.  The numbers are the
 * conventional ones so user space needs no translation table.
 *
 * The distinction between these matters: ENOSYS reports a missing
 * implementation, EINVAL blames the caller's argument, ENOMEM says the
 * kernel tried and ran out.  A caller debugging a failure acts
 * differently on each.
 *
 * Only codes ANY CSS call may return are listed.  A family with a
 * vocabulary of its own — a device-specific refusal, a bus error, a
 * media-not-present — declares that code in its own header, beside the
 * call that produces it, rather than here. */
#define SCCSS_EOK          ((uiox_int64_t)  0)   /* success                */
#define SCCSS_EPERM        ((uiox_int64_t) -1)   /* operation not permitted*/
#define SCCSS_ENOENT       ((uiox_int64_t) -2)   /* no such entry          */
#define SCCSS_ESRCH        ((uiox_int64_t) -3)   /* no such process        */
#define SCCSS_EINTR        ((uiox_int64_t) -4)   /* interrupted by a signal*/
#define SCCSS_EIO          ((uiox_int64_t) -5)   /* I/O error              */
#define SCCSS_ENXIO        ((uiox_int64_t) -6)   /* no such device or addr */
#define SCCSS_E2BIG        ((uiox_int64_t) -7)   /* argument list too long */
#define SCCSS_ENOEXEC      ((uiox_int64_t) -8)   /* exec format error      */
#define SCCSS_EBADF        ((uiox_int64_t) -9)   /* bad descriptor         */
#define SCCSS_ECHILD       ((uiox_int64_t)-10)   /* no child processes     */
#define SCCSS_EAGAIN       ((uiox_int64_t)-11)   /* try again (would block)*/
#define SCCSS_ENOMEM       ((uiox_int64_t)-12)   /* out of memory          */
#define SCCSS_EACCES       ((uiox_int64_t)-13)   /* permission denied      */
#define SCCSS_EFAULT       ((uiox_int64_t)-14)   /* bad address            */
#define SCCSS_EBUSY        ((uiox_int64_t)-16)   /* device or resource busy*/
#define SCCSS_EEXIST       ((uiox_int64_t)-17)   /* entry exists           */
#define SCCSS_ENODEV       ((uiox_int64_t)-19)   /* no such device         */
#define SCCSS_EINVAL       ((uiox_int64_t)-22)   /* invalid argument       */
#define SCCSS_ENFILE       ((uiox_int64_t)-23)   /* table overflow         */
#define SCCSS_EMFILE       ((uiox_int64_t)-24)   /* too many open descrs   */
#define SCCSS_ENOTTY       ((uiox_int64_t)-25)   /* not a terminal         */
#define SCCSS_ENOSPC       ((uiox_int64_t)-28)   /* no space left          */
#define SCCSS_ESPIPE       ((uiox_int64_t)-29)   /* illegal seek           */
#define SCCSS_EPIPE        ((uiox_int64_t)-32)   /* broken pipe            */
#define SCCSS_ERANGE       ((uiox_int64_t)-34)   /* result out of range    */
#define SCCSS_ENOSYS       ((uiox_int64_t)-38)   /* not implemented        */
#define SCCSS_ENOTEMPTY    ((uiox_int64_t)-39)   /* directory not empty    */
#define SCCSS_ELOOP        ((uiox_int64_t)-40)   /* too many symbolic links*/
#define SCCSS_EAFNOSUPPORT ((uiox_int64_t)-47)   /* address family bad     */
#define SCCSS_ENOTCONN     ((uiox_int64_t)-57)   /* socket not connected   */
#define SCCSS_ETIMEDOUT    ((uiox_int64_t)-60)   /* operation timed out    */
#define SCCSS_ECONNREFUSED ((uiox_int64_t)-61)   /* connection refused     */
#define SCCSS_EOPNOTSUPP   ((uiox_int64_t)-95)   /* operation not supported*/

/* ── The number space the table models, and the trap's arity ─────────
 * SCCSS_MAX spans the NUMBER space, not a count — 10_scfs chose 512 for
 * the same reason, so a table row's position and its BSD number are the
 * same thing by construction.
 *
 * SCCSS_MAX_ARGS is what the trap handler saves, not what every call
 * uses: a call needing three of the six ignores the rest. */
#define SCCSS_MAX        512u
#define SCCSS_MAX_ARGS     6u

/* ── The entry struct a dispatch row is built from ───────────────────
 * fn names a FAMILY's entry point; name is the call's spelling for a
 * diagnostic.  The two columns are filled independently, which is what
 * lets the dispatcher tell:
 *
 *   named, no fn   -> a real BSD call with no implementation  -> ENOSYS
 *   no name        -> a number this subsystem does not own    -> EINVAL
 *
 * A row set to (void *)0 is a number held with no implementation.  The
 * rows are written out rather than omitted so a reader can see the
 * whole surface and which parts of it exist.
 *
 * This type lives here rather than in the .c so a family's writer has it
 * in scope when uncommenting that family's rows.  That is the one place
 * this differs from 10_scfs, which keeps its equivalent local. */
typedef uiox_int64_t (*uiox_kix_css_fn_t)(uiox_uintptr_t, uiox_uintptr_t,
                                          uiox_uintptr_t, uiox_uintptr_t,
                                          uiox_uintptr_t, uiox_uintptr_t);

typedef struct uiox_kix_css_entry {
    uiox_kix_css_fn_t fn;
    const char       *name;
} uiox_kix_css_entry_t;

/* ── Introspection, so a diagnostic does not have to guess ────────────
 * Returns the call's spelling for a number this arm owns, or NULL when
 * the number is out of range.  Same shape as uiox_kix_scfs_name(). */
const char *uiox_kix_css_name(uiox_uint64_t nr);

#endif /* UIOX_KIX_SCCCS_H */
