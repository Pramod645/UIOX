/**
 * @file  uiox_kp_arch_arm64.c
 * @brief UIOX kpatch — ARM64 trampoline and jump stub writer.
 * @date  2026-07-07
 */

 #include "../../include/uiox_kp_arch.h"

 /* ── No libc — inline memcpy ────────────────────────────────── */
 static void kp_memcpy(void *d, const void *s, size_t n)
 { uint8_t *dp=(uint8_t*)d; const uint8_t *sp=(const uint8_t*)s;
   while(n--)*dp++=*sp++; }
 static void kp_memset(void *d, int v, size_t n)
 { uint8_t *dp=(uint8_t*)d; while(n--)*dp++=(uint8_t)v; }
 
 /* ── Inline 32-bit LE write ─────────────────────────────────── */
 static void wr32(uint8_t *p, uint32_t v)
 { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
   p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24); }
 
 /* ── Inline 64-bit LE write ─────────────────────────────────── */
 static void wr64(uint8_t *p, uint64_t v)
 { wr32(p,(uint32_t)v); wr32(p+4,(uint32_t)(v>>32)); }
 
 /* =========================================================================
  * write_jump: write a branch from src to dst into buf
  * ====================================================================== */
 
 static int arm64_write_jump(uint8_t *buf, uintptr_t src, uintptr_t dst,
                               uint8_t max_len)
 {
     int64_t offset = (int64_t)dst - (int64_t)src;
 
     /* Near: B <imm26> — range ±128 MB */
     if (offset >= -(int64_t)ARM64_B_RANGE &&
         offset <  (int64_t)ARM64_B_RANGE  && max_len >= 4u) {
         uint32_t imm26 = (uint32_t)((offset >> 2) & 0x3FFFFFFu);
         wr32(buf, ARM64_B_OPCODE | imm26);
         return 4;
     }
 
     /* Far: LDR x16, #8; BR x16; .quad target (16 bytes) */
     if (max_len >= 16u) {
         wr32(buf + 0u, ARM64_LDR_X16_8);   /* LDR x16, PC+8 */
         wr32(buf + 4u, ARM64_BR_X16);       /* BR  x16       */
         wr64(buf + 8u, (uint64_t)dst);      /* target addr   */
         return 16;
     }
     return -1;
 }
 
 /* =========================================================================
  * jump_size: return bytes needed for a jump from src to dst
  * ====================================================================== */
 
 static uint8_t arm64_jump_size(uintptr_t src, uintptr_t dst)
 {
     int64_t off = (int64_t)dst - (int64_t)src;
     if (off >= -(int64_t)ARM64_B_RANGE &&
         off <  (int64_t)ARM64_B_RANGE)
         return UIOX_KP_JMP_SIZE_ARM64_NEAR;
     return UIOX_KP_JMP_SIZE_ARM64_FAR;
 }
 
 /* =========================================================================
  * build_trampoline:
  *   [saved_bytes copied verbatim]
  *   [jump to orig_func + saved_len]
  * ====================================================================== */
 
 static int arm64_build_trampoline(uint8_t *buf, uint32_t max,
                                     uintptr_t tramp_addr,
                                     const uint8_t *saved, uint8_t slen,
                                     uintptr_t orig_func)
 {
     if (max < (uint32_t)slen + 16u) return -1;
 
     /* Copy saved bytes */
     kp_memcpy(buf, saved, slen);
 
     /* Jump to rest of original */
     uintptr_t rest   = orig_func + slen;
     uintptr_t jmp_at = tramp_addr + slen;
     int jlen = arm64_write_jump(buf + slen, jmp_at, rest, (uint8_t)(max - slen));
     if (jlen < 0) return -1;
     return (int)slen + jlen;
 }
 
 /* =========================================================================
  * icache_flush
  * ====================================================================== */
 
 static void arm64_icache_flush(uintptr_t addr, size_t len)
 {
     uintptr_t a = addr & ~63u;
     uintptr_t e = addr + len;
     while (a < e) {
         __asm__ volatile("dc cvau, %0" :: "r"(a) : "memory");
         a += 64u;
     }
     __asm__ volatile("dsb ish" ::: "memory");
     __asm__ volatile("ic ialluis" ::: "memory");
     __asm__ volatile("dsb ish; isb" ::: "memory");
 }
 
 /* =========================================================================
  * make_writable / restore_protect
  * On UIOX early firmware: kernel text is mapped RWX — no extra work needed.
  * A production implementation would use break-before-make MMU technique.
  * ====================================================================== */
 
 static int arm64_make_writable(uintptr_t addr, size_t len)
 {
     UIOX_KP_UNUSED(addr); UIOX_KP_UNUSED(len);
     /* UIOX maps kernel RWX during firmware init — nothing to do */
     return 0;
 }
 
 static void arm64_restore_protect(uintptr_t addr, size_t len)
 {
     UIOX_KP_UNUSED(addr); UIOX_KP_UNUSED(len);
 }
 
 static const uiox_kp_arch_ops_t s_arm64_ops = {
     .arch              = UIOX_KP_ARCH_ARM64,
     .name              = "arm64",
     .write_jump        = arm64_write_jump,
     .jump_size         = arm64_jump_size,
     .build_trampoline  = arm64_build_trampoline,
     .icache_flush      = arm64_icache_flush,
     .make_writable     = arm64_make_writable,
     .restore_protect   = arm64_restore_protect,
 };
 
 void uiox_kp_arch_arm64_register(void)
 {
     uiox_kp_arch_register(&s_arm64_ops);
 }
 