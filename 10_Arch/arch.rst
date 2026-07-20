/*
1.ARM64
*/
/*
2.ARM32
*/
/*
3.0x80_64
*/

Boot time        arch_init()     ← already done
                      │
Runtime          ─────┼─────────────────────────────────────────
                      │
          Every interrupt/exception:
          │   arch_irq_entry.S   ← save all registers (ISA)
          │   arch_dispatch_irq() ← read IAR/CAUSE (ISA register)
          │   irq_handler()       ← driver layer
          │   arch_irq_exit.S    ← restore all registers (ISA)
          │   ERET / SRET / IRET ← return from interrupt (ISA)
          │
          Context switch (scheduler):
          │   arch_context_save()   ← save CPU registers to TCB
          │   arch_context_load()   ← restore CPU registers from TCB
          │   arch_context_switch() ← called by 33_PCS scheduler
          │
          MMU / memory management:
          │   arch_mmu_map()        ← write page table entry (ISA)
          │   arch_mmu_unmap()      ← clear PTE
          │   arch_tlb_flush()      ← TLBI/INVLPG/SFENCE.VMA (ISA)
          │
          System calls:
          │   arch_syscall_entry.S  ← SVC/ECALL/SYSCALL (ISA)
          │   arch_syscall_return() ← set return register, ERET
          │
          Atomic operations:
              arch_atomic_add()    ← LDXR/STXR / LOCK XADD / AMOADD
              arch_spinlock_*()    ← LDAXR/STLXR / WFE

========================================
Aspect	ARM32	ARM64	x86_64	RISC-V64
Context switch asm	stm/ldm r4-r11,lr,sp	stp/ldp x19-x30,sp + system regs	push/pop rbx-r15,rsp + CR3	sd/ld s0-s11,ra,sp + CSRs
IRQ entry	SRSDB + CPS + PUSH	STP all regs + MRS/MSR	PUSH all regs + stubs	SAVE_ALL macro + scause
Return from IRQ	RFEIA	ERET	IRETQ	SRET
Syscall instruction	SVC	SVC	SYSCALL / INT 0x80	ECALL
TLB flush	MCR p15,0,r0,c8,c7,1	TLBI VAE1IS	INVLPG	SFENCE.VMA
Atomic ops	LDREX / STREX	LDXR / STXR	LOCK XADD / CMPXCHG	LR.D / SC.D / AMOADD.D
Spinlock wait	WFE	WFE	PAUSE	LR.D spin
MMU page table	Short-descriptor 2-level	4-level (TCR/TTBR0)	4-level PML4 (CR3)	Sv39 3-level (satp)