/*
 * 30_KIX/33_PCS/src/uiox_vdso.c
 *
 * The vDSO page — allocation, mapping, the writer, and the readers.
 *
 * ── WHERE THE TIME COMES FROM ─────────────────────────────────────────
 * Not from this file.  01_schedular owns the clock:
 *
 *   uiox_kix_clock.c        advances xtime on every tick
 *   uiox_kix_timekeeping.c  selects the source and calibrates it
 *   uiox_kix_time_service.c reads xtime and serves the syscalls above it
 *
 * sched_types.h declares `extern XTime xtime`.  This file is a
 * PUBLISHER, not a clock: uiox_vdso_update() is called from the tick with
 * values already computed.
 *
 * ── WHY THE WRITER CANNOT BE INTERRUPTED BY A READER ──────────────────
 *   writer:  seq++   (odd — a reader will retry)
 *            ...store the fields...
 *            seq++   (even — the data is consistent)
 *
 * The stores between the two increments must not reorder across them;
 * the barrier enforces that.
 *
 * ── WHAT IS NOT IMPLEMENTED ───────────────────────────────────────────
 * uiox_vdso_map() returns 0.  It needs uiox_mm_map_user_phys() to be
 * real, which needs 02_MemMngnt to export uiox_vma_alloc(),
 * uiox_pte_set() and uiox_tlb_flush_user().  The page CAN be allocated
 * and written today; it cannot yet be put in front of a process.
 *
 * @version 1.0.0  @date 2026-10-03
 */
#include "uiox_vdso.h"

extern uintptr_t uiox_mm_phys_alloc(size_t size);

extern uintptr_t uiox_mm_map_user_phys(struct uiox_proc *proc,
                                       uintptr_t         va_hint,
                                       uintptr_t         pa,
                                       size_t            size,
                                       uint32_t          prot);

#define UIOX_VDSO_PROT_READ   0x01u

static uiox_vdso_data_t *s_vdso_page = (uiox_vdso_data_t *)0;
static uintptr_t         s_vdso_pa   = 0u;
static uintptr_t         s_vdso_va   = 0u;

static void vdso_barrier(void)
{
    __asm__ volatile("" ::: "memory");
}

int uiox_vdso_init(void)
{
    unsigned char *p;
    unsigned long  i;

    if (s_vdso_page) return 0;

    s_vdso_pa = uiox_mm_phys_alloc(sizeof(uiox_vdso_data_t));
    if (s_vdso_pa == 0u) return -1;

    /* A vDSO whose `seq` started odd would make every reader retry
     * forever, so zero rather than trusting the allocator. */
    p = (unsigned char *)(uintptr_t)s_vdso_pa;
    for (i = 0u; i < sizeof(uiox_vdso_data_t); i++) p[i] = 0u;

    s_vdso_page = (uiox_vdso_data_t *)(uintptr_t)s_vdso_pa;

    s_vdso_page->version = UIOX_VDSO_VERSION;
    s_vdso_page->seq     = 0u;

    return 0;
}

uintptr_t uiox_vdso_map(struct uiox_proc *proc)
{
    if (!proc)          return 0u;
    if (!s_vdso_page)   return 0u;

#if 0
    /* Uncomment when uiox_mm_map_user_phys() is real.  A FIXED hint is
     * wanted: every process must find the page at the SAME address,
     * because 50_UIX's libc caches one value. */
    s_vdso_va = uiox_mm_map_user_phys(proc,
                                      (uintptr_t)0x000000000000E000UL,
                                      s_vdso_pa,
                                      sizeof(uiox_vdso_data_t),
                                      UIOX_VDSO_PROT_READ);
    return s_vdso_va;
#else
    (void)proc;
    return 0u;
#endif
}

uintptr_t uiox_vdso_base(void)
{
    return s_vdso_va;
}

void uiox_vdso_update(uint64_t wall_ns, uint64_t mono_ns, uint64_t ticks)
{
    uiox_vdso_data_t *d = s_vdso_page;

    if (!d) return;

    d->seq = d->seq + 1u;          /* open: odd = write in progress */
    vdso_barrier();

    d->wall_ns    = wall_ns;
    d->mono_ns    = mono_ns;
    d->tick_count = ticks;

    vdso_barrier();                /* the reader's check depends on this */
    d->seq = d->seq + 1u;          /* close */
}

/* Bounded: an unbounded loop would spin forever against a writer that
 * died between its two increments — a hung process is worse than stale
 * time. */
#define UIOX_VDSO_RETRY_MAX   64u

int uiox_vdso_read(uiox_vdso_data_t *out)
{
    uiox_vdso_data_t *d = s_vdso_page;
    uint32_t attempts;

    if (!out || !d) return -1;

    for (attempts = 0u; attempts < UIOX_VDSO_RETRY_MAX; attempts++) {
        uint32_t s1;
        uint32_t s2;
        uint64_t w;
        uint64_t m;
        uint64_t t;

        s1 = d->seq;
        if (s1 & 1u) continue;

        vdso_barrier();

        w = d->wall_ns;
        m = d->mono_ns;
        t = d->tick_count;

        vdso_barrier();

        s2 = d->seq;
        if (s1 != s2) continue;

        out->seq           = s1;
        out->version       = d->version;
        out->wall_ns       = w;
        out->mono_ns       = m;
        out->tick_count    = t;
        out->tz_offset_min = d->tz_offset_min;
        return 0;
    }

    return -1;
}

int uiox_vdso_read_wall_ns(uint64_t *out)
{
    uiox_vdso_data_t snap;

    if (!out) return -1;
    if (uiox_vdso_read(&snap) != 0) return -1;

    *out = snap.wall_ns;
    return 0;
}

int uiox_vdso_read_mono_ns(uint64_t *out)
{
    uiox_vdso_data_t snap;

    if (!out) return -1;
    if (uiox_vdso_read(&snap) != 0) return -1;

    *out = snap.mono_ns;
    return 0;
}
