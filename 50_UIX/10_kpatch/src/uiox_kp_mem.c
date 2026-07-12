/**
 * @file  uiox_kp_mem.c
 * @brief UIOX kpatch — executable trampoline pool allocator. No libc.
 * @date  2026-07-07
 */

 #include "../include/uiox_kp_mem.h"

 /* Trampoline pool — must be in executable memory.
  * Placed in a dedicated section so the linker can mark it executable. */
 static uint8_t  s_pool[UIOX_KP_TRAMP_POOL_SIZE]
                 __attribute__((aligned(UIOX_KP_TRAMP_ALIGN),
                                section(".kpatch.tramp")));
 static size_t   s_pool_top  = 0u;
 static bool     s_pool_init = false;
 
 /* Allocation descriptor (simple slab for free tracking) */
 typedef struct { uintptr_t base; size_t size; bool used; } kp_alloc_t;
 #define KP_ALLOC_MAX 128u
 static kp_alloc_t s_allocs[KP_ALLOC_MAX];
 static uint32_t   s_alloc_cnt = 0u;
 
 uiox_kp_err_t uiox_kp_mem_init(void)
 {
     /* Zero the pool */
     for (size_t i = 0u; i < UIOX_KP_TRAMP_POOL_SIZE; i++) s_pool[i] = 0u;
     s_pool_top  = 0u;
     s_alloc_cnt = 0u;
     s_pool_init = true;
     return UIOX_KP_OK;
 }
 
 void *uiox_kp_mem_alloc(size_t size)
 {
     if (!s_pool_init) return NULL;
     if (size == 0u) return NULL;
 
     /* Align up */
     size_t aligned = (size + UIOX_KP_TRAMP_ALIGN - 1u) &
                      ~(UIOX_KP_TRAMP_ALIGN - 1u);
 
     if (s_pool_top + aligned > UIOX_KP_TRAMP_POOL_SIZE) return NULL;
     if (s_alloc_cnt >= KP_ALLOC_MAX) return NULL;
 
     void *ptr = (void *)((uintptr_t)s_pool + s_pool_top);
 
     /* Record allocation */
     s_allocs[s_alloc_cnt].base = (uintptr_t)ptr;
     s_allocs[s_alloc_cnt].size = aligned;
     s_allocs[s_alloc_cnt].used = true;
     s_alloc_cnt++;
 
     s_pool_top += aligned;
     return ptr;
 }
 
 void uiox_kp_mem_free(void *ptr, size_t size)
 {
     if (!ptr) return;
     for (uint32_t i = 0u; i < s_alloc_cnt; i++) {
         if (s_allocs[i].base == (uintptr_t)ptr) {
             s_allocs[i].used = false;
             /* Zero the freed region */
             uint8_t *p = (uint8_t *)ptr;
             for (size_t j = 0u; j < size; j++) p[j] = 0u;
             return;
         }
     }
 }
 
 size_t uiox_kp_mem_avail(void)
 {
     return UIOX_KP_TRAMP_POOL_SIZE - s_pool_top;
 }
 
 /* Forward declaration of kprintf */
 extern void uiox_fw_printf(const char *fmt, ...);
 
 void uiox_kp_mem_print(void)
 {
     uiox_fw_printf("[kpatch] Trampoline pool: %zu / %u bytes used\n",
                     s_pool_top, UIOX_KP_TRAMP_POOL_SIZE);
     for (uint32_t i = 0u; i < s_alloc_cnt; i++) {
         if (s_allocs[i].used)
             uiox_fw_printf("  [%u] base=0x%016llx  size=%zu\n",
                             i,
                             (unsigned long long)s_allocs[i].base,
                             s_allocs[i].size);
     }
 }
 