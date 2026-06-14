/*
 * uiox_boot_main.c  —  UIOX bootloader 7-stage pipeline.
 */
#include "uiox_boot.h"

static uiox_boot_args_t g_args;
static uboot_mem_map_t  g_mmap;
static uboot_fat32_t    g_fat32;
static uboot_blk_ops_t  g_blk;

#define KBUF_SZ (16u*1024u*1024u)
static uboot_u8_t g_kbuf[KBUF_SZ] __attribute__((aligned(4096)));

static void panic(const char *m)
{uboot_puts("\r\n[PANIC] ");uboot_puts(m);uboot_puts("\r\n");
for(;;) uboot_cpu_idle();}

static uboot_u32_t arch_id(void)
{
#if   defined(__aarch64__) return UBOOT_ARCH_ARM64;
#elif defined(__arm__)     return UBOOT_ARCH_ARM32;
#else                      return UBOOT_ARCH_X86_64;
#endif
}

int uiox_boot_main(uboot_u64_t dtb, uboot_u32_t flags)
{
    (void)flags;
    uboot_banner();

    /* Stage 2: Memory */
    uboot_puts("[BOOT] Stage 2: Memory\r\n");
    int rc;
#if   defined(__aarch64__)
    rc=uboot_mem_probe_arm64(&g_mmap,dtb);
#elif defined(__arm__)
    rc=uboot_mem_probe_arm32(&g_mmap,(uboot_u32_t*)(uboot_addr_t)dtb);
#else
    rc=uboot_mem_probe_x86(&g_mmap,(uboot_u32_t)(uboot_addr_t)dtb);
#endif
    if(rc!=UBOOT_OK) panic("Memory probe failed");
    uboot_mem_print(&g_mmap);
    uboot_heap_init(g_mmap.regions[0].base+0x100000u,UIOX_BOOT_HEAP_SIZE);
    uboot_puts("  OK\r\n");

    /* Stage 3: Storage */
    uboot_puts("[BOOT] Stage 3: Storage\r\n");
    rc=uboot_blk_probe_emmc(&g_blk);
    if(rc!=UBOOT_OK) rc=uboot_blk_probe_nvme(&g_blk,0u);
    if(rc!=UBOOT_OK) rc=uboot_blk_probe_usb(&g_blk);
    if(rc==UBOOT_OK){
        if(uboot_fat32_init(&g_fat32,&g_blk,2048u)==UBOOT_OK)
            uboot_puts("  FAT32 mounted\r\n");
    } else {
        uboot_puts("  No storage — simulation mode\r\n");
    }

    /* Stage 4: Load */
    uboot_puts("[BOOT] Stage 4: Load kernel\r\n");
    uboot_size_t ksize=0;
    rc=uboot_fat32_load(&g_fat32,UIOX_KERNEL_FILENAME,
                         g_kbuf,KBUF_SZ,&ksize);
    if(rc==UBOOT_ENODEV||rc==UBOOT_ENOENT){
        uboot_puts("  No kernel file — QEMU simulation handoff\r\n");
        ksize=0; goto handoff;
    }
    if(rc!=UBOOT_OK) panic("Kernel load failed");
    uboot_printf("  Loaded %u bytes\r\n",(uboot_u32_t)ksize);

    /* Stage 5: Verify */
    uboot_puts("[BOOT] Stage 5: Verify\r\n");
    rc=uboot_verify_image(g_kbuf,ksize);
    if(rc==UBOOT_EVERIFY) panic("SHA-256 mismatch — image tampered!");
    if(rc==UBOOT_EBADIMG) uboot_puts("  No UIOX header — treating as raw ELF\r\n");
    else                  uboot_puts("  Verify OK\r\n");

handoff:;
    /* Stage 6: ELF load */
    uboot_puts("[BOOT] Stage 6: ELF\r\n");
    uboot_u64_t entry=0;
    rc=uboot_elf64_load(g_kbuf,ksize,&entry);
    if(rc!=UBOOT_OK){
#if   defined(__aarch64__)
        entry=UIOX_KERNEL_LOAD_ARM64;
#elif defined(__arm__)
        entry=UIOX_KERNEL_LOAD_ARM32;
#else
        entry=UIOX_KERNEL_LOAD_X86;
#endif
        if(ksize) uboot_memcpy((void*)(uboot_addr_t)entry,g_kbuf,ksize);
        uboot_puts("  Flat binary load\r\n");
    }
    uboot_puts("  Entry: "); uboot_puthex64(entry); uboot_puts("\r\n");

    /* Stage 7: Handoff */
    uboot_puts("[BOOT] Stage 7: Handoff\r\n");
    uboot_args_init(&g_args);
    g_args.arch        =arch_id();
    g_args.mem_map     =;
    g_args.kernel_entry=entry;
    g_args.kernel_size =ksize;
    g_args.dtb_phys    =dtb;
    g_args.uart_baud   =UIOX_UART_DEFAULT_BAUD;
#if   defined(__aarch64__)
    g_args.uart_type=UBOOT_UART_PL011; g_args.uart_base=0x09000000ULL;
#elif defined(__arm__)
    g_args.uart_type=UBOOT_UART_PL011; g_args.uart_base=0x10009000ULL;
#else
    g_args.uart_type=UBOOT_UART_16550; g_args.uart_base=0x3F8u;
#endif
    const char *cl=UIOX_CMDLINE_DEFAULT;
    uboot_size_t ci=0;
    while(cl[ci]&&ci<UIOX_BOOT_CMDLINE-1u) g_args.cmdline[ci++]=cl[ci];
    g_args.cmdline[ci]='\0';
    uboot_args_checksum(&g_args);
    uboot_args_print(&g_args);

    uboot_u64_t ap;
#if   defined(__aarch64__)
    ap=UIOX_BOOT_ARGS_PHYS_ARM64;
#elif defined(__arm__)
    ap=UIOX_BOOT_ARGS_PHYS_ARM32;
#else
    ap=UIOX_BOOT_ARGS_PHYS_X86;
#endif
    uboot_memcpy((void*)(uboot_addr_t)ap,&g_args,sizeof(g_args));
    uboot_mem_barrier();

    uboot_puts("[BOOT] Jumping to kernel...\r\n\r\n");
    uboot_jump_to_kernel(entry,ap);
    return 0;
}
