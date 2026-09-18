/*
 * 30_KIX/33_PCS/src/uiox_mmap.c
 *
 * Zero-copy mmap: maps kernel physical pages (buffer pool frames,
 * GPU buffers, camera DMA buffers) directly into user VA space.
 *
 * No copy_to_user needed — user reads the physical page directly
 * through their own page table mapping.
 *
 * Call sequence:
 *   1. User: ioctl(fd, UIOX_IOC_CAM_GET_FRAME, &info)
 *              → kernel fills info.paddr
 *   2. User: ptr = mmap(NULL, info.size, PROT_READ,
 *                       MAP_SHARED, fd, info.paddr)
 *   3. Kernel: uiox_mm_map_user_phys(proc, va_hint,
 *                                     info.paddr, info.size,
 *                                     prot)
 *              → inserts PTE into user page table
 *   4. User: reads ptr directly — zero copy, no syscall
 *
 * @version 1.0.0
 * @date    2026-07-28
 */

 #include "uiox_mmap.h"
 #include "uiox_uaccess.h"
 #include "uiox_soc.h"
 
 #define EINVAL  22
 #define ENOMEM  12
 #define EPERM    1
 
 /* Page-align helpers */
 #define PAGE_SIZE   4096UL
 #define PAGE_MASK   (~(PAGE_SIZE - 1UL))
 #define PAGE_ALIGN_UP(x)   (((uintptr_t)(x) + PAGE_SIZE - 1UL) & PAGE_MASK)
 #define PAGE_ALIGN_DOWN(x) ((uintptr_t)(x) & PAGE_MASK)
 
 /*
  * uiox_mm_map_user_phys
  *
  * Maps physical address range [pa, pa+size) into the current
  * process's user page table at a kernel-chosen VA (or va_hint
  * if non-zero and available).
  *
  * prot: UIOX_PROT_READ / UIOX_PROT_READ|UIOX_PROT_WRITE
  *
  * Returns user VA on success, 0 on failure.
  *
  * Architecture page table update:
  *   ARM64:   write PTE into TTBR0_EL1 page table,
  *            then TLBI VMALLE1IS (broadcast TLB invalidate)
  *   ARM32:   write PTE into TTBR0 page table,
  *            then TLBI VMALLE1
  *   RISC-V:  write PTE into satp page table,
  *            then SFENCE.VMA
  *   x86-64:  write PTE into CR3 page table,
  *            then INVLPG(va)
  */
 uintptr_t uiox_mm_map_user_phys(uiox_proc_t  *proc,
                                   uintptr_t     va_hint,
                                   uintptr_t     pa,
                                   size_t        size,
                                   uint32_t      prot)
 {
     if (!proc)           return 0u;
     if (size == 0u)      return 0u;
     if (pa == 0u)        return 0u;
 
     /* Align pa and size to page boundary */
     uintptr_t pa_aligned   = PAGE_ALIGN_DOWN(pa);
     size_t    size_aligned = PAGE_ALIGN_UP(size +
                                (pa - pa_aligned));
 
     /*
      * TODO: allocate a free VA range in proc->mm (user VMA list).
      * For now, stub returns 0 (not implemented).
      *
      * Real implementation:
      *
      *   uintptr_t va = uiox_vma_alloc(proc, va_hint,
      *                                  size_aligned, prot);
      *   if (va == 0) return 0;
      *
      *   for (size_t off = 0; off < size_aligned; off += PAGE_SIZE) {
      *       uiox_pte_t pte = uiox_mk_pte(pa_aligned + off, prot);
      *       uiox_pte_set(proc->mm.pgd, va + off, pte);
      *   }
      *
      *   uiox_tlb_flush_user(proc);  // arch-specific TLB invalidate
      *   return va;
      */
     (void)pa_aligned; (void)size_aligned; (void)va_hint; (void)prot;
     return 0u;  /* stub */
 }
 
 /*
  * uiox_mm_unmap_user
  *
  * Removes the user VA mapping and flushes TLB.
  * Does NOT free the underlying physical pages
  * (those belong to the buffer pool / device).
  */
 int uiox_mm_unmap_user(uiox_proc_t *proc,
                         uintptr_t    va,
                         size_t       size)
 {
     if (!proc)       return -EINVAL;
     if (size == 0u)  return -EINVAL;
     if (!uiox_uaccess_ok((void *)va, size))
         return -EINVAL;
 
     /*
      * TODO:
      *   uintptr_t va_end = va + PAGE_ALIGN_UP(size);
      *   for (uintptr_t v = va; v < va_end; v += PAGE_SIZE)
      *       uiox_pte_clear(proc->mm.pgd, v);
      *   uiox_tlb_flush_user(proc);
      *   uiox_vma_free(proc, va, PAGE_ALIGN_UP(size));
      */
     return 0;
 }
 