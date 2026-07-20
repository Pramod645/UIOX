#include "shm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ShmRegion regions[IPC_MAX_SHM];

void shm_init(void)
{
    memset(regions, 0, sizeof regions);
    printf("[shm] init: max_regions=%d  max_size=%d\n",
           IPC_MAX_SHM, IPC_MAX_SHM_SIZE);
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm shmget  (§5)
 * ───────────────────────────────────────────────────────────── */
int shmget(int key, size_t size, int flag)
{
    /* Search for existing region */
    for (int i = 0; i < IPC_MAX_SHM; i++) {
        if (regions[i].active && regions[i].perm.key == key) {
            printf("[shmget] found existing region id=%d key=%d\n", i, key);
            return i;
        }
    }

    if (!(flag & IPC_CREAT)) {
        fprintf(stderr, "[shmget] no region key=%d and no IPC_CREAT\n", key);
        return -1;
    }

    /* Validate size */
    if (size == 0 || size > IPC_MAX_SHM_SIZE) {
        fprintf(stderr, "[shmget] size %zu out of range\n", size);
        return -1;
    }

    for (int i = 0; i < IPC_MAX_SHM; i++) {
        if (!regions[i].active) {
            regions[i].active        = true;
            regions[i].perm.key      = key;
            regions[i].perm.mode     = (uint16_t)(flag & 0x1FF);
            regions[i].size          = size;
            regions[i].mem           = NULL;  /* allocated on first shmat */
            regions[i].mem_allocated = false;
            regions[i].attach_count  = 0;
            regions[i].ctime         = time(NULL);
            printf("[shmget] created region id=%d key=%d size=%zu\n",
                   i, key, size);
            return i;
        }
    }

    fprintf(stderr, "[shmget] no free region slots\n");
    return -1;
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm shmat  (§6)
 * ───────────────────────────────────────────────────────────── */
void *shmat(int shmid, void *va_hint, int flags,
            ShmAttach attaches[], int *attach_count)
{
    if (shmid < 0 || shmid >= IPC_MAX_SHM || !regions[shmid].active) {
        fprintf(stderr, "[shmat] invalid shmid %d\n", shmid);
        return NULL;
    }

    ShmRegion *r = &regions[shmid];

    /* Find a free attach slot */
    if (*attach_count >= MAX_SHM_ATTACHES) {
        fprintf(stderr, "[shmat] attach table full\n");
        return NULL;
    }

    /* If user provided a virtual address, use it; else kernel picks */
    void *va;
    if (va_hint) {
        /* Round to page boundary (simulated: 4096) */
        uintptr_t aligned = ((uintptr_t)va_hint) & ~(uintptr_t)0xFFF;
        va = (void *)aligned;
        printf("[shmat] user-specified va rounded to %p\n", va);
    } else {
        /* Kernel picks — allocate real memory in simulation */
        if (!r->mem_allocated) {
            r->mem = calloc(1, r->size);
            if (!r->mem) { perror("calloc"); return NULL; }
            r->mem_allocated = true;
            printf("[shmat] first attach — allocated %zu bytes "
                   "(growreg/pagetables)\n", r->size);
        }
        va = r->mem;
        printf("[shmat] kernel chose va=%p\n", va);
    }

    /* Record attachment */
    ShmAttach *a  = &attaches[*attach_count];
    a->shmid      = shmid;
    a->va         = va;
    a->rdonly     = (flags & SHM_RDONLY) != 0;
    a->active     = true;
    (*attach_count)++;

    r->attach_count++;
    r->atime = r->last_pid = (int)(size_t)va; /* simplified */
    r->atime = time(NULL);

    printf("[shmat] shmid=%d attached at va=%p  rdonly=%s  attach_count=%d\n",
           shmid, va, a->rdonly ? "yes" : "no", r->attach_count);
    return va;
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm shmdt  (§7)
 * ───────────────────────────────────────────────────────────── */
int shmdt(void *va, ShmAttach attaches[], int *attach_count)
{
    for (int i = 0; i < *attach_count; i++) {
        if (attaches[i].active && attaches[i].va == va) {
            int shmid = attaches[i].shmid;
            attaches[i].active = false;

            if (shmid >= 0 && shmid < IPC_MAX_SHM) {
                ShmRegion *r = &regions[shmid];
                r->attach_count--;
                r->dtime = time(NULL);
                /*
                 * Region is NOT freed when last process detaches —
                 * data persists until IPC_RMID is called.
                 */
                printf("[shmdt] va=%p  shmid=%d  remaining_attachments=%d\n",
                       va, shmid, r->attach_count);
            }
            return 0;
        }
    }
    fprintf(stderr, "[shmdt] va=%p not found in attach table\n", va);
    return -1;
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm shmctl  (§8)
 * ───────────────────────────────────────────────────────────── */
int shmctl(int shmid, int cmd, ShmRegion *buf)
{
    if (shmid < 0 || shmid >= IPC_MAX_SHM || !regions[shmid].active) {
        fprintf(stderr, "[shmctl] invalid shmid %d\n", shmid);
        return -1;
    }

    ShmRegion *r = &regions[shmid];

    switch (cmd) {
    case IPC_STAT:
        if (buf) *buf = *r;
        printf("[shmctl] IPC_STAT shmid=%d size=%zu attachments=%d\n",
               shmid, r->size, r->attach_count);
        break;

    case IPC_SET:
        if (buf) {
            r->perm.mode = buf->perm.mode;
            r->ctime     = time(NULL);
        }
        printf("[shmctl] IPC_SET shmid=%d\n", shmid);
        break;

    case IPC_RMID:
        if (r->mem) { free(r->mem); r->mem = NULL; }
        memset(r, 0, sizeof *r);
        printf("[shmctl] IPC_RMID shmid=%d removed\n", shmid);
        break;

    default:
        fprintf(stderr, "[shmctl] unknown cmd %d\n", cmd);
        return -1;
    }
    return 0;
}
