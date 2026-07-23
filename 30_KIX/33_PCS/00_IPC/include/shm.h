#ifndef UIOX_SHM_H
#define UIOX_SHM_H

#include "ipc_types.h"

/* Flags for shmat */
#define SHM_RDONLY  0x1000
#define SHM_RND    0x2000

/* ─────────────────────────────────────────────────────────────
 * Shared-memory region descriptor
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    IpcPerm  perm;
    size_t   size;            /* bytes in this region            */
    uint8_t *mem;             /* backing storage                 */
    int      attach_count;    /* processes currently attached    */
    bool     active;
    bool     mem_allocated;   /* memory allocated on first shmat */
    time_t   atime;           /* last attach time                */
    time_t   dtime;           /* last detach time                */
    time_t   ctime;           /* last change time                */
    int      creator_pid;
    int      last_pid;
} ShmRegion;

/* ─────────────────────────────────────────────────────────────
 * Per-process attachment record
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    int      shmid;
    void    *va;        /* virtual address in this process's VA  */
    bool     rdonly;
    bool     active;
} ShmAttach;

#define MAX_SHM_ATTACHES 8

/* ─────────────────────────────────────────────────────────────
 * Shared Memory API
 * ───────────────────────────────────────────────────────────── */
void  shm_init(void);

/* Algorithm shmget (§5) — create or find shared-memory region */
int   shmget(int key, size_t size, int flag);

/*
 * Algorithm shmat (§6) — attach region to process address space.
 * va_hint: requested virtual address (0 = let kernel choose).
 * Returns the attached virtual address, or NULL on error.
 */
void *shmat(int shmid, void *va_hint, int flags,
            ShmAttach attaches[], int *attach_count);

/* Algorithm shmdt (§7) — detach region */
int   shmdt(void *va, ShmAttach attaches[], int *attach_count);

/* Algorithm shmctl (§8) — control operations */
int   shmctl(int shmid, int cmd, ShmRegion *buf);

#endif /* UIOX_SHM_H */
