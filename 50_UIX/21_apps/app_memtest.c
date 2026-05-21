#include "../../40_SystemCallInterface/uix_sys.h"

int main(void)
{
    /* mmap anonymous page */
    void *ptr = sys_mmap((void*)0, 4096,
                          0x1|0x2,       /* PROT_READ|PROT_WRITE */
                          0x02|0x20,     /* MAP_PRIVATE|MAP_ANONYMOUS */
                          -1, 0);
    if (ptr == (void*)-1) sys_exit(1);

    /* write to mapped region */
    char *p = (char*)ptr;
    p[0]='U'; p[1]='I'; p[2]='X'; p[3]='\0';

    /* change protection to read-only */
    sys_mprotect(ptr, 4096, 0x1 /* PROT_READ */);

    /* unmap */
    sys_munmap(ptr, 4096);

    /* brk test */
    uix_uintptr_t old_brk = sys_brk(0);
    uix_uintptr_t new_brk = sys_brk(old_brk + 4096);
    (void)new_brk;

    return 0;
}
