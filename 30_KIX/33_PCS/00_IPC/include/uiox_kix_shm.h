/*
 * 30KIX/33PCS/00IPC/include/uiox_kix_shm.h
 *
 * Shared memory — Bach's Algorithms 5-8.
 *
 * ── version 3.0.0 ───────────────────────────────────────────────────
 * This header names NO process type.  It never took a SimProcess * —
 * only pid COPIES (creator_pid, last_pid), which stay int for the same
 * reason msg.h's did: a region outlives the processes that attach to it,
 * so holding a pointer would be worse than holding a stale number.
 *
 * So the change here is smaller than in msg.h, sem.h or uiox_kix_ptrace.h: the
 * include of ipc_types.h is now enough, and no signature moved.
 *
 * ── the attach array, and why it stays the caller's ─────────────────
 * uiox_kix_shm_at takes `ShmAttach attaches[], int *attach_count` — the caller
 * supplies its own attachment table.  That is per-process state living
 * outside the process entry, and it could move onto
 * uiox_kix_psa_proc_t as another field.
 *
 * It is left as a parameter because nothing is blocked on it.  Every
 * field added to the process entry this session — p_alarm_expire,
 * p_alarm_active, p_swap_time, p_swap_blk — had a mechanism that could
 * not work without it.  This one works, and a caller-owned table is a
 * legitimate design: the kernel does not need to enumerate a process's
 * attachments, only to record them.
 *
 * @version 3.0.0  @date 2026-09-30
 */
#ifndef UIOX_SHM_H
#define UIOX_SHM_H

#include "uiox_kix_ipc_types.h"

/* Attach flags */
#define SHM_RDONLY  0x1000
#define SHM_RND     0x2000
#define SHM_RND2    0x2000   /* reserved, kept for a future alignment mode */

/* ── Region descriptor ───────────────────────────────────────────────
 * mem is claimed LAZILY — on the first uiox_kix_shm_at rather than at uiox_kix_shm_get.
 * That is Bach's design: get creates the name and the size, attach is
 * what needs storage.
 *
 * mem_allocated distinguishes "no memory yet" from "memory that happens
 * to be at address zero", which is why both fields exist. */
typedef struct {
    IpcPerm       perm;
    uiox_size_t   size;           /* bytes in this region        */
    uiox_uint8_t *mem;            /* backing storage             */
    int           attach_count;   /* processes currently attached */
    bool          active;
    bool          mem_allocated;  /* storage claimed at first attach */
    time_t        atime;          /* last attach   */
    time_t        dtime;          /* last detach   */
    time_t        ctime;          /* last change   */
    int           creator_pid;
    int           last_pid;
} ShmRegion;

/* ── One process's attachment to a region ──────────────────────────── */
typedef struct {
    int   shmid;
    void *va;          /* the address in this process's space */
    bool  rdonly;
    bool  active;
} ShmAttach;

#define MAX_SHM_ATTACHES 8

/* ── API ───────────────────────────────────────────────────────────── */

void uiox_kix_shm_init(void);

/* Algorithm 5 — find by key, or create.  Storage is NOT allocated here;
 * only the name, the size and the permission block. */
int uiox_kix_shm_get(int key, uiox_size_t size, int flag);

/* Algorithm 6 — attach to a process's address space.
 *
 * With va_hint: the address is rounded DOWN to a page boundary, because
 * a mapping must start where a page does.  Without one: the kernel
 * chooses, which on this build means the static backing store's address.
 *
 * Returns the attached address, or NULL. */
void *uiox_kix_shm_at(int shmid, void *va_hint, int flags,
            ShmAttach attaches[], int *attach_count);

/* Algorithm 7 — detach.
 *
 * The region is NOT freed when the last process detaches.  Bach and
 * POSIX both keep the data until IPC_RMID, which is the behaviour
 * everything relying on shared memory depends on. */
int uiox_kix_shm_dt(void *va, ShmAttach attaches[], int *attach_count);

/* Algorithm 8 — IPC_STAT, IPC_SET, IPC_RMID.  Only IPC_RMID releases
 * the backing store. */
int uiox_kix_shm_ctl(int shmid, int cmd, ShmRegion *buf);

#endif /* UIOX_SHM_H */
