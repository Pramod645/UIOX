/*
 * 30KIX/33PCS/00IPC/src/shm.c
 *
 * Shared memory — Bach's Algorithms 5-8.
 *
 * ── version 3.0.0: what changed, and what did not ───────────────────
 * Almost nothing.  This file never took a SimProcess * — it carries pid
 * COPIES only, and those were already int.  What it needed was the
 * header's include to follow ipc_types.h's change, and that is done.
 *
 * It is recorded here because it is worth knowing which files a
 * process-model merge does NOT touch: shm.c and socket.c are the two in
 * this layer that only ever referenced a pid by value.
 *
 * ── the lazy allocation, which is the design worth keeping ──────────
 * uiox_kix_shm_get records a name and a size; the backing store is claimed on the
 * FIRST uiox_kix_shm_at.  Bach does it that way because a region may be created
 * and never attached, and allocating 64 KB for a name nobody uses is a
 * waste in a kernel with no swap for its own data.
 *
 * ── detach does not free ────────────────────────────────────────────
 * uiox_kix_shm_dt decrements the count and records dtime.  The data survives until
 * IPC_RMID, which is what makes shared memory USEFUL: a writer can
 * detach and a reader attach later to find the message.  The old version
 * printed this correctly and kept it; it is called out here because it
 * is the behaviour most often got wrong when people port this API.
 *
 * @version 3.0.0  @date 2026-09-30
 */
#include "../include/uiox_kix_shm.h"
#include "uiox_klibc.h"

/* Wall clock, maintained by 01_schedular's timekeeping. */
extern volatile uiox_uint64_t jiffies;

/* ── Static backing store ────────────────────────────────────────────
 * One 64 KB slot per region.  A flat array rather than a heap, and
 * per-slot rather than packed, so freeing a slot cannot fragment the
 * store. */
static uiox_uint8_t s_shm_pool[IPC_MAX_SHM][IPC_MAX_SHM_SIZE];
static uiox_uint8_t s_shm_pool_used[IPC_MAX_SHM];

static uiox_uint8_t *shm_mem_alloc(int slot, uiox_size_t size)
{
    uiox_uint64_t n;
    uiox_uint8_t *p;

    if (slot < 0 || slot >= IPC_MAX_SHM)          return (uiox_uint8_t *)0;
    if (size == 0u || size > IPC_MAX_SHM_SIZE)    return (uiox_uint8_t *)0;
    if (s_shm_pool_used[slot])                    return (uiox_uint8_t *)0;

    s_shm_pool_used[slot] = 1;

    p = s_shm_pool[slot];
    n = size;
    while (n--) *p++ = 0u;          /* zeroed on hand-out, not on free */
    return s_shm_pool[slot];
}

static void shm_mem_free(int slot)
{
    uiox_uint64_t n;
    uiox_uint8_t *p;

    if (slot < 0 || slot >= IPC_MAX_SHM) return;

    /* Clear the whole slot, not just the region's size — a later region
     * of a different size must not inherit bytes from this one. */
    p = s_shm_pool[slot];
    n = IPC_MAX_SHM_SIZE;
    while (n--) *p++ = 0u;

    s_shm_pool_used[slot] = 0;
}

/* ── Region table ────────────────────────────────────────────────────── */
static ShmRegion regions[IPC_MAX_SHM];

/* ── uiox_kix_shm_init ────────────────────────────────────────────────────────── */
void uiox_kix_shm_init(void)
{
    int i;
    for (i = 0; i < IPC_MAX_SHM; i++) {
        memset(&regions[i], 0, sizeof regions[i]);
        s_shm_pool_used[i] = 0;
    }
}

/* ── Algorithm 5 — uiox_kix_shm_get ────────────────────────────────────────────
 * Name and size, no storage.  IPC_CREAT required to create; the key
 * search runs first so a second uiox_kix_shm_get finds the existing region. */
