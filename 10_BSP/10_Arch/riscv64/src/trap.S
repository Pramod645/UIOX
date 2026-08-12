/*
 * RISC-V 64 trap entry — stvec points here.
 * Handles ecall (scause=8), page faults, timer, and IRQs.
 */
    .align  2
    .global trap_entry
trap_entry:
    /* Save all caller-saved regs on kernel stack */
    addi    sp, sp, -256
    sd      ra,   0(sp)
    sd      t0,   8(sp)
    sd      t1,  16(sp)
    sd      t2,  24(sp)
    sd      a0,  32(sp)
    sd      a1,  40(sp)
    sd      a2,  48(sp)
    sd      a3,  56(sp)
    sd      a4,  64(sp)
    sd      a5,  72(sp)
    sd      a6,  80(sp)
    sd      a7,  88(sp)    /* syscall number */
    sd      t3,  96(sp)
    sd      t4, 104(sp)
    sd      t5, 112(sp)
    sd      t6, 120(sp)

    /* Read trap cause */
    csrr    t0, scause

    /* Check for ecall from U-mode: scause = 8 */
    li      t1, 8
    bne     t0, t1, not_ecall

    /* Advance sepc past the ecall instruction (4 bytes) */
    csrr    t0, sepc
    addi    t0, t0, 4
    csrw    sepc, t0

    /*
     * Call arch_syscall_entry(a0,a1,a2,a3,a4,a5,a6_unused, nr=a7)
     * a0–a7 already in registers from user context.
     * Move a7 (nr) to match C argument order:
     *   RV64 ABI: args in a0,a1,a2,a3,a4,a5,a6,a7
     *   Our C fn:        a0,a1,a2,a3,a4,a5,unused,nr
     * a7 is already in position 8 — nothing to move.
     */
    call    arch_syscall_entry
    /* result in a0 — leave it there for SRET */
    j       restore_and_sret

not_ecall:
    /* Timer IRQ: scause = 0x8000000000000005 (S-mode timer) */
    li      t1, 5
    /* check bit 63 (interrupt flag) + cause=5 */
    csrr    t0, scause
    bltz    t0, check_interrupt   /* bit 63 set = interrupt */
    /* otherwise = exception (page fault etc.) */
    call    uiox_mm_page_fault    /* 33_PCS/02_MemMngnt */
    j       restore_and_sret

check_interrupt:
    andi    t0, t0, 0xFF
    li      t1, 5                 /* supervisor timer interrupt */
    beq     t0, t1, timer_irq
    call    uiox_soc_irq_dispatch /* PLIC IRQ — 03_SoC */
    j       restore_and_sret

timer_irq:
    call    uiox_sched_tick       /* 33_PCS/01_schedular */
    /* acknowledge timer via SBI_SET_TIMER ecall */
    li      a7, 0                 /* SBI_SET_TIMER = 0 */
    li      a0, -1                /* next tick = max (one-shot disable) */
    ecall                         /* → OpenSBI M-mode */

restore_and_sret:
    ld      ra,   0(sp)
    ld      t0,   8(sp)
    ld      t1,  16(sp)
    ld      t2,  24(sp)
    ld      a0,  32(sp)
    ld      a1,  40(sp)
    ld      a2,  48(sp)
    ld      a3,  56(sp)
    ld      a4,  64(sp)
    ld      a5,  72(sp)
    ld      a6,  80(sp)
    ld      a7,  88(sp)
    ld      t3,  96(sp)
    ld      t4, 104(sp)
    ld      t5, 112(sp)
    ld      t6, 120(sp)
    addi    sp, sp, 256
    sret
