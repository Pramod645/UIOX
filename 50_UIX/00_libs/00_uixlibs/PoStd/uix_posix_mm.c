/**
 * @file  uix_posix_mm.c
 * @brief UIOX POSIX — memory management syscall implementations.
 */

 #include "uix_posix_mm.h"

 static void *_mret(long r)
 {
     if (r < 0L && r > -4096L) {
         uix_errno = (int)(-r);
         return (void *)-1L;
     }
     return (void *)r;
 }
 
 static inline long _ret(long r)
 {
     if (r < 0L) { uix_errno = (int)(-r); return -1L; }
     return r;
 }
 
 void *uix_mmap(void *addr, size_t length, int prot, int flags,
                 int fd, off_t offset)
 {
     return _mret(uix_syscall6(SYS_MMAP,
                                (long)addr, (long)length,
                                (long)prot, (long)flags,
                                (long)fd,   (long)offset));
 }
 
 int uix_munmap(void *addr, size_t length)
 {
     return (int)_ret(uix_syscall2(SYS_MUNMAP,(long)addr,(long)length));
 }
 
 int uix_mprotect(void *addr, size_t length, int prot)
 {
     return (int)_ret(uix_syscall3(SYS_MPROTECT,
                                    (long)addr,(long)length,(long)prot));
 }
 
 int uix_mlock(const void *addr, size_t length)
 {
     return (int)_ret(uix_syscall2(SYS_MLOCK,(long)addr,(long)length));
 }
 
 int uix_munlock(const void *addr, size_t length)
 {
     return (int)_ret(uix_syscall2(SYS_MUNLOCK,(long)addr,(long)length));
 }
 
 int uix_msync(void *addr, size_t length, int flags)
 {
     return (int)_ret(uix_syscall3(SYS_MSYNC,
                                    (long)addr,(long)length,(long)flags));
 }
 
 void *uix_mremap(void *old_addr, size_t old_size,
                   size_t new_size, int flags, void *new_addr)
 {
     return _mret(uix_syscall5(SYS_MREMAP,
                                (long)old_addr,(long)old_size,
                                (long)new_size,(long)flags,
                                (long)new_addr));
 }
 
 static void *s_brk_cur = NULL;
 
 void *uix_brk_sbrk(intptr_t increment)
 {
     if (!s_brk_cur) {
         s_brk_cur = (void *)uix_syscall1(SYS_BRK, 0L);
     }
     if (increment == 0) return s_brk_cur;
     void *new_brk = (char *)s_brk_cur + increment;
     void *result  = (void *)uix_syscall1(SYS_BRK, (long)new_brk);
     if (result == new_brk) { s_brk_cur = result; return s_brk_cur; }
     uix_errno = ENOMEM;
     return (void *)-1L;
 }
 