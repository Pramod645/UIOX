/**
 * @file  uiox_kix_kp_arch.h
 * @brief UIOX Live Kernel Patching — arch-specific trampoline opcodes.
 *
 * Each architecture needs:
 *   1. A jump stub written at the start of the original function.
 *   2. A trampoline stub that executes the saved original bytes and then
 *      jumps past the overwritten prologue, so new_func() can call the
 *      original.
 *
 * @version 1.1.0
 * @date    2026-10-02
 */

 #ifndef UIOX_KIX_KP_ARCH_H
 #define UIOX_KIX_KP_ARCH_H
 
 #include "uiox_kix_kp_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Architecture-specific ops vtable
  * ====================================================================== */
 
 typedef struct {
     uiox_kix_kp_arch_t arch;
     const char        *name;
 
     /** Write a jump from @src to @dst into @buf.  Returns bytes written,
      *  or negative on error. */
     int (*write_jump)      (uint8_t *buf, uintptr_t src, uintptr_t dst,
                              uint8_t max_len);
 
     /** Bytes that must be saved — the stub length for this src/dst pair. */
     uint8_t (*jump_size)   (uintptr_t src, uintptr_t dst);
 
     /**
      * The WIDEST jump stub this architecture can emit.
      *
      * Used by the engine to size a trampoline allocation, so it must be
      * the FAR form's length.  It exists because the engine is
      * arch-neutral and the value is not.
      *
      * Each arch source carries a _Static_assert tying this field to its
      * own UIOX_KIX_KP_JMP_SIZE_<ARCH>_FAR constant.
      */
     uint8_t  max_jump_size;
 
     /**
      * Build a trampoline at @tramp_addr:
      *   1. Executes @saved_bytes (the original prologue)
      *   2. Jumps to @orig_func + @saved_len (rest of the original)
      *
      * The CALLER passes the bare orig_func; this function adds saved_len,
      * because the return offset is an encoding decision.  All four arch
      * sources do so.
      */
     int (*build_trampoline)(uint8_t *tramp_buf, uint32_t tramp_max,
                              uintptr_t tramp_addr,
                              const uint8_t *saved_bytes, uint8_t saved_len,
                              uintptr_t orig_func);
 
     /** Flush instruction cache for [addr, addr+len). */
     void (*icache_flush)   (uintptr_t addr, size_t len);
 
     /**
      * Make memory writable.  ALL FOUR IMPLEMENTATIONS ARE NO-OPS because
      * text is RWX during firmware init — see uiox_kix_kp_patch.h.
      */
     int (*make_writable)   (uintptr_t addr, size_t len);
 
     /** Restore memory protection after patching. */
     void (*restore_protect)(uintptr_t addr, size_t len);
 
 } uiox_kix_kp_arch_ops_t;
 
 /* =========================================================================
  * ARM64-specific constants
  * ====================================================================== */
 
 #define ARM64_B_OPCODE          0x14000000u
 #define ARM64_B_RANGE           (128u * 1024u * 1024u)
 
 #define ARM64_LDR_X16_8         0x58000050u  /* LDR x16, PC+8 */
 #define ARM64_BR_X16            0xD61F0200u  /* BR x16        */
 
 /* =========================================================================
  * ARM32-specific constants
  *
  * The range is ±32 MB of BYTES, expressed below as a byte count.  The
  * arch source converts to the signed-word count the B immediate carries
  * by shifting right 2 — do not compare a byte offset against this value
  * directly.
  * ====================================================================== */
 
 #define ARM32_B_COND_AL         0xEA000000u
 #define ARM32_B_RANGE           (32u * 1024u * 1024u)
 
 /* LDR pc, [pc, #-4] — the operand is #-4, not #0: PC reads as
  * instruction_address + 8 in ARM mode, so the word that follows sits at
  * PC-4. */
 #define ARM32_LDR_PC_PC         0xE51FF004u
 
 /* =========================================================================
  * x86-64-specific constants
  * ====================================================================== */
 
 #define X86_JMP_NEAR_OPCODE     0xE9u
 #define X86_JMP_NEAR_SIZE       5u
 
 /* FF 25 00 00 00 00  JMP [RIP+0]  ; <8-byte target>   = 14 bytes */
 #define X86_JMP_FAR_OP0         0xFFu
 #define X86_JMP_FAR_OP1         0x25u
 #define X86_JMP_FAR_SIZE        14u
 
 #define X86_NOP                 0x90u
 
 /* =========================================================================
  * RISC-V RV64-specific constants
  *
  * RISC-V has NO PC-relative branch with a large displacement and NO
  * absolute jump.  JAL reaches ±1 MB; JALR reaches ±2 KB from a register.
  * The far form is the two-instruction sequence the assembler emits for
  * `call`.
  * ====================================================================== */
 
 #define RISCV_JAL_OPCODE        0x6Fu
 #define RISCV_JAL_RANGE         (1024u * 1024u)
 
 #define RISCV_AUIPC_OPCODE      0x17u
 #define RISCV_JALR_OPCODE       0x67u
 
 /* The scratch register the far form loads the target into.  x6 is t1 — a
  * temporary, so clobbering it in a function PROLOGUE is free. */
 #define RISCV_SCRATCH_REG       6u
 
 /* =========================================================================
  * Arch ops registration / lookup
  * ====================================================================== */
 
 void                            uiox_kix_kp_arch_register  (const uiox_kix_kp_arch_ops_t *ops);
 const uiox_kix_kp_arch_ops_t   *uiox_kix_kp_arch_get       (void);
 uiox_kix_kp_arch_t              uiox_kix_kp_arch_current   (void);
 const char                     *uiox_kix_kp_arch_name      (uiox_kix_kp_arch_t arch);
 
 void uiox_kix_kp_arch_arm64_register   (void);
 void uiox_kix_kp_arch_arm32_register   (void);
 void uiox_kix_kp_arch_x86_register     (void);
 void uiox_kix_kp_arch_riscv64_register (void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KIX_KP_ARCH_H */
 