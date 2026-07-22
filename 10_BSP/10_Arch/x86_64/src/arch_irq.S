/*
 * 10_Arch/x86_64/src/arch_irq.S
 * x86_64 IDT exception/IRQ entry stubs and vector table.
 *
 * The kernel installs the IDT in arch_init() (lidt).
 * Each vector saves the full register frame, calls a C dispatcher,
 * restores, and returns with IRETQ.
 *
 * Fix 2026-07-22: replaced .rept/.set i loops with explicit ISR_NOERR
 * invocations. The .rept loop passed the symbol 'i' (not its value) to
 * the macro, generating duplicate label 'isri' on every iteration.
 */

    .text
    .align 4

/* ── Register frame layout (pushed onto stack) ───────────────────────
 * Low address (top of frame after sub rsp):
 *   rax rcx rdx rbx rbp rsi rdi r8 r9 r10 r11 r12 r13 r14 r15
 *   vector  error_code
 *   rip cs rflags rsp ss   ← hardware-pushed
 * ──────────────────────────────────────────────────────────────────── */
#define FRAME_GPR   120   /* 15 registers × 8                          */

.macro SAVE_REGS
    push    %r15
    push    %r14
    push    %r13
    push    %r12
    push    %r11
    push    %r10
    push    %r9
    push    %r8
    push    %rdi
    push    %rsi
    push    %rbp
    push    %rbx
    push    %rdx
    push    %rcx
    push    %rax
.endm

.macro RESTORE_REGS
    pop     %rax
    pop     %rcx
    pop     %rdx
    pop     %rbx
    pop     %rbp
    pop     %rsi
    pop     %rdi
    pop     %r8
    pop     %r9
    pop     %r10
    pop     %r11
    pop     %r12
    pop     %r13
    pop     %r14
    pop     %r15
.endm

/* ── Stubs without error code ────────────────────────────────────────── */
.macro ISR_NOERR num
    .global isr\num
    .align  4
    isr\num:
        push    $0              /* dummy error code                    */
        push    $\num           /* vector number                       */
        jmp     isr_common
.endm

/* ── Stubs with CPU-pushed error code ────────────────────────────────── */
.macro ISR_ERR num
    .global isr\num
    .align  4
    isr\num:
        push    $\num
        jmp     isr_common
.endm

/* ── CPU exception vectors 0–20 ─────────────────────────────────────── */
ISR_NOERR  0    /* #DE divide error                           */
ISR_NOERR  1    /* #DB debug                                  */
ISR_NOERR  2    /* NMI                                        */
ISR_NOERR  3    /* #BP breakpoint                             */
ISR_NOERR  4    /* #OF overflow                               */
ISR_NOERR  5    /* #BR bound range                            */
ISR_NOERR  6    /* #UD invalid opcode                         */
ISR_NOERR  7    /* #NM device not available                   */
ISR_ERR    8    /* #DF double fault                           */
ISR_NOERR  9    /* coprocessor segment overrun (obsolete)     */
ISR_ERR   10    /* #TS invalid TSS                            */
ISR_ERR   11    /* #NP segment not present                    */
ISR_ERR   12    /* #SS stack fault                            */
ISR_ERR   13    /* #GP general protection                     */
ISR_ERR   14    /* #PF page fault                             */
ISR_NOERR 15    /* reserved                                   */
ISR_NOERR 16    /* #MF x87 FP error                          */
ISR_ERR   17    /* #AC alignment check                        */
ISR_NOERR 18    /* #MC machine check                          */
ISR_NOERR 19    /* #XM SIMD FP                                */
ISR_NOERR 20    /* #VE virtualization                         */

/* Reserved exceptions 21–31 */
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

