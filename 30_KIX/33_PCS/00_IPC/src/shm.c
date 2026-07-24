#include "shm.h"
#include "../include/uiox_klibc.h"
/*
 * 30_KIX/33_PCS/00_IPC/src/shm.c
 *
 * Freestanding fixes (v2.0)
 * ─────────────────────────
 *   REMOVED: #include <stdio.h>  <stdlib.h>  <string.h>
 *            All provided through shm.h → ipc_types.h → uiox_klibc.h
 *
 *   FIXED: fprintf(stderr, ...) → printf(...)     (6 occurrences)
 *   FIXED: time(NULL)           → jiffies          (4 occurrences)
 *   FIXED: calloc(1, r->size)   → static byte pool (no heap)
 *   FIXED: free(r->mem)         → mark pool slot free
 *   FIXED: perror("calloc")     → printf(...)
 *   FIXED: %zu                  → %llu + (unsigned long long) cast (3 occurrences)
 *
 * No algorithm changes — all shmget/shmat/shmdt/shmctl logic unchanged.
 *
 * @version 2.0.0  @date 2026-07-23
 */
 
 /* External tick counter — replaces time(NULL) */
 extern volatile uint64_t jiffies;
 
 /* ── Static shared-memory backing store — replaces calloc/free ───────
  * Each region gets up to IPC_MAX_SHM_SIZE bytes from a static pool.
  * Total pool = IPC_MAX_SHM × IPC_MAX_SHM_SIZE = 16 × 65536 = 1 MB.  */
 static uint8_t  s_shm_pool[IPC_MAX_SHM][IPC_MAX_SHM_SIZE];
 static uint8_t  s_shm_pool_used[IPC_MAX_SHM];   /* 0 = free, 1 = in use */
 
 static uint8_t *shm_mem_alloc(int slot, size_t size)
 {
     if (slot < 0 || slot >= IPC_MAX_SHM) return (uint8_t *)0;
     if (size > IPC_MAX_SHM_SIZE)         return (uint8_t *)0;
     if (s_shm_pool_used[slot])           return (uint8_t *)0;
     s_shm_pool_used[slot] = 1;
     memset(s_shm_pool[slot], 0, size);
     return s_shm_pool[slot];
 }
 
 static void shm_mem_free(int slot)
 {
     if (slot >= 0 && slot < IPC_MAX_SHM) {
         memset(s_shm_pool[slot], 0, IPC_MAX_SHM_SIZE);
         s_shm_pool_used[slot] = 0;
     }
 }
 
 /* ── Region table ────────────────────────────────────────────────────── */
 static ShmRegion regions[IPC_MAX_SHM];
 
 /* ── shm_init ────────────────────────────────────────────────────────── */
 void shm_init(void)
 {
     memset(regions,        0, sizeof regions);
     memset(s_shm_pool_used, 0, sizeof s_shm_pool_used);
     printf("[shm] init: max_regions=%d  max_size=%d\n",
            IPC_MAX_SHM, IPC_MAX_SHM_SIZE);
 }
 
 /* ── Algorithm shmget (§5) ───────────────────────────────────────────── */
 int shmget(int key, size_t size, int flag)
 {
     int i;
 
     for (i = 0; i < IPC_MAX_SHM; i++) {
         if (regions[i].active && regions[i].perm.key == key) {
             printf("[shmget] found existing region id=%d key=%d\n", i, key);
             return i;
         }
     }
 
     if (!(flag & IPC_CREAT)) {
         printf("[shmget] ERROR: no region key=%d and no IPC_CREAT\n", key);
         return -1;
     }
     if (size == 0 || size > IPC_MAX_SHM_SIZE) {
         printf("[shmget] ERROR: size %llu out of range\n",  /* was: %zu */
                (unsigned long long)size);
         return -1;
     }
 
     for (i = 0; i < IPC_MAX_SHM; i++) {
         if (!regions[i].active) {
             regions[i].active        = true;
             regions[i].perm.key      = key;
             regions[i].perm.mode     = (uint16_t)(flag & 0x1FF);
             regions[i].size          = size;
             regions[i].mem           = (uint8_t *)0;
             regions[i].mem_allocated = false;
             regions[i].attach_count  = 0;
             regions[i].ctime         = (int64_t)jiffies; /* was: time(NULL) */
             printf("[shmget] created region id=%d key=%d size=%llu\n",
                    i, key, (unsigned long long)size);           /* was: %zu */
             return i;
         }
     }
 
     printf("[shmget] ERROR: no free region slots\n");
     return -1;
 }
 
 /* ── Algorithm shmat (§6) ────────────────────────────────────────────── */
 void *shmat(int shmid, void *va_hint, int flags,
             ShmAttach attaches[], int *attach_count)
 {
     void *va;
 
     if (shmid < 0 || shmid >= IPC_MAX_SHM || !regions[shmid].active) {
         printf("[shmat] ERROR: invalid shmid %d\n", shmid);
         return (void *)0;
     }
     ShmRegion *r = &regions[shmid];
 
     if (*attach_count >= MAX_SHM_ATTACHES) {
         printf("[shmat] ERROR: attach table full\n");
         return (void *)0;
     }
 
     if (va_hint) {
         uintptr_t aligned = ((uintptr_t)va_hint) & ~(uintptr_t)0xFFF;
         va = (void *)aligned;
         printf("[shmat] user-specified va rounded to %p\n", va);
     } else {
         if (!r->mem_allocated) {
             /* was: calloc(1, r->size) */
             r->mem = shm_mem_alloc(shmid, r->size);
             if (!r->mem) {
                 printf("[shmat] ERROR: shm backing pool exhausted\n"); /* was: perror */
                 return (void *)0;
             }
             r->mem_allocated = true;
             printf("[shmat] first attach — allocated %llu bytes "
                    "(growreg/pagetables)\n",
                    (unsigned long long)r->size);               /* was: %zu */
         }
         va = r->mem;
         printf("[shmat] kernel chose va=%p\n", va);
     }
 
     ShmAttach *a  = &attaches[*attach_count];
     a->shmid      = shmid;
     a->va         = va;
     a->rdonly     = (flags & SHM_RDONLY) != 0;
     a->active     = true;
     (*attach_count)++;
     r->attach_count++;
     r->atime      = (int64_t)jiffies;                         /* was: time(NULL) */
     r->last_pid   = 0;
 
     printf("[shmat] shmid=%d attached at va=%p  rdonly=%s  attach_count=%d\n",
            shmid, va, a->rdonly ? "yes" : "no", r->attach_count);
     return va;
 }
 
 /* ── Algorithm shmdt (§7) ────────────────────────────────────────────── */
 int shmdt(void *va, ShmAttach attaches[], int *attach_count)
 {
     int i;
     for (i = 0; i < *attach_count; i++) {
         if (attaches[i].active && attaches[i].va == va) {
             int shmid = attaches[i].shmid;
             attaches[i].active = false;
 
             if (shmid >= 0 && shmid < IPC_MAX_SHM) {
                 ShmRegion *r = &regions[shmid];
                 r->attach_count--;
                 r->dtime = (int64_t)jiffies;                   /* was: time(NULL) */
                 /*
                  * Region is NOT freed when last process detaches —
                  * data persists until IPC_RMID is called.
                  */
                 printf("[shmdt] shmid=%d detached va=%p  attach_count=%d\n",
                        shmid, va, r->attach_count);
             }
             return 0;
         }
     }
     printf("[shmdt] ERROR: va=%p not found in attach table\n", va);
     return -1;
 }
 
 /* ── Algorithm shmctl (§8) ───────────────────────────────────────────── */
 int shmctl(int shmid, int cmd, ShmRegion *buf)
 {
     if (shmid < 0 || shmid >= IPC_MAX_SHM || !regions[shmid].active) {
         printf("[shmctl] ERROR: invalid shmid %d\n", shmid);
         return -1;
     }
     ShmRegion *r = &regions[shmid];
 
     switch (cmd) {
     case IPC_STAT:
         if (buf) *buf = *r;
         printf("[shmctl] IPC_STAT shmid=%d size=%llu attachments=%d\n",
                shmid,
                (unsigned long long)r->size,                    /* was: %zu */
                r->attach_count);
         break;
 
     case IPC_SET:
         if (buf) {
             r->perm.mode = buf->perm.mode;
             r->ctime     = (int64_t)jiffies;                   /* was: time(NULL) */
         }
         printf("[shmctl] IPC_SET shmid=%d\n", shmid);
         break;
 
     case IPC_RMID:
         if (r->mem_allocated) {
             shm_mem_free(shmid);                               /* was: free(r->mem) */
             r->mem = (uint8_t *)0;
         }
         memset(r, 0, sizeof *r);
         printf("[shmctl] IPC_RMID shmid=%d removed\n", shmid);
         break;
 
     default:
         printf("[shmctl] ERROR: unknown cmd %d\n", cmd);
         return -1;
     }
     return 0;
 }
 