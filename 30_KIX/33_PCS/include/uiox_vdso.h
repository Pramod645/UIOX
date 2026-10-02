/*
 * 30_KIX/33_PCS/include/uiox_vdso.h
 *
 * The vDSO page — kernel-updated time, read by userspace with no trap.
 *
 * ── WHAT THIS IS ──────────────────────────────────────────────────────
 * ONE read-only page mapped into every process's address space.  The
 * kernel writes current time into it as the clock advances; a program
 * reads it directly, with no syscall and no privilege transition.
 *
 * It is the fourth way data crosses from kernel to user, and the only one
 * with no number in any syscall table: there is nothing to dispatch,
 * because nothing traps.
 *
 * ── WHY IT NEEDS A SEQLOCK, AND NOT JUST A STRUCT ─────────────────────
 * The kernel writes the page; userspace reads it; there is no lock
 * between them and there cannot be one — taking a lock would be the trap
 * the page exists to avoid.
 *
 * So the reader can observe a PARTIALLY UPDATED page.  The `seq` counter
 * is what makes a lock-free read correct:
 *
 *     do {
 *         s1 = seq;
 *         if (s1 & 1) continue;     ← a write is in progress
 *         copy the fields out
 *         s2 = seq;
 *     } while (s1 != s2);           ← it changed while we read
 *
 * ── THE LAYOUT IS ABI ─────────────────────────────────────────────────
 * Shared between the kernel and 50_UIX's libc, compiled separately.  A
 * field added, removed or reordered changes every offset and the
 * userspace side reads garbage with no diagnostic.  Append only, never
 * reorder, bump UIOX_VDSO_VERSION when the shape changes.
 *
 * @version 1.0.0  @date 2026-10-03
 */
#ifndef UIOX_VDSO_H
#define UIOX_VDSO_H

#include "uiox_klibc.h"

#define UIOX_VDSO_VERSION   1u

typedef struct uiox_vdso_data {
    volatile uint32_t seq;          /* seqlock: odd = write in progress  */
    volatile uint32_t version;      /* UIOX_VDSO_VERSION                 */

    /* ── the two clocks ──────────────────────────────────────────────
     * Both are nanoseconds since their epoch, derived from xtime, which
     * sched_types.h declares and 01_schedular advances per tick. */
    volatile uint64_t wall_ns;      /* CLOCK_REALTIME                    */
    volatile uint64_t mono_ns;      /* CLOCK_MONOTONIC                   */

    volatile uint64_t tick_count;   /* jiffies, for a coarse reader      */

    volatile int32_t  tz_offset_min;  /* minutes east of UTC             */

    volatile uint32_t _pad[3];
} uiox_vdso_data_t;

#define UIOX_VDSO_DATA_SIZE   (sizeof(uiox_vdso_data_t))

struct uiox_proc;

int       uiox_vdso_init(void);
uintptr_t uiox_vdso_map(struct uiox_proc *proc);
uintptr_t uiox_vdso_base(void);
void      uiox_vdso_update(uint64_t wall_ns, uint64_t mono_ns, uint64_t ticks);

int uiox_vdso_read(uiox_vdso_data_t *out);
int uiox_vdso_read_wall_ns(uint64_t *out);
int uiox_vdso_read_mono_ns(uint64_t *out);

#endif /* UIOX_VDSO_H */
