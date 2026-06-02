#ifndef CPU_MMU_H
#define CPU_MMU_H
/*
 * cpu_mmu.h - MMU / page-table interface
 */
#include "cpu_types.h"

/* -- Page attributes ---------------------------------------- */
#define CPU_MMU_RO    (1u << 0)   /* read-only                    */
#define CPU_MMU_RW    (1u << 1)   /* read-write                   */
#define CPU_MMU_EXEC  (1u << 2)   /* executable                   */
#define CPU_MMU_NOEXEC (1u << 3)  /* no-execute                   */
#define CPU_MMU_CACHE (1u << 4)   /* normal cached memory         */
#define CPU_MMU_DEVICE (1u << 5)  /* device / MMIO memory         */
#define CPU_MMU_SO    (1u << 6)   /* strongly ordered             */
#define CPU_MMU_USER  (1u << 7)   /* accessible from user mode    */
#define CPU_MMU_GLOBAL (1u << 8)  /* global (not ASID tagged)     */
#define CPU_MMU_HUGE  (1u << 9)   /* large page (2 MB / 1 GB)     */

/* -- MMU operations ----------------------------------------- */
int  cpu_mmu_enable      (void);
void cpu_mmu_disable     (void);
int  cpu_mmu_map         (cpu_addr_t virt, cpu_addr_t phys,
                           cpu_u64_t size, cpu_u32_t attrs);
int  cpu_mmu_unmap       (cpu_addr_t virt, cpu_u64_t size);
int  cpu_mmu_set_attrs   (cpu_addr_t virt, cpu_u64_t size,
                           cpu_u32_t attrs);
cpu_addr_t cpu_mmu_virt_to_phys(cpu_addr_t virt);
void cpu_mmu_tlb_flush_all(void);
void cpu_mmu_tlb_flush_va(cpu_addr_t virt);
void cpu_mmu_set_page_table(cpu_addr_t pt_phys);
cpu_addr_t cpu_mmu_get_page_table(void);

/* -- ASID management ---------------------------------------- */
typedef cpu_u16_t cpu_asid_t;
void cpu_mmu_set_asid(cpu_asid_t asid);
cpu_asid_t cpu_mmu_get_asid(void);

#endif /* CPU_MMU_H */
