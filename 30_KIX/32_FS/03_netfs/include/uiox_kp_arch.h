/**
 * @file  uiox_kp_arch.h
 * @brief UIOX Live Kernel Patching — arch-specific trampoline opcodes.
 *
 * Each architecture needs:
 *   1. A jump stub written at the start of the original function.
 *   2. A trampoline stub that:
 *      a. Executes the saved original bytes
 *      b. Jumps to the rest of the original function
 *   This allows new_func() to optionally call the original.
 *
 * @version 1.0.0
 * @date    2026-07-07
 */

 #ifndef UIOX_KP_ARCH_H
 #define UIOX_KP_ARCH_H
 
 #include "uiox_kp_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Architecture-specific ops vtable
  * ====================================================================== */
 
 typedef struct {
     uiox_kp_arch_t arch;
     const char    *name;
 
     /**
      * Write a jump from @src to @dst into the @buf byte array.
      * @buf must have at least @max_len bytes.
      * Returns number of bytes written, or negative on error.
      */
     int (*write_jump)      (uint8_t *buf, uintptr_t src, uintptr_t dst,
                              uint8_t max_len);
 
     /**
      * Return the minimum number of bytes that must be saved
      * (jump stub size for this architecture).
      */
     uint8_t (*jump_size)   (uintptr_t src, uintptr_t dst);
 
     /**
      * Build a trampoline at @tramp_addr that:
      *   1. Executes @saved_bytes (original prologue)
      *   2. Jumps to @orig_func + @saved_len (rest of original)
      * Writes the trampoline into @tramp_buf (size @tramp_max).
      * Returns bytes written.
      */
     int (*build_trampoline)(uint8_t *tramp_buf, uint32_t tramp_max,
                              uintptr_t tramp_addr,
                              const uint8_t *saved_bytes, uint8_t saved_len,
                              uintptr_t orig_func);
 
     /**
      * Flush instruction cache for [addr, addr+len).
      * Required after writing jump stubs or trampolines.
      */
     void (*icache_flush)   (uintptr_t addr, size_t len);
 
     /**
      * Make memory at [addr, addr+len) writable (remove write-protect).
      * Returns 0 on success.
      */
     int (*make_writable)   (uintptr_t addr, size_t len);
 
     /**
      * Restore memory protection after patching.
      */
     void (*restore_protect)(uintptr_t addr, size_t len);
 
 } uiox_kp_arch_ops_t;
 
 /* =========================================================================
  * ARM64-specific constants
  * ====================================================================== */
 
 /* ARM64 near branch: B <offset> (signed 26-bit imm, range ±128 MB) */
 #define ARM64_B_OPCODE          0x14000000u
 #define ARM64_B_RANGE           (128u * 1024u * 1024u)
 
 /* ARM64 far branch sequence (absolute indirect):
  *   LDR x16, #8       ; load 64-bit target from +8
  *   BR  x16           ; branch to target
  *   .quad <target>    ; 8-byte target address
  */
 #define ARM64_LDR_X16_8         0x58000050u  /* LDR x16, PC+8 */
 #define ARM64_BR_X16            0xD61F0200u  /* BR x16        */
 
 /* =========================================================================
  * ARM32-specific constants
  * ====================================================================== */
 
 /* ARM32 near branch: B <offset> (signed 24-bit imm, range ±32 MB) */
 #define ARM32_B_COND_AL         0xEA000000u
 #define ARM32_B_RANGE           (32u * 1024u * 1024u)
 
 /* ARM32 far branch:
  *   LDR pc, [pc, #0]  ; load absolute address
  *   .word <target>
  */
 #define ARM32_LDR_PC_PC         0xE51FF004u  /* LDR pc, [pc, #-4] */
 
 /* =========================================================================
  * x86-64-specific constants
  * ====================================================================== */
 
 /* x86_64 near JMP: E9 <rel32> (5 bytes, range ±2 GB) */
 #define X86_JMP_NEAR_OPCODE     0xE9u
 #define X86_JMP_NEAR_SIZE       5u
 
 /* x86_64 far JMP via RIP-relative indirect:
  *   FF 25 00 00 00 00   JMP [RIP+0]
  *   <8-byte target>
  * Total: 14 bytes
  */
 #define X86_JMP_FAR_OP0         0xFFu
 #define X86_JMP_FAR_OP1         0x25u
 #define X86_JMP_FAR_SIZE        14u
 
 /* x86_64 NOP */
 #define X86_NOP                 0x90u
 
 /* =========================================================================
  * Arch ops registration / lookup
  * ====================================================================== */
 
 void                      uiox_kp_arch_register  (const uiox_kp_arch_ops_t *ops);
 const uiox_kp_arch_ops_t *uiox_kp_arch_get       (void);
 uiox_kp_arch_t            uiox_kp_arch_current   (void);
 const char               *uiox_kp_arch_name      (uiox_kp_arch_t arch);
 
 /* Per-arch registration (called by arch source at init) */
 void uiox_kp_arch_arm64_register (void);
 void uiox_kp_arch_arm32_register (void);
 void uiox_kp_arch_x86_register   (void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KP_ARCH_H */
 