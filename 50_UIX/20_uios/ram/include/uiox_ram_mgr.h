/**
 * @file    uiox_ram_mgr.h
 * @brief   UIOX RAM memory manager: heap, slab, buddy allocator.
 * @date    2026-06-03
 */

 #ifndef UIOX_RAM_MGR_H
 #define UIOX_RAM_MGR_H
 
 #include "uiox_ram_ecc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_RAM_HEAP_ALIGN         16u
 #define UIOX_RAM_SLAB_MAX_CLASSES   8
 #define UIOX_RAM_BUDDY_MAX_ORDER    16   /**< Max 2^16 × min_block bytes  */
 #define UIOX_RAM_MIN_BLOCK          64u  /**< Buddy allocator min block    */
 
 /* =========================================================================
  * Heap allocator (first-fit linked list)
  * ====================================================================== */
 
 typedef struct uiox_heap_block {
     uint32_t               magic;     /**< 0xA110C1ED = allocated         */
     uint32_t               size;      /**< User-visible size (bytes)       */
     struct uiox_heap_block *prev;
     struct uiox_heap_block *next;
     bool                   free;
 } uiox_heap_block_t;
 
 #define UIOX_HEAP_MAGIC_FREE  0xFEEEFEEEu
 #define UIOX_HEAP_MAGIC_ALLOC 0xA110C1EDu
 
 typedef struct {
     uint8_t           *base;
     size_t             capacity;
     uiox_heap_block_t *head;
     size_t             used;
     size_t             peak;
     uint32_t           alloc_count;
     uint32_t           free_count;
 } uiox_heap_t;
 
 /* =========================================================================
  * Slab allocator (fixed-size object caches)
  * ====================================================================== */
 
  typedef struct uiox_slab_obj {
    struct uiox_slab_obj *next_free;
} uiox_slab_obj_t;

typedef struct {
    const char       *name;
    size_t            obj_size;    /**< Object size (bytes)               */
    uint32_t          obj_count;   /**< Total objects in slab             */
    uint32_t          free_count;
    uint8_t          *slab_mem;    /**< Slab backing memory               */
    uiox_slab_obj_t  *free_list;
    uint32_t          alloc_count;
    uint32_t          free_ops;
} uiox_slab_cache_t;

/* =========================================================================
 * Buddy allocator
 * ====================================================================== */

typedef struct uiox_buddy_block {
    struct uiox_buddy_block *next;
} uiox_buddy_block_t;

typedef struct {
    uint8_t            *base;
    size_t              capacity;
    uint32_t            min_block;  /**< Minimum allocation (bytes)        */
    uint8_t             max_order;  /**< log2(capacity/min_block)          */
    uiox_buddy_block_t *free_list[UIOX_RAM_BUDDY_MAX_ORDER];
    uint8_t            *bitmap;     /**< Split-state bitmap                */
    size_t              used;
    uint32_t            alloc_count;
    uint32_t            free_count;
} uiox_buddy_t;

/* =========================================================================
 * Memory manager context
 * ====================================================================== */

typedef struct {
    uiox_ram_ecc_t    *ecc;
    uiox_heap_t        heap;
    uiox_buddy_t       buddy;
    uiox_slab_cache_t  slabs[UIOX_RAM_SLAB_MAX_CLASSES];
    uint8_t            num_slabs;
} uiox_ram_mgr_t;

/* =========================================================================
 * Memory manager API
 * ====================================================================== */

int   uiox_ram_mgr_init        (uiox_ram_mgr_t *mgr,
                                 uiox_ram_ecc_t *ecc,
                                 void *heap_base, size_t heap_size,
                                 void *buddy_base, size_t buddy_size);

/* Heap */
void *uiox_heap_alloc          (uiox_heap_t *h, size_t size);
void *uiox_heap_calloc         (uiox_heap_t *h, size_t n, size_t sz);
void *uiox_heap_realloc        (uiox_heap_t *h, void *ptr, size_t new_size);
void  uiox_heap_free           (uiox_heap_t *h, void *ptr);
void  uiox_heap_stats          (const uiox_heap_t *h,
                                 size_t *used, size_t *free_bytes,
                                 size_t *peak);

/* Slab */
int   uiox_slab_create         (uiox_ram_mgr_t *mgr, const char *name,
                                 size_t obj_size, uint32_t count,
                                 void *backing_mem);
void *uiox_slab_alloc          (uiox_ram_mgr_t *mgr, size_t obj_size);
void  uiox_slab_free           (uiox_ram_mgr_t *mgr, void *ptr,
                                 size_t obj_size);

/* Buddy */
int   uiox_buddy_init          (uiox_buddy_t *b, void *base, size_t size,
                                 uint32_t min_block, void *bitmap_mem);
void *uiox_buddy_alloc         (uiox_buddy_t *b, size_t size);
void  uiox_buddy_free          (uiox_buddy_t *b, void *ptr, size_t size);
void  uiox_buddy_stats         (const uiox_buddy_t *b,
                                 size_t *used, size_t *free_bytes);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_RAM_MGR_H */
