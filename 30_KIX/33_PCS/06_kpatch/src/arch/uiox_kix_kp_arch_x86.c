/**
 * @file  uiox_kix_kp_arch_x86.c
 * @brief UIOX kpatch — x86_64 trampoline and jump stub writer.
 * @version 1.1.0  @date 2026-10-02
 */

 #include "../../include/uiox_kix_kp_arch.h"

 static void kp_memcpy_x86(void *d, const void *s, size_t n)
 { uint8_t *dp=(uint8_t*)d; const uint8_t *sp=(const uint8_t*)s;
   while(n--)*dp++=*sp++; }
 
 static void wr32x(uint8_t *p, uint32_t v)
 { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
   p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24); }
 
 static void wr64x(uint8_t *p, uint64_t v)
 { wr32x(p,(uint32_t)v); wr32x(p+4,(uint32_t)(v>>32)); }
 
 /* The rel32 in E9 is measured from the END of the instruction, src + 5.
  * Both writers use this one expression so they cannot disagree about
  * where the near/far boundary falls. */
 #define X86_REL32_IN_RANGE(rel)  \
     ((rel) >= (int64_t)(int32_t)0x80000000LL && \
      (rel) <= (int64_t)(int32_t)0x7FFFFFFFLL)
 
 static int x86_write_jump(uint8_t *buf, uintptr_t src, uintptr_t dst,
                             uint8_t max_len)
 {
     int64_t rel = (int64_t)dst - (int64_t)(src + 5u);
 
     if (X86_REL32_IN_RANGE(rel) && max_len >= X86_JMP_NEAR_SIZE) {
         buf[0] = X86_JMP_NEAR_OPCODE;
         wr32x(buf + 1u, (uint32_t)(int32_t)rel);
         return X86_JMP_NEAR_SIZE;
     }
 
     if (max_len >= X86_JMP_FAR_SIZE) {
         buf[0] = X86_JMP_FAR_OP0;
         buf[1] = X86_JMP_FAR_OP1;
         wr32x(buf + 2u, 0u);
         wr64x(buf + 6u, (uint64_t)dst);
         return X86_JMP_FAR_SIZE;
     }
     return -1;
 }
 
 static uint8_t x86_jump_size(uintptr_t src, uintptr_t dst)
 {
     int64_t rel = (int64_t)dst - (int64_t)(src + 5u);
     if (X86_REL32_IN_RANGE(rel))
         return UIOX_KIX_KP_JMP_SIZE_X86_NEAR;
     return UIOX_KIX_KP_JMP_SIZE_X86_FAR;
 }
 
 static int x86_build_trampoline(uint8_t *buf, uint32_t max,
                                   uintptr_t tramp_addr,
                                   const uint8_t *saved, uint8_t slen,
                                   uintptr_t orig_func)
 {
     uintptr_t rest;
     uintptr_t jmp_at;
     int       jlen;
 
     if (max < (uint32_t)slen + X86_JMP_FAR_SIZE) return -1;
 
     kp_memcpy_x86(buf, saved, slen);
 
     rest   = orig_func + slen;
     jmp_at = tramp_addr + slen;
 
     jlen = x86_write_jump(buf + slen, jmp_at, rest, (uint8_t)(max - slen));
     if (jlen < 0) return -1;
     return (int)slen + jlen;
 }
 
 static void x86_icache_flush(uintptr_t addr, size_t len)
 {
     uintptr_t a = addr & ~63u;
     uintptr_t e = addr + len;
     while (a < e) {
         __asm__ volatile("clflush (%0)" :: "r"(a) : "memory");
         a += 64u;
     }
     __asm__ volatile("mfence" ::: "memory");
 }
 
 /* NO-OPS — see uiox_kix_kp_patch.h.  A production kernel clears CR0.WP,
  * patches, and sets it again. */
 static int x86_make_writable(uintptr_t addr, size_t len)
 { UIOX_KIX_KP_UNUSED(addr); UIOX_KIX_KP_UNUSED(len); return 0; }
 
 static void x86_restore_protect(uintptr_t addr, size_t len)
 { UIOX_KIX_KP_UNUSED(addr); UIOX_KIX_KP_UNUSED(len); }
 
 static const uiox_kix_kp_arch_ops_t s_x86_ops = {
     .arch              = UIOX_KIX_KP_ARCH_X86_64,
     .name              = "x86_64",
     .write_jump        = x86_write_jump,
     .jump_size         = x86_jump_size,
     .max_jump_size     = X86_JMP_FAR_SIZE,
     .build_trampoline  = x86_build_trampoline,
     .icache_flush      = x86_icache_flush,
     .make_writable     = x86_make_writable,
     .restore_protect   = x86_restore_protect,
 };
 
 _Static_assert(X86_JMP_FAR_SIZE == 14u,
                "x86_64 max_jump_size must match its own far constant");
 
 void uiox_kix_kp_arch_x86_register(void)
 { uiox_kix_kp_arch_register(&s_x86_ops); }
 