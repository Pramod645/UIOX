/**
 * @file  uiox_kp_mem.h
 * @brief UIOX Live Kernel Patching — executable memory allocator.
 *
 * Trampolines must live in executable memory. This module provides a
 * small bump allocator backed by a statically-allocated executable region.
 *
 * @version 1.0.0
 * @date    2026-07-07
 */

 #ifndef UIOX_KP_MEM_H
 #define UIOX_KP_MEM_H
 
 #include "uiox_kp_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Trampoline pool configuration
  * ====================================================================== */
 
 #define UIOX_KP_TRAMP_POOL_SIZE   (4u * 1024u)   /**< 4 KB trampoline pool */
 #define UIOX_KP_TRAMP_ALIGN       16u             /**< 16-byte aligned      */
 
 /* =========================================================================
  * Memory allocator API
  * ====================================================================== */
 
 /**
  * Initialise the trampoline pool.
  * Must be called before any uiox_kp_mem_alloc().
  */
 uiox_kp_err_t  uiox_kp_mem_init  (void);
 
 /**
  * Allocate @size bytes from the executable trampoline pool.
  * Returns aligned pointer or NULL on failure.
  * Memory is zeroed before return.
  */
 void          *uiox_kp_mem_alloc (size_t size);
 
 /**
  * Free memory previously returned by uiox_kp_mem_alloc.
  * (Simple bump allocator: free is a no-op but marks the region unused.)
  */
 void           uiox_kp_mem_free  (void *ptr, size_t size);
 
 /** Return bytes remaining in the pool. */
 size_t         uiox_kp_mem_avail (void);
 
 /** Print pool state to kernel console. */
 void           uiox_kp_mem_print (void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KP_MEM_H */
 