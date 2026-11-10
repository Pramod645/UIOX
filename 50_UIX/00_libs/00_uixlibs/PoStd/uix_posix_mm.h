/**
 * @file  uix_posix_mm.h
 * @brief UIOX POSIX — memory management syscall wrappers.
 */

 #ifndef UIX_POSIX_MM_H
 #define UIX_POSIX_MM_H
 
 #include "uix_sys.h"
 #include "../sys/uix_mman.h"
 #include "uix_stdint.h"

 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 void  *uix_mmap    (void *addr, uix_size_t length, int prot, int flags,
                      int fd, uix_off_t offset);
 int    uix_munmap  (void *addr, uix_size_t length);
 int    uix_mprotect(void *addr, uix_size_t length, int prot);
 int    uix_mlock   (const void *addr, uix_size_t length);
 int    uix_munlock (const void *addr, uix_size_t length);
 int    uix_msync   (void *addr, uix_size_t length, int flags);
 void  *uix_brk_sbrk(uix_intptr_t increment);
 
 #define mmap        uix_mmap
 #define munmap      uix_munmap
 #define mprotect    uix_mprotect
 #define mlock       uix_mlock
 #define munlock     uix_munlock
 #define msync       uix_msync
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIX_POSIX_MM_H */
 