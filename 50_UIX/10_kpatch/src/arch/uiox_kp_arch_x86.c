/**
 * @file  uiox_kp_arch_x86.c
 * @brief UIOX kpatch — x86_64 trampoline and jump stub writer.
 * @date  2026-07-07
 */

 #include "../../include/uiox_kp_arch.h"

 static void kp_memcpy_x86(void *d, const void *s, size_t n)
 { uint8_t *dp=(uint8_t*)d; const uint8_t *sp=(const uint8_t*)s;
   while(n--)*dp++=*sp++; }
 
 static void wr32x(uint8_t *p, uint32_t v)
 { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
   p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24); }
 
 static void wr64x(uint8_t *p, uint64_t v)
 { wr32x(p,(uint32_t)v); wr32x(p+4,(uint32_t)(v>>32)); }
 
 static int x86_write_jump(uint8_t *buf, uintptr_t src, uintptr_t dst,
                             uint8_t max_len)
 {
     int64_t rel = (int64_t)dst - (int64_t)(src + 5u);
 
     /* Near JMP: E9 <rel32> — range ±2 GB */
     if (rel >= (int64_t)(int32_t)0x80000000LL &&
         rel <= (int64_t)(int32_t)0x7FFFFFFFLL &&
         max_len >= X86_JMP_NEAR_SIZE) {
         buf[0] = X86_JMP_NEAR_OPCODE;
         wr32x(buf + 1u, (uint32_t)(int32_t)rel);
         return X86_JMP_NEAR_SIZE;
     }
 
     /* Far JMP: FF 25 00 00 00 00; .quad target (14 bytes) */
     if (max_len >= X86_JMP_FAR_SIZE) {
         buf[0] = X86_JMP_FAR_OP0;   /* FF        */
         buf[1] = X86_JMP_FAR_OP1;   /* 25        */
         wr32x(buf + 2u, 0u);         /* RIP+0     */
         wr64x(buf + 6u, (uint64_t)dst);
         return X86_JMP_FAR_SIZE;
     }
     return -1;
 }
 
 static uint8_t x86_jump_size(uintptr_t src, uintptr_t dst)
 {
     int64_t rel = (int64_t)dst - (int64_t)(src + 5u);
     if (rel >= (int64_t)(int32_t)0x80000000LL &&
         rel <= (int64_t)(int32_t)0x7FFFFFFFLL)
         return X86_JMP_NEAR_SIZE;
     return X86_JMP_FAR_SIZE;
 }
 
 static int x86_build_trampoline(uint8_t *buf, uint32_t max,
                                   uintptr_t tramp_addr,
                                   const uint8_t *saved, uint8_t slen,
                                   uintptr_t orig_func)
 {
     if (max < (uint32_t)slen + X86_JMP_FAR_SIZE) return -1;
     kp_memcpy_x86(buf, saved, slen);
     uintptr_t rest   = orig_func + slen;
     uintptr_t jmp_at = tramp_addr + slen;
     int jlen = x86_write_jump(buf + slen, jmp_at, rest,
                                 (uint8_t)(max - slen));
     if (jlen < 0) return -1;
     return (int)slen + jlen;
 }
 
 static void x86_icache_flush(uintptr_t addr, size_t len)
 {
     /* x86: clflush each cache line (64 bytes) */
     uintptr_t a = addr & ~63u;
     uintptr_t e = addr + len;
     while (a < e) {
         __asm__ volatile("clflush (%0)" :: "r"(a) : "memory");
         a += 64u;
     }
     __asm__ volatile("mfence" ::: "memory");
 }
 
 static int x86_make_writable(uintptr_t addr, size_t len)
 {
     UIOX_KP_UNUSED(addr); UIOX_KP_UNUSED(len);
     /* UIOX kernel is mapped RWX in early boot — no CR0.WP manipulation
      * needed for the firmware/simulation environment.
      * Production: clear CR0.WP → patch → set CR0.WP */
     return 0;
 }
 
 static void x86_restore_protect(uintptr_t addr, size_t len)
 { UIOX_KP_UNUSED(addr); UIOX_KP_UNUSED(len); }
 
 static const uiox_kp_arch_ops_t s_x86_ops = {
     .arch              = UIOX_KP_ARCH_X86_64,
     .name              = "x86_64",
     .write_jump        = x86_write_jump,
     .jump_size         = x86_jump_size,
     .build_trampoline  = x86_build_trampoline,
     .icache_flush      = x86_icache_flush,
     .make_writable     = x86_make_writable,
     .restore_protect   = x86_restore_protect,
 };
 
 void uiox_kp_arch_x86_register(void)
 { uiox_kp_arch_register(&s_x86_ops); }
 