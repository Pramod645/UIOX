/**
 * @file    uiox_cpu_if.c
 * @brief   UIOX CPU interface driver implementation.
 * @date    2026-06-02
 */

 #include "uiox_cpu_if.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_cpu_if_init(uiox_cpu_if_t *cif, uiox_cpu_hw_t *hw)
 {
     if (!cif || !hw) return -EINVAL;
     memset(cif, 0, sizeof(*cif));
     cif->hw     = hw;
     cif->primed = true;
     return 0;
 }
 
 int uiox_cpu_if_vectors_init(uiox_cpu_if_t *cif)
 {
     if (!cif) return -EINVAL;
 #if defined(UIOX_ARCH_ARM64)
     /* Set VBAR_EL1 to the exception vector table base */
     extern uint64_t _vector_table[];
     __asm__ volatile("msr vbar_el1, %0" :: "r"(_vector_table));
     uiox_cpu_isb();
 #elif defined(UIOX_ARCH_X86_64)
     /* Load IDT — in a full implementation, build and lidt here */
     (void)cif;
 #elif defined(UIOX_ARCH_RV64)
     /* Set mtvec to trap handler (direct or vectored mode) */
     extern uint64_t _trap_handler[];
     uiox_cpu_csr_write(mtvec, (uint64_t)_trap_handler | 1u); /* vectored */
 #endif
     return 0;
 }
 
 int uiox_cpu_if_mmu_init(uiox_cpu_if_t *cif,
                           uiox_mmu_mode_t mode,
                           uint64_t *pgd_root)
 {
     if (!cif || !pgd_root) return -EINVAL;
     cif->mmu_mode = mode;
     cif->pgd_root = pgd_root;
     return 0;
 }
 
 int uiox_cpu_if_mmu_enable(uiox_cpu_if_t *cif)
 {
     if (!cif || !cif->pgd_root) return -EINVAL;
 #if defined(UIOX_ARCH_ARM64)
     /* Write TTBR0_EL1 with page-table physical address */
     __asm__ volatile("msr ttbr0_el1, %0" :: "r"(cif->pgd_root));
     uiox_cpu_isb();
     /* Enable MMU via SCTLR_EL1.M */
     uint64_t sctlr;
     __asm__ volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
     sctlr |= (1u << 0);  /* M bit */
     __asm__ volatile("msr sctlr_el1, %0" :: "r"(sctlr));
     uiox_cpu_isb();
 #elif defined(UIOX_ARCH_X86_64)
     /* Load CR3 with PML4 physical base */
     __asm__ volatile("mov %0, %%cr3" :: "r"((uint64_t)cif->pgd_root));
 #elif defined(UIOX_ARCH_RV64)
     /* Write SATP with mode + ASID + PPN */
     uint64_t satp = (8ULL << 60) | ((uint64_t)cif->pgd_root >> 12u);
     uiox_cpu_csr_write(satp, satp);
     uiox_cpu_fence();
 #endif
     cif->mmu_enabled = true;
     return 0;
 }
 
 void uiox_cpu_if_mmu_disable(uiox_cpu_if_t *cif)
 {
     if (!cif) return;
 #if defined(UIOX_ARCH_ARM64)
     uint64_t sctlr;
     __asm__ volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
     sctlr &= ~(1u << 0);
     __asm__ volatile("msr sctlr_el1, %0" :: "r"(sctlr));
     uiox_cpu_isb();
 #elif defined(UIOX_ARCH_RV64)
     uiox_cpu_csr_write(satp, 0);
     uiox_cpu_fence();
 #endif
     cif->mmu_enabled = false;
 }
 
 int uiox_cpu_if_timer_init(uiox_cpu_if_t *cif, uint32_t interval_ns)
 {
     if (!cif) return -EINVAL;
     cif->timer_interval_ns = interval_ns;
     cif->timer_enabled     = true;
 
 #if defined(UIOX_ARCH_ARM64)
     uint64_t freq;
     __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
     uint64_t tval = (uint64_t)freq * interval_ns / 1000000000ULL;
     __asm__ volatile("msr cntp_tval_el0, %0" :: "r"(tval));
     __asm__ volatile("msr cntp_ctl_el0, %0"  :: "r"(1ULL));
 #elif defined(UIOX_ARCH_X86_64)
     /* LAPIC timer setup would go here */
     (void)cif;
 #elif defined(UIOX_ARCH_RV64)
     uint64_t mtime   = *(volatile uint64_t *)(cif->hw->clint_base + 0xBFF8u);
     uint64_t delta   = (uint64_t)cif->hw->timer_freq_hz * interval_ns / 1000000000ULL;
     *(volatile uint64_t *)(cif->hw->clint_base + 0x4000u) = mtime + delta;
     uiox_cpu_csr_set(mie, 1u << 7u);  /* MTIE */
 #endif
     return 0;
 }
 
 int uiox_cpu_if_smp_boot(uiox_cpu_if_t *cif,
                           uint8_t core_id, uintptr_t entry)
 {
     if (!cif) return -EINVAL;
     return uiox_cpu_hw_core_powerup(cif->hw, core_id, entry);
 }
 
 void uiox_cpu_if_register_fault(uiox_cpu_if_t *cif,
                                  uiox_fault_t fault,
                                  uiox_fault_handler_t handler)
 {
     if (!cif || (uint8_t)fault >= 16u) return;
     cif->fault_handlers[(uint8_t)fault] = handler;
 }
 