int uiox_kix_shm_get(int key, uiox_size_t size, int flag)
{
    int i;

    for (i = 0; i < IPC_MAX_SHM; i++) {
        if (regions[i].active && regions[i].perm.key == key) {
            if (flag & IPC_EXCL) return -1;
            return i;
        }
    }

    if (!(flag & IPC_CREAT)) return -1;
    if (size == 0u || size > IPC_MAX_SHM_SIZE) return -1;

    for (i = 0; i < IPC_MAX_SHM; i++) {
        if (!regions[i].active) {
            regions[i].active        = true;
            regions[i].perm.key      = key;
            regions[i].perm.mode     = (uiox_uint16_t)(flag & 0x1FF);
            regions[i].size          = size;
            regions[i].mem           = (uiox_uint8_t *)0;
            regions[i].mem_allocated = false;
            regions[i].attach_count  = 0;
            regions[i].ctime         = (time_t)jiffies;
            return i;
        }
    }
    return -1;   /* no free slots */
}

/* ── Algorithm 6 — uiox_kix_shm_at ─────────────────────────────────────────────
 * Attach, claiming storage on the first attach.
 *
 * With a hint, the address is rounded DOWN to a page boundary: a mapping
 * must begin where a page begins, and rounding up could overlap the
 * range that follows. */
void *uiox_kix_shm_at(int shmid, void *va_hint, int flags,
            ShmAttach attaches[], int *attach_count)
{
    ShmRegion *r;
    ShmAttach *a;
    void      *va;

    if (shmid < 0 || shmid >= IPC_MAX_SHM) return (void *)0;
    if (!regions[shmid].active) return (void *)0;
    if (!attaches || !attach_count) return (void *)0;
    if (*attach_count >= MAX_SHM_ATTACHES) return (void *)0;

    r = &regions[shmid];

    if (va_hint) {
        uiox_uintptr_t aligned =
            ((uiox_uintptr_t)va_hint) & ~(uiox_uintptr_t)(UIOX_PAGE_SIZE - 1u);
        va = (void *)aligned;
    } else {
        if (!r->mem_allocated) {
            r->mem = shm_mem_alloc(shmid, r->size);
            if (!r->mem) return (void *)0;   /* pool exhausted */
            r->mem_allocated = true;
        }
        va = r->mem;
    }

    a = &attaches[*attach_count];
    a->shmid  = shmid;
    a->va     = va;
    a->rdonly = (flags & SHM_RDONLY) != 0;
    a->active = true;

    (*attach_count)++;
    r->attach_count++;
    r->atime    = (time_t)jiffies;
    r->last_pid = 0;    /* a real kernel records the attaching pid here */

    return va;
}

/* ── Algorithm 7 — uiox_kix_shm_dt ─────────────────────────────────────────────
 * Detach.  The region and its data survive — see the banner. */
int uiox_kix_shm_dt(void *va, ShmAttach attaches[], int *attach_count)
{
    int i;

    if (!attaches || !attach_count) return -1;

    for (i = 0; i < *attach_count; i++) {
        if (attaches[i].active && attaches[i].va == va) {
            int shmid = attaches[i].shmid;

            attaches[i].active = false;

            if (shmid >= 0 && shmid < IPC_MAX_SHM) {
                ShmRegion *r = &regions[shmid];
                if (r->attach_count > 0) r->attach_count--;
                r->dtime = (time_t)jiffies;
            }
            return 0;
        }
    }
    return -1;   /* not attached through this table */
}

/* ── Algorithm 8 — uiox_kix_shm_ctl ────────────────────────────────────────────
 * IPC_RMID is the only command that releases the backing store — which
 * is the whole difference from uiox_kix_shm_dt. */
int uiox_kix_shm_ctl(int shmid, int cmd, ShmRegion *buf)
{
    ShmRegion *r;

    if (shmid < 0 || shmid >= IPC_MAX_SHM) return -1;
    if (!regions[shmid].active) return -1;

    r = &regions[shmid];

    switch (cmd) {
    case IPC_STAT:
        if (buf) *buf = *r;
        return 0;

    case IPC_SET:
        if (buf) {
            r->perm.mode = buf->perm.mode;
            r->ctime     = (time_t)jiffies;
        }
        return 0;

    case IPC_RMID:
        if (r->mem_allocated) {
            shm_mem_free(shmid);
            r->mem           = (uiox_uint8_t *)0;
            r->mem_allocated = false;
        }
        memset(r, 0, sizeof *r);
        return 0;

    default:
        return -1;
    }
}
