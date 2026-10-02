/**
 * @file  uiox_kix_kp_arch_arm32.c
 * @brief UIOX kpatch — ARMv7-A trampoline and jump stub writer.
 *
 * ARM32_B_RANGE is now USED rather than dead: the macro is in BYTES and
 * the B immediate carries a signed WORD count, so the comparison shifts
 * the macro right by 2 rather than re-deriving ±32 MB by hand.
 *
 * @version 1.1.0  @date 2026-10-02
 */

 #include "../../include/uiox_kix_kp_arch.h"

 static void kp_memcpy32(void *d, const void *s, size_t n)
 { uint8_t *dp=(uint8_t*)d; const uint8_t *sp=(const uint8_t*)s;
   while(n--)*dp++=*sp++; }
 
 static void wr32a(uint8_t *p, uint32_t v)
 { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
   p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24); }
 
 /* B <offset>: signed WORD count in bits 23:0; target is
  * PC + 8 + (offset << 2). */
 #define ARM32_B_WORD_LIMIT  ((int32_t)(ARM32_B_RANGE >> 2))
 
 static int arm32_write_jump(uint8_t *buf, uintptr_t src, uintptr_t dst,
                               uint8_t max_len)
 {
     int32_t offset = (int32_t)((dst - (src + 8u)) >> 2);
 
     if (offset >= -ARM32_B_WORD_LIMIT &&
         offset <   ARM32_B_WORD_LIMIT && max_len >= 4u) {
         wr32a(buf, ARM32_B_COND_AL | ((uint32_t)offset & 0x00FFFFFFu));
         return 4;
     }
 
     /* Far: LDR pc, [pc, #-4]; .word target (8 bytes) */
     if (max_len >= 8u) {
         wr32a(buf + 0u, ARM32_LDR_PC_PC);
         wr32a(buf + 4u, (uint32_t)dst);
         return 8;
     }
     return -1;
 }
 
 static uint8_t arm32_jump_size(uintptr_t src, uintptr_t dst)
 {
     int32_t off = (int32_t)((dst - (src + 8u)) >> 2);
     if (off >= -ARM32_B_WORD_LIMIT && off < ARM32_B_WORD_LIMIT)
         return UIOX_KIX_KP_JMP_SIZE_ARM32_NEAR;
     return UIOX_KIX_KP_JMP_SIZE_ARM32_FAR;
 }
 
 static int arm32_build_trampoline(uint8_t *buf, uint32_t max,
                                     uintptr_t tramp_addr,
                                     const uint8_t *saved, uint8_t slen,
                                     uintptr_t orig_func)
 {
     uintptr_t rest;
     uintptr_t jmp_at;
     int       jlen;
 
     if (max < (uint32_t)slen + 8u) return -1;
 
     kp_memcpy32(buf, saved, slen);
 
     rest   = orig_func + slen;
     jmp_at = tramp_addr + slen;
 
     jlen = arm32_write_jump(buf + slen, jmp_at, rest, (uint8_t)(max - slen));
     if (jlen < 0) return -1;
     return (int)slen + jlen;
 }
 
 static void arm32_icache_flush(uintptr_t addr, size_t len)
 {
     uintptr_t a = addr & ~31u;
     uintptr_t e = addr + len;
     uint32_t  z = 0u;
 
     while (a < e) {
         __asm__ volatile("mcr p15,0,%0,c7,c10,1" :: "r"(a) : "memory");
         a += 32u;
     }
     __asm__ volatile("mcr p15,0,%0,c7,c5,0" :: "r"(z) : "memory");
     __asm__ volatile("dsb; isb" ::: "memory");
 }
 
 /* NO-OPS — see uiox_kix_kp_patch.h.  On ARMv7-A the production mechanism
  * is the MMU TEX/AP bits on the text mapping. */
 static int  arm32_make_writable  (uintptr_t a, size_t l)
 { UIOX_KIX_KP_UNUSED(a); UIOX_KIX_KP_UNUSED(l); return 0; }
 static void arm32_restore_protect(uintptr_t a, size_t l)
 { UIOX_KIX_KP_UNUSED(a); UIOX_KIX_KP_UNUSED(l); }
 
 static const uiox_kix_kp_arch_ops_t s_arm32_ops = {
     .arch              = UIOX_KIX_KP_ARCH_ARM32,
     .name              = "arm32",
     .write_jump        = arm32_write_jump,
     .jump_size         = arm32_jump_size,
     .max_jump_size     = UIOX_KIX_KP_JMP_SIZE_ARM32_FAR,
     .build_trampoline  = arm32_build_trampoline,
     .icache_flush      = arm32_icache_flush,
     .make_writable     = arm32_make_writable,
     .restore_protect   = arm32_restore_protect,
 };
 
 _Static_assert(UIOX_KIX_KP_JMP_SIZE_ARM32_FAR == 8u,
                "arm32 max_jump_size must match its own far constant");
 
 void uiox_kix_kp_arch_arm32_register(void)
 { uiox_kix_kp_arch_register(&s_arm32_ops); }
 