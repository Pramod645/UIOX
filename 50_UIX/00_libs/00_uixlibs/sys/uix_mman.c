#include "uix_mman.h"
#include "../PoStd/uix_stdlib.h"
#include "../PoStd/uix_errno.h"
#include "../PoStd/uix_string.h"

#include "../uix_sys.h"


void *uix_mmap(void *addr, uix_size_t length, int prot,
               int flags, int fd, uix_off_t offset)
{
    //extern void *sys_mmap(void *, uix_size_t, int, int,
    //                       int, uix_off_t) __attribute__((weak));
    //if (sys_mmap) return sys_mmap(addr, length, prot, flags, fd, offset);

    //if (flags & UIX_MAP_ANONYMOUS) {
    //    void *p = uix_malloc(length);
    //    if (p) uix_memset(p, 0, length);
    //    return p ? p : UIX_MAP_FAILED;
    //}
    //(void)addr; (void)fd; (void)offset;
    //uix_errno = UIX_ENOMEM;
    // UIX_MAP_FAILED;
    return sys_mmap(addr, length, prot, flags, fd, offset);
}

int uix_munmap(void *addr, uix_size_t length)
{
   // extern int sys_munmap(void *, uix_size_t) __attribute__((weak));
    //if (sys_munmap) return sys_munmap(addr, length);
    //uix_free(addr);
    //(void)length;
    //return 0;
    return sys_munmap(addr, length);
}

int uix_mprotect(void *addr, uix_size_t len, int prot)
{
    //extern int sys_mprotect(void *, uix_size_t, int)
    //    __attribute__((weak));
    //if (sys_mprotect) return sys_mprotect(addr, len, prot);
    //(void)addr; (void)len; (void)prot;
    //return 0;
    return sys_mprotect(addr, len, prot);
}

int uix_msync(void *addr, uix_size_t length, int flags)
{
    (void)addr; (void)length; (void)flags;
    return 0;
}

int uix_madvise(void *addr, uix_size_t length, int advice)
{
    (void)addr; (void)length; (void)advice;
    return 0;
}

int uix_mlock(const void *addr, uix_size_t len)
{
    (void)addr; (void)len; return 0;
}

int uix_munlock(const void *addr, uix_size_t len)
{
    (void)addr; (void)len; return 0;
}

int uix_mlockall(int flags)   { (void)flags; return 0; }
int uix_munlockall(void)      { return 0; }

void *uix_mremap(void *old_address, uix_size_t old_size,
                 uix_size_t new_size, int flags)
{
    (void)flags;
    void *p = uix_malloc(new_size);
    if (!p) return UIX_MAP_FAILED;
    uix_size_t copy = old_size < new_size ? old_size : new_size;
    uix_memcpy(p, old_address, copy);
    uix_free(old_address);
    return p;
}

int uix_shm_open(const char *name, int oflag, uix_mode_t mode)
{
    //extern int sys_shm_open(const char *, int, uix_mode_t)
    //    __attribute__((weak));
    //return sys_shm_open ? sys_shm_open(name, oflag, mode)
    //                   : (uix_errno = UIX_ENOENT, -1);
    return sys_shm_open(name, oflag, mode);
}

int uix_shm_unlink(const char *name)
{
    //extern int sys_shm_unlink(const char *) __attribute__((weak));
    //return sys_shm_unlink ? sys_shm_unlink(name)
    //                      : (uix_errno = UIX_ENOENT, -1);
    return sys_shm_unlink(name);
}

/* ***This is End of file, there is no more line should be added after this line*** */
