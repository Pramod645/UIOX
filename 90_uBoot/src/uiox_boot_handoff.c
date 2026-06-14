/*
 * uiox_boot_handoff.c - ELF64 loader + boot args + kernel jump.
 */
#include "uiox_boot_handoff.h"
#include "uiox_boot_console.h"
#include "uiox_boot_mem.h"
#include <string.h>

/* -- ELF64 loader ------------------------------------------ */
int uboot_elf64_load(const void *img, uboot_size_t size,
                      uboot_u64_t *entry_out)
{
    if (!img || size < 64u) return UBOOT_EBADIMG;
    const uboot_u8_t *b = (const uboot_u8_t *)img;
    if (b[0]!=0x7F||b[1]!='E'||b[2]!='L'||b[3]!='F')
        return UBOOT_EBADIMG;
    if (b[4] != 2) return UBOOT_ENOTSUP; /* not ELF64 */

    const uboot_elf64_ehdr_t *eh =
        (const uboot_elf64_ehdr_t *)img;
    *entry_out = eh->e_entry;

    const uboot_u8_t *phbase = b + eh->e_phoff;
    for (uboot_u16_t i = 0; i < eh->e_phnum; i++) {
        const uboot_elf64_phdr_t *ph =
            (const uboot_elf64_phdr_t *)(phbase +
             i * (uboot_size_t)eh->e_phentsize);
        if (ph->p_type != ELF_PT_LOAD) continue;
        if (ph->p_filesz == 0) continue;
        uboot_memcpy((void *)(uboot_addr_t)ph->p_paddr,
                     b + ph->p_offset,
                     (uboot_size_t)ph->p_filesz);
        if (ph->p_memsz > ph->p_filesz) {
            uboot_memset((void *)(uboot_addr_t)
                         (ph->p_paddr + ph->p_filesz),
                         0,
                         (uboot_size_t)(ph->p_memsz - ph->p_filesz));
        }
        uboot_printf("  LOAD paddr=");
        uboot_puthex64(ph->p_paddr);
        uboot_printf("  filesz=%u\r\n",
            (uboot_u32_t)ph->p_filesz);
    }
    return UBOOT_OK;
}

/* -- Boot args --------------------------------------------- */
void uboot_args_init(uiox_boot_args_t *args)
{
    uboot_memset(args, 0, sizeof(*args));
    args->magic   = UIOX_BOOT_MAGIC;
    args->version = 1u;
}

void uboot_args_checksum(uiox_boot_args_t *args)
{
    args->checksum = 0;
    const uboot_u32_t *p =
        (const uboot_u32_t *)args;
    uboot_u32_t sum = 0;
    for (uboot_size_t i = 0;
         i < sizeof(*args) / 4u; i++)
        sum += p[i];
    args->checksum = ~sum + 1u;
}

void uboot_args_print(const uiox_boot_args_t *args)
{
    uboot_printf("  Boot args at  : ");
    uboot_puthex64((uboot_u64_t)(uboot_addr_t)args);
    uboot_printf("\r\n  Arch          : ");
    uboot_puthex32(args->arch);
    uboot_printf("\r\n  Kernel entry  : ");
    uboot_puthex64(args->kernel_entry);
    uboot_printf("\r\n  DTB phys      : ");
    uboot_puthex64(args->dtb_phys);
    uboot_printf("\r\n  Cmdline       : %s\r\n", args->cmdline);
}

/* -- Arch-specific kernel jump ----------------------------- */
#if defined(__aarch64__)
void uboot_jump_to_kernel(uboot_u64_t entry, uboot_u64_t args_phys)
{
    __asm__ volatile(
        "mov  x0, %0\n\t"
        "mov  x1, xzr\n\t"
        "mov  x2, xzr\n\t"
        "mov  x3, xzr\n\t"
        "br   %1\n\t"
        :
        : "r"(args_phys), "r"(entry)
        : "x0","x1","x2","x3","memory"
    );
    __builtin_unreachable();
}

#elif defined(__arm__)
void uboot_jump_to_kernel(uboot_u64_t entry, uboot_u64_t args_phys)
{
    uboot_u32_t e32 = (uboot_u32_t)entry;
    uboot_u32_t a32 = (uboot_u32_t)args_phys;
    __asm__ volatile(
        "mov r0, #0\n\t"
        "mov r1, #0\n\t"
        "mov r2, %0\n\t"
        "bx  %1\n\t"
        :
        : "r"(a32), "r"(e32)
        : "r0","r1","r2","memory"
    );
    __builtin_unreachable();
}

#elif defined(__x86_64__)
void uboot_jump_to_kernel(uboot_u64_t entry, uboot_u64_t args_phys)
{
    __asm__ volatile(
        "movq  %0, %%rdi\n\t"
        "xorq  %%rsi, %%rsi\n\t"
        "xorq  %%rdx, %%rdx\n\t"
        "jmpq  *%1\n\t"
        :
        : "r"(args_phys), "r"(entry)
        : "rdi","rsi","rdx","memory"
    );
    __builtin_unreachable();
}
#else
void uboot_jump_to_kernel(uboot_u64_t e, uboot_u64_t a)
{ (void)e; (void)a; for(;;); }
#endif
