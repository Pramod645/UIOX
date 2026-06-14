/*
 * uiox_boot_handoff.c  —  ELF64 loader + boot args + kernel jump.
 */
#include "uiox_boot_handoff.h"
#include "uiox_boot_console.h"
#include "uiox_boot_mem.h"

int uboot_elf64_load(const void *img, uboot_size_t size,
                      uboot_u64_t *entry_out)
{
    if(!img||size<64u) return UBOOT_EBADIMG;
    const uboot_u8_t *b=(const uboot_u8_t*)img;
    if(b[0]!=0x7F||b[1]!='E'||b[2]!='L'||b[3]!='F') return UBOOT_EBADIMG;
    if(b[4]!=2) return UBOOT_ENOTSUP;

    const uboot_elf64_ehdr_t *eh=(const uboot_elf64_ehdr_t*)img;
    *entry_out=eh->e_entry;

    for(uboot_u16_t i=0;i<eh->e_phnum;i++){
        const uboot_elf64_phdr_t *ph=(const uboot_elf64_phdr_t*)
            (b+eh->e_phoff+i*(uboot_size_t)eh->e_phentsize);
        if(ph->p_type!=ELF_PT_LOAD||ph->p_filesz==0) continue;
        uboot_memcpy((void*)(uboot_addr_t)ph->p_paddr,
                     b+ph->p_offset,(uboot_size_t)ph->p_filesz);
        if(ph->p_memsz>ph->p_filesz)
            uboot_memset((void*)(uboot_addr_t)(ph->p_paddr+ph->p_filesz),
                         0,(uboot_size_t)(ph->p_memsz-ph->p_filesz));
        uboot_puts("  SEG paddr="); uboot_puthex64(ph->p_paddr);
        uboot_printf("  sz=%u\r\n",(uboot_u32_t)ph->p_filesz);
    }
    return UBOOT_OK;
}

void uboot_args_init(uiox_boot_args_t *a)
{uboot_memset(a,0,sizeof(*a));a->magic=UIOX_BOOT_MAGIC;a->version=1u;}

void uboot_args_checksum(uiox_boot_args_t *a)
{
    a->checksum=0;
    const uboot_u32_t *p=(const uboot_u32_t*)a;
    uboot_u32_t s=0;
    for(uboot_size_t i=0;i<sizeof(*a)/4u;i++) s+=p[i];
    a->checksum=~s+1u;
}

void uboot_args_print(const uiox_boot_args_t *a)
{
    uboot_puts("  args@"); uboot_puthex64((uboot_u64_t)(uboot_addr_t)a);
    uboot_puts("\r\n  entry="); uboot_puthex64(a->kernel_entry);
    uboot_puts("\r\n  dtb=");   uboot_puthex64(a->dtb_phys);
    uboot_printf("\r\n  cmd: %s\r\n",a->cmdline);
}

/* ── arch-specific kernel jump (never returns) ───────────── */
#if defined(__aarch64__)
void uboot_jump_to_kernel(uboot_u64_t entry, uboot_u64_t args)
{
    __asm__ volatile(
        "mov x0,%0\n\t mov x1,xzr\n\t mov x2,xzr\n\t mov x3,xzr\n\t br %1"
        ::"r"(args),"r"(entry):"x0","x1","x2","x3","memory");
    __builtin_unreachable();
}
#elif defined(__arm__)
void uboot_jump_to_kernel(uboot_u64_t entry, uboot_u64_t args)
{
    uboot_u32_t e=(uboot_u32_t)entry, a=(uboot_u32_t)args;
    __asm__ volatile(
        "mov r0,#0\n\t mov r1,#0\n\t mov r2,%0\n\t bx %1"
        ::"r"(a),"r"(e):"r0","r1","r2","memory");
    __builtin_unreachable();
}
#elif defined(__x86_64__)
void uboot_jump_to_kernel(uboot_u64_t entry, uboot_u64_t args)
{
    __asm__ volatile(
        "movq %0,%%rdi\n\t xorq %%rsi,%%rsi\n\t jmpq *%1"
        ::"r"(args),"r"(entry):"rdi","rsi","memory");
    __builtin_unreachable();
}
#else
void uboot_jump_to_kernel(uboot_u64_t e, uboot_u64_t a)
{(void)e;(void)a;for(;;);}
#endif
