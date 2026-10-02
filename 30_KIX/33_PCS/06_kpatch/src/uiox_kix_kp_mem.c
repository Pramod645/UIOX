/**
 * @file  uiox_kix_kp_mem.c
 * @brief UIOX kpatch — executable trampoline pool allocator. No libc.
 *
 * s_pool_top is monotonic.  A freed region is zeroed and its slot marked
 * unused, but the space is never handed out again, so the pool is a
 * lifetime budget rather than a working set.  See uiox_kix_kp_mem.h.
 *
 * @version 1.1.0
 * @date    2026-10-02
 */

 #include "../include/uiox_kix_kp_mem.h"

 extern void uiox_fw_printf(const char *fmt, ...);
 
 /* Trampoline pool — must be in executable memory. */
 static uint8_t  s_pool[UIOX_KIX_KP_TRAMP_POOL_SIZE]
                 __attribute__((aligned(UIOX_KIX_KP_TRAMP_ALIGN),
                                section(".kpatch.tramp")));
 static size_t   s_pool_top  = 0u;
 static bool     s_pool_init = false;
 
 typedef struct { uintptr_t base; size_t size; bool used; } kp_alloc_t;
 #define KP_ALLOC_MAX 128u
 static kp_alloc_t s_allocs[KP_ALLOC_MAX];
 static uint32_t   s_alloc_cnt   = 0u;
 static uint32_t   s_alloc_total = 0u;
 
 uiox_kix_kp_err_t uiox_kix_kp_mem_init(void)
 {
     for (size_t i = 0u; i < UIOX_KIX_KP_TRAMP_POOL_SIZE; i++) s_pool[i] = 0u;
     s_pool_top    = 0u;
     s_alloc_cnt   = 0u;
     s_alloc_total = 0u;
     s_pool_init   = true;
     return UIOX_KIX_KP_OK;
 }
 
 void *uiox_kix_kp_mem_alloc(size_t size)
 {
     size_t   aligned;
     void    *ptr;
 
     if (!s_pool_init) return NULL;
     if (size == 0u)   return NULL;
 
     /* The bookkeeping table fills independently of the pool.  Report it
      * as a distinct condition — otherwise a caller sees NOMEM with 3 KB
      * free and no way to explain it. */
     if (s_alloc_cnt >= KP_ALLOC_MAX) {
         uiox_fw_printf("[kpatch] tramp bookkeeping full (%u slots); "
                        "%zu B still free in the pool\n",
                        KP_ALLOC_MAX, uiox_kix_kp_mem_avail());
         return NULL;
     }
 
     aligned = (size + UIOX_KIX_KP_TRAMP_ALIGN - 1u) &
               ~(UIOX_KIX_KP_TRAMP_ALIGN - 1u);
 
     if (s_pool_top + aligned > UIOX_KIX_KP_TRAMP_POOL_SIZE) {
         uiox_fw_printf("[kpatch] tramp pool exhausted: want %zu B "
                        "(aligned), %zu B free, %u allocs so far\n",
                        aligned, uiox_kix_kp_mem_avail(), s_alloc_total);
         return NULL;
     }
 
     ptr = (void *)((uintptr_t)s_pool + s_pool_top);
 
     s_allocs[s_alloc_cnt].base = (uintptr_t)ptr;
     s_allocs[s_alloc_cnt].size = aligned;
     s_allocs[s_alloc_cnt].used = true;
     s_alloc_cnt++;
     s_alloc_total++;
 
     s_pool_top += aligned;
     return ptr;
 }
 
 void uiox_kix_kp_mem_free(void *ptr, size_t size)
 {
     if (!ptr) return;
 
     for (uint32_t i = 0u; i < s_alloc_cnt; i++) {
         if (s_allocs[i].base == (uintptr_t)ptr) {
             uint8_t *p = (uint8_t *)ptr;
 
             /* Zero the region.  A freed trampoline left with its bytes
              * intact would still EXECUTE if a caller cached the pointer.
              * Zeroed bytes trap on every architecture this builds for —
              * a crash at the call site rather than a silent misroute. */
             for (size_t j = 0u; j < size; j++) p[j] = 0u;
 
             s_allocs[i].used = false;
 
             /* Deliberately NOT s_pool_top -= size.  A bump allocator
              * cannot return an interior block. */
             return;
         }
     }
 }
 
 size_t uiox_kix_kp_mem_avail(void)
 {
     return UIOX_KIX_KP_TRAMP_POOL_SIZE - s_pool_top;
 }
 
 void uiox_kix_kp_mem_print(void)
 {
     uint32_t live = 0u;
 
     for (uint32_t i = 0u; i < s_alloc_cnt; i++)
         if (s_allocs[i].used) live++;
 
     uiox_fw_printf("[kpatch] Trampoline pool: %zu / %u bytes used "
                    "(%u live, %u slots consumed, %u allocs total)\n",
                     s_pool_top, UIOX_KIX_KP_TRAMP_POOL_SIZE,
                     live, s_alloc_cnt, s_alloc_total);
 
     if (s_alloc_cnt != live)
         uiox_fw_printf("         %u slot(s) freed but NOT reclaimed — "
                        "pool space is permanent\n",
                        s_alloc_cnt - live);
 
     for (uint32_t i = 0u; i < s_alloc_cnt; i++) {
         if (s_allocs[i].used)
             uiox_fw_printf("  [%u] base=0x%016llx  size=%zu\n",
                             i,
                             (unsigned long long)s_allocs[i].base,
                             s_allocs[i].size);
     }
 }
 