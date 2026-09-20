/**
 * @file  uiox_boot_main.c
 * @brief UIOX Bootloader — 7-stage boot pipeline.
 *
 * Stage 1   Hardware init      (board bring-up → UART → GIC/PLIC/VIC)
 * Stage 2   Memory             (DTB /memory probe → region table)
 * Stage 2.5 Device tree        (/chosen bootargs, /soc peripheral bases)
 * Stage 3   Storage            (media_select: virtio | sdmmc)
 * Stage 4   Load kernel        (UNFS mount → kernel image to DRAM)
 * Stage 5   Verify             (SHA-256 + image header check)
 * Stage 6   ELF load           (ELF64 segment copy / flat binary)
 * Stage 7   Handoff            (build boot-args → arch jump)
 *
 * @version 1.1.0  @date 2026-09-20
 */

 #include "uiox_boot.h"
 #include "uiox_boot_mem.h"
 #include "uiox_boot_media.h"
 
 #ifndef UIOX_UNUSED
   #define UIOX_UNUSED(x) ((void)(x))
 #endif
 
 /* =========================================================================
  * Platform configuration
  * ====================================================================== */
 
 #ifndef UIOX_CMDLINE
   #define UIOX_CMDLINE  "console=ttyAMA0,115200 rw quiet"
 #endif
 
 #if defined(__aarch64__)
   #define UIOX_KERN_LOAD_PA   0x40080000ULL
   #define UIOX_ARGS_PA        0x40070000ULL
   #define UIOX_DTB_FALLBACK   0x40000000ULL
   #define UIOX_ARCH_ID        UIOX_ARCH_ARM64
   #define UIOX_ARCH_STR       "ARM64"
 #elif defined(__arm__)
   #define UIOX_KERN_LOAD_PA   0x00200000ULL
   #define UIOX_ARGS_PA        0x00180000ULL
   #define UIOX_DTB_FALLBACK   0x00010000ULL
   #define UIOX_ARCH_ID        UIOX_ARCH_ARM32
   #define UIOX_ARCH_STR       "ARM32"
 #elif defined(__riscv)
   #define UIOX_KERN_LOAD_PA   0x80280000ULL
   #define UIOX_ARGS_PA        0x80270000ULL
   #define UIOX_DTB_FALLBACK   0x80200000ULL
   #define UIOX_ARCH_ID        UIOX_ARCH_RV64
   #define UIOX_ARCH_STR       "RISC-V"
 #else
   #define UIOX_KERN_LOAD_PA   0x00200000ULL
   #define UIOX_ARGS_PA        0x00180000ULL
   #define UIOX_DTB_FALLBACK   0x00000000ULL
   #define UIOX_ARCH_ID        UIOX_ARCH_X86_64
   #define UIOX_ARCH_STR       "x86_64"
 #endif
 
 #define UIOX_KERN_MAX_SIZE  (32u * 1024u * 1024u)   /* 32 MB */
 
 /* =========================================================================
  * Arch HW registration
  * ====================================================================== */
 #if defined(__aarch64__)
   extern void uiox_boot_hw_arm64_register(void);
   #define UIOX_HW_REGISTER()  uiox_boot_hw_arm64_register()
 #elif defined(__arm__)
   extern void uiox_boot_hw_arm32_register(void);
   #define UIOX_HW_REGISTER()  uiox_boot_hw_arm32_register()
 #elif defined(__x86_64__)
   extern void uiox_boot_hw_x86_register(void);
   #define UIOX_HW_REGISTER()  uiox_boot_hw_x86_register()
 #else
   extern void uiox_boot_hw_riscv64_register(void);
   #define UIOX_HW_REGISTER()  uiox_boot_hw_riscv64_register()
 #endif
 
 /* =========================================================================
  * Stage-3 media drivers (constructors defined in each driver file)
  * ====================================================================== */
 extern const uiox_boot_media_ops_t *uiox_boot_media_virtio(void);
 extern const uiox_boot_media_ops_t *uiox_boot_media_sdmmc(void);
 
 /* =========================================================================
  * Stage-4 UNFS bridge (src/uiox_boot_bridge_unfs.c)
  * ====================================================================== */
 extern int unfs_boot_probe(void);
 //extern int unfs_boot_load(void);
 
 /* =========================================================================
  * uiox_boot_main — the pipeline
  * ====================================================================== */
 void __attribute__((noreturn))
 uiox_boot_main(uint64_t dtb_pa, uint64_t x1, uint64_t x2)
 {
     UIOX_UNUSED(x1);
     UIOX_UNUSED(x2);
 
     /* ================================================================== */
     /* Stage 1: Hardware init                                             */
     /*   UIOX_HW_REGISTER() installs the HAL vtable; its init() calls     */
     /*   uiox_board_bringup() (clock/PLL/power/pin-mux) and then the UART */
     /*   + interrupt controller. Nothing prints until this returns.       */
     /* ================================================================== */
     UIOX_HW_REGISTER();
     uiox_boot_console_init();
 
     BOOT_BANNER(UIOX_ARCH_STR);
     BOOT_LOG(1, "HW init");
     BOOT_OK();
 
     /* ================================================================== */
     /* Stage 2: Memory probe — /memory from the DTB                       */
     /* ================================================================== */
     BOOT_LOG(2, "Memory");
 
     uiox_mem_map_t  mem_map;
     uiox_boot_err_t rc = uiox_boot_mem_probe(dtb_pa, &mem_map);
     if (rc != UIOX_BOOT_OK)
         BOOT_FATAL("memory probe failed (err=%d)", (int)rc);
 
     uiox_boot_mem_print(&mem_map);
 
     uiox_bump_alloc_t alloc;
     rc = uiox_boot_mem_alloc_init(&alloc, &mem_map,
                                   (uintptr_t)UIOX_ARGS_PA,
                                   sizeof(uiox_boot_args_t) + 0x10000u);
     if (rc != UIOX_BOOT_OK)
         BOOT_FATAL("bump allocator init failed (err=%d)", (int)rc);
 
     BOOT_OK();
 
     /* ================================================================== */
     /* Stage 2.5: Device-tree runtime extraction                          */
     /*   /chosen  -> bootargs   (overrides UIOX_CMDLINE)                  */
     /*   /soc     -> peripheral base addresses                            */
     /* ================================================================== */
     BOOT_LOG(2, "Device tree");
 
     char               bootargs[UIOX_IMAGE_CMDLINE_MAX];
     uiox_soc_runtime_t soc;
     bootargs[0] = '\0';
 
     if (dtb_pa != 0u) {
         uiox_boot_err_t drc = uiox_boot_dt_apply(dtb_pa, bootargs,
                                                  sizeof(bootargs), &soc);
         if (drc == UIOX_BOOT_OK) {
             if (bootargs[0] != '\0')
                 uiox_boot_printf("  bootargs: %s\n", bootargs);
             uiox_boot_printf("  uart0=0x%llx gic=0x%llx virtio=0x%llx\n",
                              (unsigned long long)soc.uart0_base,
                              (unsigned long long)soc.gic_dist_base,
                              (unsigned long long)soc.virtio_base);
         } else {
             uiox_boot_puts("  no /chosen or /soc nodes — using defaults\n");
         }
     } else {
         uiox_boot_puts("  no DTB (x86) — using compile-time defaults\n");
     }
 
     const char *cmdline = (bootargs[0] != '\0') ? bootargs : UIOX_CMDLINE;
     BOOT_OK();
 
     /* ================================================================== */
     /* Stage 3: Storage — media layer selects the device                  */
     /* ================================================================== */
     BOOT_LOG(3, "Storage");
 
     uiox_boot_media_register(uiox_boot_media_virtio());
     uiox_boot_media_register(uiox_boot_media_sdmmc());
 
     bool storage_ok = false;
     uiox_boot_media_kind_t media = uiox_boot_media_select();
     if (media != UIOX_MEDIA_NONE) {
         storage_ok = true;
         uiox_boot_printf("  media: %s\n", uiox_boot_media_active_name());
         BOOT_OK();
     } else {
         uiox_boot_puts("  no boot media — cannot load a kernel\n");
     }
 
     /* ================================================================== */
     /* Stage 4: Load kernel from UNFS                                     */
     /* ================================================================== */
     BOOT_LOG(4, "Load kernel");
 
     void *load_buf = uiox_boot_mem_alloc(&alloc,
                                           UIOX_KERN_MAX_SIZE + 512u,
                                           4096u);
     if (!load_buf)
         BOOT_FATAL("no memory for kernel load buffer");
 
     size_t   loaded       = 0u;
     bool     has_header   = false;
     bool     is_elf       = false;
     uint64_t entry_pa     = UIOX_KERN_LOAD_PA;
     uiox_image_hdr_t img_hdr;
     uint64_t bytes_loaded = 0u;
 
     if (storage_ok) {
         if (unfs_boot_probe() == 0) {
             uiox_boot_puts("  UNFS mounted\n");
             if (unfs_boot_load(UIOX_KERN_LOAD_PA,
                                (uint64_t)UIOX_KERN_MAX_SIZE,
                                &bytes_loaded, &img_hdr) == 0) {
                 loaded     = (size_t)bytes_loaded;
                 has_header = (loaded >= sizeof(uiox_image_hdr_t));
                 uiox_boot_printf("  loaded %lu bytes\n",
                                  (unsigned long)loaded);
             } else {
                 uiox_boot_puts("  kernel image not found on volume\n");
             }
         } else {
             uiox_boot_puts("  UNFS mount failed\n");
         }
     } else {
         uiox_boot_puts("  no kernel file — simulation handoff\n");
     }
 
     /* ================================================================== */
     /* Stage 5: Verify                                                    */
     /* ================================================================== */
     BOOT_LOG(5, "Verify");
 
     if (has_header) {
         const uiox_image_hdr_t *hdr =
             (const uiox_image_hdr_t *)load_buf;
         const uint8_t *payload =
             (const uint8_t *)load_buf + sizeof(uiox_image_hdr_t);
         size_t pay_len = loaded - sizeof(uiox_image_hdr_t);
 
         rc = uiox_boot_verify_image(hdr, payload, pay_len, UIOX_ARCH_ID);
         if (rc != UIOX_BOOT_OK)
             BOOT_FATAL("image verification failed (err=%d)", (int)rc);
 
         uint32_t magic = 0u;
         uiox_boot_memcpy(&magic, payload, 4u);
         is_elf = (magic == ELF64_MAGIC) &&
                  (pay_len >= sizeof(uiox_elf64_ehdr_t)) &&
                  (((const uiox_elf64_ehdr_t *)payload)->e_ident[4]
                       == ELF_CLASS_64);
 
         entry_pa = hdr->entry_point;
         uiox_boot_printf("  SHA-256 OK  entry=%016llx\n",
                          (unsigned long long)entry_pa);
         BOOT_OK();
     } else {
         uiox_boot_puts("  no image to verify — simulation handoff\n");
     }
 
     /* ================================================================== */
     /* Stage 6: ELF load / flat binary                                    */
     /* ================================================================== */
     BOOT_LOG(6, "ELF");
 
     if (has_header) {
         const uiox_image_hdr_t *hdr =
             (const uiox_image_hdr_t *)load_buf;
         const uint8_t *payload =
             (const uint8_t *)load_buf + sizeof(uiox_image_hdr_t);
         size_t pay_len = loaded - sizeof(uiox_image_hdr_t);
 
         if (is_elf) {
             rc = uiox_boot_elf64_load(payload, pay_len, &entry_pa);
             if (rc != UIOX_BOOT_OK)
                 BOOT_FATAL("ELF64 load failed (err=%d)", (int)rc);
             uiox_boot_printf("  ELF64 loaded  entry=%016llx\n",
                              (unsigned long long)entry_pa);
         } else {
             rc = uiox_boot_flat_load(payload, pay_len,
                                       (uintptr_t)hdr->load_addr);
             if (rc != UIOX_BOOT_OK)
                 BOOT_FATAL("flat binary load failed (err=%d)", (int)rc);
             entry_pa = hdr->entry_point;
             uiox_boot_printf("  flat binary  entry=%016llx\n",
                              (unsigned long long)entry_pa);
         }
     } else {
         entry_pa = UIOX_KERN_LOAD_PA;
         uiox_boot_printf("  simulation entry=%016llx\n",
                          (unsigned long long)entry_pa);
     }
     BOOT_OK();
 
     /* ================================================================== */
     /* Stage 7: Handoff — never returns                                   */
     /* ================================================================== */
     BOOT_LOG(7, "Handoff");
 
     uint64_t final_dtb = (dtb_pa != 0u) ? dtb_pa : UIOX_DTB_FALLBACK;
 
     uiox_boot_handoff(entry_pa,
                       final_dtb,
                       (uint64_t)UIOX_ARGS_PA,
                       &mem_map,
                       cmdline);
 
     for (;;)
         __asm__ volatile("" ::: "memory");
 }
 