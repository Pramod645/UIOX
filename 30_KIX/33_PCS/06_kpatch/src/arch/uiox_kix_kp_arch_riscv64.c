/**
 * @file  uiox_kix_kp_arch_riscv64.c
 * @brief UIOX kpatch — RISC-V RV64 trampoline and jump stub writer.
 *
 * ── why this arch is not like the others ────────────────────────────────
 * RISC-V has no PC-relative branch with a large displacement and no
 * absolute jump.  JAL reaches ±1 MB; JALR reaches ±2 KB from a register.
 * The far form is the two-instruction sequence the assembler emits for
 * `call`:
 *
 *   auipc x6, <hi20>      x6 = PC + (hi20 << 12)
 *   jalr  x0, x6, <lo12>  jump to x6 + lo12
 *
 * x6 is t1 — a temporary, so no callee-saved register is clobbered by a
 * jump that replaces a function PROLOGUE.
 *
 * ── instruction cache ───────────────────────────────────────────────────
 * RISC-V has no architected I-cache and no flush instruction.  fence.i
 * orders the instruction stream against prior stores to the same hart,
 * which is what a patch needs.  A platform with a genuinely incoherent
 * cache must supply more.
 *
 * @version 1.0.0
 * @date    2026-10-02
 */

 #include "../../include/uiox_kix_kp_arch.h"

 static void kp_memcpy_rv(void *d, const void *s, size_t n)
 { uint8_t *dp=(uint8_t*)d; const uint8_t *sp=(const uint8_t*)s;
   while(n--)*dp++=*sp++; }
 
 static void wr32r(uint8_t *p, uint32_t v)
 { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
   p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24); }
 
 static void wr64r(uint8_t *p, uint64_t v)
 { wr32r(p,(uint32_t)v); wr32r(p+4,(uint32_t)(v>>32)); }
 
 /* =========================================================================
  * Instruction encoders
  * ====================================================================== */
 
 /* JAL rd, offset — opcode 0x6F.
  * imm[20] | imm[10:1] | imm[11] | imm[19:12] | rd | opcode
  * The immediate is a signed multiple of 2, so bit 0 is not encoded. */
 static uint32_t rv_encode_jal(uint32_t rd, int32_t offset)
 {
     uint32_t imm = (uint32_t)offset;
     uint32_t u = 0u;
 
     u |= ((imm >> 20) & 0x1u)   << 31;
     u |= ((imm >>  1) & 0x3FFu) << 21;
     u |= ((imm >> 11) & 0x1u)   << 20;
     u |= ((imm >> 12) & 0xFFu)  << 12;
     u |= (rd & 0x1Fu)           <<  7;
     u |= 0x6Fu;
     return u;
 }
 
 /* AUIPC rd, imm20 — opcode 0x17.  rd = PC + (imm20 << 12). */
 static uint32_t rv_encode_auipc(uint32_t rd, int32_t imm20)
 {
     uint32_t u = 0u;
     u |= ((uint32_t)imm20 & 0xFFFFFu) << 12;
     u |= (rd & 0x1Fu)                 <<  7;
     u |= 0x17u;
     return u;
 }
 
 /* JALR rd, rs1, imm12 — opcode 0x67, funct3 = 0.
  * rd = x0 discards the return address, giving a plain jump. */
 static uint32_t rv_encode_jalr(uint32_t rd, uint32_t rs1, int32_t imm12)
 {
     uint32_t u = 0u;
     u |= ((uint32_t)imm12 & 0xFFFu) << 20;
     u |= (rs1 & 0x1Fu)              << 15;
     u |= 0x0u                       << 12;
     u |= (rd & 0x1Fu)               <<  7;
     u |= 0x67u;
     return u;
 }
 
 /* =========================================================================
  * write_jump
  * ====================================================================== */
 
 #define RV_SCRATCH_X6   6u
 
 static int riscv_write_jump(uint8_t *buf, uintptr_t src, uintptr_t dst,
                               uint8_t max_len)
 {
     int64_t off = (int64_t)dst - (int64_t)src;
 
     /* Near: JAL x0, off — signed 21-bit byte offset, bit 0 ignored */
     if (off >= -1048576LL && off <= 1048575LL && max_len >= 4u) {
         wr32r(buf, rv_encode_jal(0u, (int32_t)off));
         return 4;
     }
 
     /* Far: AUIPC x6, hi20 ; JALR x0, x6, lo12
      *
      * lo12 is a SIGNED 12-bit field, so hi20 must be rounded UP when the
      * low half would otherwise be negative.  Getting this wrong by one
      * puts the jump 4 KB off — landing in the middle of some other
      * function rather than failing. */
     if (max_len >= 8u) {
         int64_t diff = (int64_t)dst - (int64_t)src;
         int32_t lo12 = (int32_t)(diff & 0xFFF);
 
         if (lo12 & 0x800) lo12 -= 0x1000;
 
         int64_t hi20 = ((diff - lo12) >> 12);
 
         wr32r(buf + 0u, rv_encode_auipc(RV_SCRATCH_X6, (int32_t)hi20));
         wr32r(buf + 4u, rv_encode_jalr(0u, RV_SCRATCH_X6, lo12));
         return 8;
     }
     return -1;
 }
 
 static uint8_t riscv_jump_size(uintptr_t src, uintptr_t dst)
 {
     int64_t off = (int64_t)dst - (int64_t)src;
     if (off >= -1048576LL && off <= 1048575LL)
         return UIOX_KIX_KP_JMP_SIZE_RISCV64_NEAR;
     return UIOX_KIX_KP_JMP_SIZE_RISCV64_FAR;
 }
 
 /* =========================================================================
  * build_trampoline
  *
  * LIMITATION: the saved prologue is copied as raw bytes.  Safe here
  * because a prologue does not normally contain a PC-relative instruction
  * whose offset depends on where it sits — but compilers DO emit auipc in
  * PIC prologues, and a copy of one would compute from the trampoline's
  * PC.  A production version would decode and relocate; this one copies,
  * and the limitation is stated rather than assumed away.
  * ====================================================================== */
 static int riscv_build_trampoline(uint8_t *buf, uint32_t max,
                                     uintptr_t tramp_addr,
                                     const uint8_t *saved, uint8_t slen,
                                     uintptr_t orig_func)
 {
     uintptr_t rest;
     uintptr_t jmp_at;
     int       jlen;
 
     if (max < (uint32_t)slen + UIOX_KIX_KP_JMP_SIZE_RISCV64_FAR) return -1;
 
     kp_memcpy_rv(buf, saved, slen);
 
     rest   = orig_func + slen;
     jmp_at = tramp_addr + slen;
 
     jlen = riscv_write_jump(buf + slen, jmp_at, rest, (uint8_t)(max - slen));
     if (jlen < 0) return -1;
     return (int)slen + jlen;
 }
 
 static void riscv_icache_flush(uintptr_t addr, size_t len)
 {
     UIOX_KIX_KP_UNUSED(addr);
     UIOX_KIX_KP_UNUSED(len);
 
     __asm__ volatile("fence.i" ::: "memory");
 }
 
 /* NO-OPS — see uiox_kix_kp_patch.h.  A production RISC-V kernel would set
  * the W bit on the text PTE (Sv39/Sv48) or open a PMP entry. */
 static int riscv_make_writable(uintptr_t addr, size_t len)
 { UIOX_KIX_KP_UNUSED(addr); UIOX_KIX_KP_UNUSED(len); return 0; }
 
 static void riscv_restore_protect(uintptr_t addr, size_t len)
 { UIOX_KIX_KP_UNUSED(addr); UIOX_KIX_KP_UNUSED(len); }
 
 static const uiox_kix_kp_arch_ops_t s_riscv_ops = {
     .arch              = UIOX_KIX_KP_ARCH_RISCV64,
     .name              = "riscv64",
     .write_jump        = riscv_write_jump,
     .jump_size         = riscv_jump_size,
     .max_jump_size     = UIOX_KIX_KP_JMP_SIZE_RISCV64_FAR,
     .build_trampoline  = riscv_build_trampoline,
     .icache_flush      = riscv_icache_flush,
     .make_writable     = riscv_make_writable,
     .restore_protect   = riscv_restore_protect,
 };
 
 _Static_assert(UIOX_KIX_KP_JMP_SIZE_RISCV64_FAR == 8u,
                "riscv64 max_jump_size must match its own far constant");
 _Static_assert(RISCV_SCRATCH_REG == 6u,
                "riscv64 far jump assumes x6 is the scratch register");
 
 void uiox_kix_kp_arch_riscv64_register(void)
 { uiox_kix_kp_arch_register(&s_riscv_ops); }
 