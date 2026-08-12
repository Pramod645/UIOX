/* ARM32 SVC (software interrupt) vector entry */
svc_entry:
    /* Save caller state on SVC-mode stack */
    push    {r0-r12, lr}        /* save args + link register          */
    mrs     r12, spsr           /* save user CPSR                     */
    push    {r12}               /* onto SVC stack                     */

    /* Pass syscall number (r7) as last C argument                    */
    /* C signature: arch_syscall_entry(r0,r1,r2,r3,r4,r5, nr=r7)    */
    mov     r6, r7              /* nr → r6 then stack for 7th arg     */
    push    {r6}                /* 7th argument on stack              */
    bl      arch_syscall_entry  /* returns result in r0               */
    add     sp, sp, #4          /* pop nr                             */

    /* Restore and return to user mode                                */
    pop     {r12}               /* restore saved CPSR                 */
    msr     spsr_cxsf, r12
    ldm     sp!, {r1-r12, pc}^  /* restore r1-r12, return, ^ = CPSR  */
    /* r0 already holds return value from bl arch_syscall_entry       */
