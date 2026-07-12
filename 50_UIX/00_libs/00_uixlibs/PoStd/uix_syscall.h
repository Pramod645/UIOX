/**
 * @file  uix_syscall.h
 * @brief UIOX POSIX — arch-portable syscall() wrapper and errno wiring.
 *
 * Bridges uix_archSysCall.h (ARM64/ARM32/x86_64 SVC/syscall macros)
 * to POSIX-compatible errno + return-value semantics.
 *
 * Usage:
 *   long ret = uix_syscall(SYS_READ, fd, buf, count);
 *   if (ret < 0) { uix_errno = (int)(-ret); return -1; }
 *
 * Place: 50_UIX/00_libs/00_uixlibs/PoStd/uix_syscall.h
 */

 #ifndef UIX_SYSCALL_H
 #define UIX_SYSCALL_H
 
 #include "../../../40_SystemCallInterface/uix_sys.h"
 #include "../../../40_SystemCallInterface/uix_archSysCall.h"
 #include "uix_errno.h"
 #include "uix_stddef.h"
 #include "uix_stdint.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * uix_syscall — portable 0–6 argument syscall dispatcher
  *
  * Selects the correct my_syscallN macro from uix_archSysCall.h based on
  * argument count.  Returns raw kernel value (negative = -errno on error).
  * ====================================================================== */
 
 static inline long uix_syscall0(long num)
 {
 #ifdef __aarch64__
     return my_syscall0(num);
 #elif defined(__arm__)
     return my_syscall0(num);
 #elif defined(__x86_64__)
     return my_syscall0(num);
 #else
     (void)num; return -ENOSYS;
 #endif
 }
 
 static inline long uix_syscall1(long num, long a1)
 {
 #if defined(__aarch64__) || defined(__arm__) || defined(__x86_64__)
     return my_syscall1(num, a1);
 #else
     (void)num;(void)a1; return -ENOSYS;
 #endif
 }
 
 static inline long uix_syscall2(long num, long a1, long a2)
 {
 #if defined(__aarch64__) || defined(__arm__) || defined(__x86_64__)
     return my_syscall2(num, a1, a2);
 #else
     (void)num;(void)a1;(void)a2; return -ENOSYS;
 #endif
 }
 
 static inline long uix_syscall3(long num, long a1, long a2, long a3)
 {
 #if defined(__aarch64__) || defined(__arm__) || defined(__x86_64__)
     return my_syscall3(num, a1, a2, a3);
 #else
     (void)num;(void)a1;(void)a2;(void)a3; return -ENOSYS;
 #endif
 }
 
 static inline long uix_syscall4(long num, long a1, long a2,
                                   long a3, long a4)
 {
 #if defined(__aarch64__) || defined(__arm__) || defined(__x86_64__)
     return my_syscall4(num, a1, a2, a3, a4);
 #else
     (void)num;(void)a1;(void)a2;(void)a3;(void)a4; return -ENOSYS;
 #endif
 }
 
 static inline long uix_syscall5(long num, long a1, long a2,
                                   long a3, long a4, long a5)
 {
 #if defined(__aarch64__) || defined(__arm__) || defined(__x86_64__)
     return my_syscall5(num, a1, a2, a3, a4, a5);
 #else
     (void)num;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5; return -ENOSYS;
 #endif
 }
 
 static inline long uix_syscall6(long num, long a1, long a2, long a3,
                                   long a4, long a5, long a6)
 {
 #if defined(__aarch64__) || defined(__arm__) || defined(__x86_64__)
     return my_syscall6(num, a1, a2, a3, a4, a5, a6);
 #else
     (void)num;(void)a1;(void)a2;(void)a3;
     (void)a4;(void)a5;(void)a6; return -ENOSYS;
 #endif
 }
 
 /* =========================================================================
  * uix_syscall — variadic-style wrapper (sets errno, returns -1 on error)
  * ====================================================================== */
 
 #define UIX_SYSCALL_ERR(ret)  \
     do { if ((ret) < 0) { uix_errno = (int)(-(ret)); return -1L; } } while(0)
 
 /* =========================================================================
  * Portable syscall() alias matching POSIX long syscall(long number, ...)
  * Implemented in uix_syscall.c
  * ====================================================================== */
 
 long uix_syscall(long number, ...);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIX_SYSCALL_H */
 