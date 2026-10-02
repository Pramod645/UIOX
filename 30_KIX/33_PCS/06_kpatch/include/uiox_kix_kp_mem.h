/**
 * @file  uiox_kix_kp_mem.h
 * @brief UIOX Live Kernel Patching — executable memory allocator.
 *
 * ── THE CEILING: this allocator does not reclaim ───────────────────────
 * uiox_kix_kp_mem_free() zeroes the region and marks its bookkeeping slot
 * unused, but s_pool_top is NEVER REDUCED and no freed slot is handed out
 * again.  The pool therefore has a hard lifetime budget, not a working set:
 *
 *   pool            4096 bytes
 *   per trampoline  saved_len + max_jump_size
 *                     arm64   saved + 16
 *                     arm32   saved + 8
 *                     x86_64  saved + 14
 *                     riscv64 saved + 8
 *
 * At a 16-byte stub and no saved prologue that is ~256 disable/enable
 * cycles before uiox_kix_kp_mem_alloc() returns NULL and enable answers
 * UIOX_KIX_KP_ERR_NOMEM.  A module loaded and unloaded in a loop will
 * reach it.
 *
 * Deliberate for the current build — a fixed set of patches installed
 * once — and stated rather than left to be discovered at the point of
 * failure.  Making the allocator reclaim is the fix.
 *
 * @version 1.1.0
 * @date    2026-10-02
 */

 #ifndef UIOX_KIX_KP_MEM_H
 #define UIOX_KIX_KP_MEM_H
 
 #include "uiox_kix_kp_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_KIX_KP_TRAMP_POOL_SIZE   (4u * 1024u)   /**< 4 KB trampoline pool */
 #define UIOX_KIX_KP_TRAMP_ALIGN       16u             /**< 16-byte aligned      */
 
 /** Initialise the trampoline pool.  Before any alloc. */
 uiox_kix_kp_err_t  uiox_kix_kp_mem_init  (void);
 
 /** Allocate @size bytes.  Returns aligned zeroed pointer or NULL.
  *  Bytes are consumed permanently — see the ceiling note above. */
 void              *uiox_kix_kp_mem_alloc (size_t size);
 
 /**
  * Free memory previously returned by alloc.
  *
  * Zeroes the region and marks the slot unused.  Does NOT return the space
  * to the pool: s_pool_top is unchanged.
  *
  * @param ptr   a pointer previously returned by uiox_kix_kp_mem_alloc
  * @param size  the length to zero.  A wrong value here zeroes the wrong
  *              region — uiox_kix_kp_disable() passes the length RECORDED
  *              at allocation (patch->tramp_alloc_len) for that reason.
  */
 void               uiox_kix_kp_mem_free  (void *ptr, size_t size);
 
 /** Bytes remaining in the pool.  Only ever decreases. */
 size_t             uiox_kix_kp_mem_avail (void);
 
 /** Print pool state to kernel console. */
 void               uiox_kix_kp_mem_print (void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KIX_KP_MEM_H */
 