/**
 * @file    uiox_cpu_if.h
 * @brief   UIOX CPU interface driver (core init, MMU, exception vectors).
 *
 * Manages:
 *   - Bootstrap / reset vector setup per architecture
 *   - Exception / interrupt vector table initialisation
 *   - MMU page-table setup (AArch64 TTBR0/1, x86 CR3, RISC-V SATP)
 *   - SMP secondary core bring-up sequence
 *   - Architectural timer initialisation
 *
 * @date    2026-06-02
 */

 #ifndef UIOX_CPU_IF_H
 #define UIOX_CPU_IF_H
 
 #include "uiox_cpu_hw.h"
 #include "uiox_cpu_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * MMU page-table mode
  * ====================================================================== */
 
 typedef enum {
     UIOX_MMU_OFF    = 0,
     UIOX_MMU_4K_39  = 1,   /**< ARM64 4K pages, 39-bit (SV39 / 3-level)  */
     UIOX_MMU_4K_48  = 2,   /**< ARM64 4K pages, 48-bit (SV48 / 4-level)  */
     UIOX_MMU_2M_39  = 3,   /**< RISC-V Sv39 (2 MB huge pages)            */
     UIOX_MMU_X86_4L = 4,   /**< x86-64 4-level (PML4, PDPT, PD, PT)     */
     UIOX_MMU_X86_5L = 5,   /**< x86-64 5-level (PML5 + LA57)            */
 } uiox_mmu_mode_t;
 
 /* =========================================================================
  * Exception / fault types
  * ====================================================================== */
 
 typedef enum {
     UIOX_FAULT_SYNC_ABT  = 0,  /**< ARM64 synchronous abort               */
     UIOX_FAULT_IRQ,             /**< External interrupt                    */
     UIOX_FAULT_FIQ,             /**< Fast interrupt (ARM64)                */
     UIOX_FAULT_SERROR,          /**< System error / async abort            */
     UIOX_FAULT_GPF,             /**< x86 General Protection Fault (#GP)   */
     UIOX_FAULT_PF,              /**< x86/RV Page Fault                     */
     UIOX_FAULT_DE,              /**< x86 Divide Error (#DE)               */
     UIOX_FAULT_NMI,             /**< Non-Maskable Interrupt                */
     UIOX_FAULT_ECALL,           /**< RISC-V ECALL (system call)            */
     UIOX_FAULT_ILLEGAL,         /**< Illegal instruction                   */
 } uiox_fault_t;
 
 typedef void (*uiox_fault_handler_t)(uiox_fault_t fault,
                                       uint64_t fault_addr,
                                       uint64_t pc,
                                       void *ctx);
 
 /* =========================================================================
  * Interface descriptor
  * ====================================================================== */
 
 typedef struct {
     uiox_cpu_hw_t        *hw;
     uiox_mmu_mode_t       mmu_mode;
     uint64_t             *pgd_root;    /**< Root page-table physical addr  */
     uiox_fault_handler_t  fault_handlers[16];
     bool                  mmu_enabled;
     bool                  timer_enabled;
     uint32_t              timer_interval_ns; /**< Timer tick period         */
     bool                  primed;
 } uiox_cpu_if_t;
 
 /* =========================================================================
  * Interface API
  * ====================================================================== */
 
 int  uiox_cpu_if_init         (uiox_cpu_if_t *cif, uiox_cpu_hw_t *hw);
 int  uiox_cpu_if_vectors_init (uiox_cpu_if_t *cif);
 int  uiox_cpu_if_mmu_init     (uiox_cpu_if_t *cif,
                                 uiox_mmu_mode_t mode,
                                 uint64_t *pgd_root);
 int  uiox_cpu_if_mmu_enable   (uiox_cpu_if_t *cif);
 void uiox_cpu_if_mmu_disable  (uiox_cpu_if_t *cif);
 int  uiox_cpu_if_timer_init   (uiox_cpu_if_t *cif,
                                 uint32_t interval_ns);
 int  uiox_cpu_if_smp_boot     (uiox_cpu_if_t *cif,
                                 uint8_t core_id, uintptr_t entry);
 void uiox_cpu_if_register_fault(uiox_cpu_if_t *cif,
                                  uiox_fault_t fault,
                                  uiox_fault_handler_t handler);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_CPU_IF_H */
 