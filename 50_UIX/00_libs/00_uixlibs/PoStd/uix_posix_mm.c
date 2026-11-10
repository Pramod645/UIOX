/**
 * @file  uix_posix_mm.c
 * @brief UIOX POSIX — memory management syscall implementations.
 */

 #include "uix_posix_mm.h"
#include "uix_errno.h"
#include "uix_sys.h"
 #include "uix_errno.h"     /* uix_errno, UIX_ENOMEM */

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
 
 void *uix_mmap(void *addr, uix_size_t length, int prot, int flags,
                 int fd, uix_off_t offset)
 {
     return _mret(my_syscall6(SYS_MMAP,
                                (long)addr, (long)length,
                                (long)prot, (long)flags,
                                (long)fd,   (long)offset));
 }
 
 int uix_munmap(void *addr, uix_size_t length)
 {
     return (int)_ret(my_syscall2(SYS_MUNMAP,(long)addr,(long)length));
 }
 
 int uix_mprotect(void *addr, uix_size_t length, int prot)
 {
     return (int)_ret(my_syscall3(SYS_MPROTECT,
                                    (long)addr,(long)length,(long)prot));
 }
 
 int uix_mlock(const void *addr, uix_size_t length)
 {
     return (int)_ret(my_syscall2(SYS_MLOCK,(long)addr,(long)length));
 }
 
 int uix_munlock(const void *addr, uix_size_t length)
 {
     return (int)_ret(my_syscall2(SYS_MUNLOCK,(long)addr,(long)length));
 }
 
 int uix_msync(void *addr, uix_size_t length, int flags)
 {
     return (int)_ret(my_syscall3(SYS_MSYNC,
                                    (long)addr,(long)length,(long)flags));
 }
 
 void *uix_mremap(void *old_addr, uix_size_t old_size,
                   uix_size_t new_size, int flags, void *new_addr)
 {
     return _mret(my_syscall5(SYS_MREMAP,
                                (long)old_addr,(long)old_size,
                                (long)new_size,(long)flags,
                                (long)new_addr));
 }
 
 static void *s_brk_cur = NULL;
 
 void *uix_brk_sbrk(uix_intptr_t increment)
 {
     if (!s_brk_cur) {
         s_brk_cur = (void *)my_syscall1(SYS_BRK, 0L);
     }
     if (increment == 0) return s_brk_cur;
     void *new_brk = (char *)s_brk_cur + increment;
     void *result  = (void *)my_syscall1(SYS_BRK, (long)new_brk);
     if (result == new_brk) { s_brk_cur = result; return s_brk_cur; }
     uix_errno = UIX_ENOMEM;
     return (void *)-1L;
 }
 