/* Hardware IRQ vectors 32–255 */
ISR_NOERR 32
ISR_NOERR 33
ISR_NOERR 34
ISR_NOERR 35
ISR_NOERR 36
ISR_NOERR 37
ISR_NOERR 38
ISR_NOERR 39
ISR_NOERR 40
ISR_NOERR 41
ISR_NOERR 42
ISR_NOERR 43
ISR_NOERR 44
ISR_NOERR 45
ISR_NOERR 46
ISR_NOERR 47
ISR_NOERR 48
ISR_NOERR 49
ISR_NOERR 50
ISR_NOERR 51
ISR_NOERR 52
ISR_NOERR 53
ISR_NOERR 54
ISR_NOERR 55
ISR_NOERR 56
ISR_NOERR 57
ISR_NOERR 58
ISR_NOERR 59
ISR_NOERR 60
ISR_NOERR 61
ISR_NOERR 62
ISR_NOERR 63
ISR_NOERR 64
ISR_NOERR 65
ISR_NOERR 66
ISR_NOERR 67
ISR_NOERR 68
ISR_NOERR 69
ISR_NOERR 70
ISR_NOERR 71
ISR_NOERR 72
ISR_NOERR 73
ISR_NOERR 74
ISR_NOERR 75
ISR_NOERR 76
ISR_NOERR 77
ISR_NOERR 78
ISR_NOERR 79
ISR_NOERR 80
ISR_NOERR 81
ISR_NOERR 82
ISR_NOERR 83
ISR_NOERR 84
ISR_NOERR 85
ISR_NOERR 86
ISR_NOERR 87
ISR_NOERR 88
ISR_NOERR 89
ISR_NOERR 90
ISR_NOERR 91
ISR_NOERR 92
ISR_NOERR 93
ISR_NOERR 94
ISR_NOERR 95
ISR_NOERR 96
ISR_NOERR 97
ISR_NOERR 98
ISR_NOERR 99
ISR_NOERR 100
ISR_NOERR 101
ISR_NOERR 102
ISR_NOERR 103
ISR_NOERR 104
ISR_NOERR 105
ISR_NOERR 106
ISR_NOERR 107
ISR_NOERR 108
ISR_NOERR 109
ISR_NOERR 110
ISR_NOERR 111
ISR_NOERR 112
ISR_NOERR 113
ISR_NOERR 114
ISR_NOERR 115
ISR_NOERR 116
ISR_NOERR 117
ISR_NOERR 118
ISR_NOERR 119
ISR_NOERR 120
ISR_NOERR 121
ISR_NOERR 122
ISR_NOERR 123
ISR_NOERR 124
ISR_NOERR 125
ISR_NOERR 126
ISR_NOERR 127
ISR_NOERR 128
ISR_NOERR 129
ISR_NOERR 130
ISR_NOERR 131
ISR_NOERR 132
ISR_NOERR 133
ISR_NOERR 134
ISR_NOERR 135
ISR_NOERR 136
ISR_NOERR 137
ISR_NOERR 138
ISR_NOERR 139
ISR_NOERR 140
ISR_NOERR 141
ISR_NOERR 142
ISR_NOERR 143
ISR_NOERR 144
ISR_NOERR 145
ISR_NOERR 146
ISR_NOERR 147
ISR_NOERR 148
ISR_NOERR 149
ISR_NOERR 150
ISR_NOERR 151
ISR_NOERR 152
ISR_NOERR 153
ISR_NOERR 154
ISR_NOERR 155
ISR_NOERR 156
ISR_NOERR 157
ISR_NOERR 158
ISR_NOERR 159
ISR_NOERR 160
ISR_NOERR 161
ISR_NOERR 162
ISR_NOERR 163
ISR_NOERR 164
ISR_NOERR 165
ISR_NOERR 166
ISR_NOERR 167
ISR_NOERR 168
ISR_NOERR 169
ISR_NOERR 170
ISR_NOERR 171
ISR_NOERR 172
ISR_NOERR 173
ISR_NOERR 174
ISR_NOERR 175
ISR_NOERR 176
ISR_NOERR 177
ISR_NOERR 178
ISR_NOERR 179
ISR_NOERR 180
ISR_NOERR 181
ISR_NOERR 182
ISR_NOERR 183
ISR_NOERR 184
ISR_NOERR 185
ISR_NOERR 186
ISR_NOERR 187
ISR_NOERR 188
ISR_NOERR 189
ISR_NOERR 190
ISR_NOERR 191
ISR_NOERR 192
ISR_NOERR 193
ISR_NOERR 194
ISR_NOERR 195
ISR_NOERR 196
ISR_NOERR 197
ISR_NOERR 198
ISR_NOERR 199
ISR_NOERR 200
ISR_NOERR 201
ISR_NOERR 202
ISR_NOERR 203
ISR_NOERR 204
ISR_NOERR 205
ISR_NOERR 206
ISR_NOERR 207
ISR_NOERR 208
ISR_NOERR 209
ISR_NOERR 210
ISR_NOERR 211
ISR_NOERR 212
ISR_NOERR 213
ISR_NOERR 214
ISR_NOERR 215
ISR_NOERR 216
ISR_NOERR 217
ISR_NOERR 218
ISR_NOERR 219
ISR_NOERR 220
ISR_NOERR 221
ISR_NOERR 222
ISR_NOERR 223
ISR_NOERR 224
ISR_NOERR 225
ISR_NOERR 226
ISR_NOERR 227
ISR_NOERR 228
ISR_NOERR 229
ISR_NOERR 230
ISR_NOERR 231
ISR_NOERR 232
ISR_NOERR 233
ISR_NOERR 234
ISR_NOERR 235
ISR_NOERR 236
ISR_NOERR 237
ISR_NOERR 238
ISR_NOERR 239
ISR_NOERR 240
ISR_NOERR 241
ISR_NOERR 242
ISR_NOERR 243
ISR_NOERR 244
ISR_NOERR 245
ISR_NOERR 246
ISR_NOERR 247
ISR_NOERR 248
ISR_NOERR 249
ISR_NOERR 250
ISR_NOERR 251
ISR_NOERR 252
ISR_NOERR 253
ISR_NOERR 254
ISR_NOERR 255

/* ── Common ISR handler ──────────────────────────────────────────────── */
    .global isr_common
    .align  4
isr_common:
    SAVE_REGS

    /*
     * Stack layout at this point (high → low):
     *   ... SS RSP RFLAGS CS RIP  ← hardware pushed
     *   error_code vector          ← our stubs pushed
     *   r15 r14 ... rax            ← SAVE_REGS
     *
     * vector is at rsp + FRAME_GPR
     * error_code at rsp + FRAME_GPR + 8
     */
    mov     FRAME_GPR(%rsp),     %rdi   /* vector → arg1               */
    mov     FRAME_GPR+8(%rsp),   %rsi   /* error_code → arg2           */
    mov     %rsp,                %rdx   /* frame ptr → arg3            */

    /* Align stack to 16 bytes before call */
    and     $-16, %rsp
    sub     $8,   %rsp

    /* vector < 32 = exception, >= 32 = hardware IRQ, 128 = syscall    */
    cmp     $128, %rdi
    je      .Lsyscall_entry
    cmp     $32,  %rdi
    jl      .Lexception_entry

    /* Hardware IRQ: call C dispatcher */
    call    x86_irq_dispatch
    jmp     .Ldone

.Lexception_entry:
    call    x86_exception_dispatch
    jmp     .Ldone

.Lsyscall_entry:
    mov     FRAME_GPR+112(%rsp), %rdi  /* saved rax = syscall nr       */
    mov     FRAME_GPR+ 56(%rsp), %rsi  /* saved rdi = a0               */
    mov     FRAME_GPR+ 48(%rsp), %rdx  /* saved rsi = a1               */
    mov     FRAME_GPR+ 16(%rsp), %rcx  /* saved rdx = a2               */
    mov     FRAME_GPR+ 72(%rsp), %r8   /* saved r10 = a3               */
    mov     FRAME_GPR+ 64(%rsp), %r9   /* saved r8  = a4               */
    push    FRAME_GPR+ 56(%rsp)        /* saved r9  = a5               */
    call    arch_syscall_dispatch
    add     $8, %rsp
    mov     %rax, FRAME_GPR+112(%rsp)

.Ldone:
    mov     %rdx, %rsp
    RESTORE_REGS
    add     $16, %rsp       /* pop vector + error_code                 */
    iretq


/* ── SYSCALL fast path (MSR_LSTAR) ──────────────────────────────────── */
    .global x86_syscall_entry
    .align  4
x86_syscall_entry:
    swapgs
    mov     %rsp, %r10
    push    %r11
    push    %rcx
    push    %rbp
    mov     %rax, %rdi
    mov     %r10, %rcx
    call    arch_syscall_dispatch
    pop     %rbp
    pop     %rcx
    pop     %r11
    swapgs
    sysretq


/* ── IDT vector table ────────────────────────────────────────────────── */
    .section .rodata
    .global isr_stub_table
    .align  8
isr_stub_table:
    .quad isr0,   isr1,   isr2,   isr3,   isr4,   isr5,   isr6,   isr7
    .quad isr8,   isr9,   isr10,  isr11,  isr12,  isr13,  isr14,  isr15
    .quad isr16,  isr17,  isr18,  isr19,  isr20,  isr21,  isr22,  isr23
    .quad isr24,  isr25,  isr26,  isr27,  isr28,  isr29,  isr30,  isr31
    .quad isr32,  isr33,  isr34,  isr35,  isr36,  isr37,  isr38,  isr39
    .quad isr40,  isr41,  isr42,  isr43,  isr44,  isr45,  isr46,  isr47
    .quad isr48,  isr49,  isr50,  isr51,  isr52,  isr53,  isr54,  isr55
    .quad isr56,  isr57,  isr58,  isr59,  isr60,  isr61,  isr62,  isr63
    .quad isr64,  isr65,  isr66,  isr67,  isr68,  isr69,  isr70,  isr71
    .quad isr72,  isr73,  isr74,  isr75,  isr76,  isr77,  isr78,  isr79
    .quad isr80,  isr81,  isr82,  isr83,  isr84,  isr85,  isr86,  isr87
    .quad isr88,  isr89,  isr90,  isr91,  isr92,  isr93,  isr94,  isr95
    .quad isr96,  isr97,  isr98,  isr99,  isr100, isr101, isr102, isr103
    .quad isr104, isr105, isr106, isr107, isr108, isr109, isr110, isr111
    .quad isr112, isr113, isr114, isr115, isr116, isr117, isr118, isr119
    .quad isr120, isr121, isr122, isr123, isr124, isr125, isr126, isr127
    .quad isr128, isr129, isr130, isr131, isr132, isr133, isr134, isr135
    .quad isr136, isr137, isr138, isr139, isr140, isr141, isr142, isr143
    .quad isr144, isr145, isr146, isr147, isr148, isr149, isr150, isr151
    .quad isr152, isr153, isr154, isr155, isr156, isr157, isr158, isr159
    .quad isr160, isr161, isr162, isr163, isr164, isr165, isr166, isr167
    .quad isr168, isr169, isr170, isr171, isr172, isr173, isr174, isr175
    .quad isr176, isr177, isr178, isr179, isr180, isr181, isr182, isr183
    .quad isr184, isr185, isr186, isr187, isr188, isr189, isr190, isr191
    .quad isr192, isr193, isr194, isr195, isr196, isr197, isr198, isr199
    .quad isr200, isr201, isr202, isr203, isr204, isr205, isr206, isr207
    .quad isr208, isr209, isr210, isr211, isr212, isr213, isr214, isr215
    .quad isr216, isr217, isr218, isr219, isr220, isr221, isr222, isr223
    .quad isr224, isr225, isr226, isr227, isr228, isr229, isr230, isr231
    .quad isr232, isr233, isr234, isr235, isr236, isr237, isr238, isr239
    .quad isr240, isr241, isr242, isr243, isr244, isr245, isr246, isr247
    .quad isr248, isr249, isr250, isr251, isr252, isr253, isr254, isr255
