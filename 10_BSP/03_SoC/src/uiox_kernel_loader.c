/**
 * @file    uiox_kernel_loader.c
 * @brief   UIOX BSP — Dynamic kernel loader implementation.
 */

 #include "../include/uiox_kernel_loader.h"
 #include "../include/uiox_soc_map.h"
 #include "../include/uiox_soc_hw.h"
 #include "../include/uiox_soc_stdio.h"
 #include "../include/uiox_soc_secboot.h"
 
 /* ── Bare-metal helpers ──────────────────────────────────────────── */
 static void *bm_memcpy(void *dst, const void *src, uiox_size_t n)
 {
     uiox_uint8_t *d = (uiox_uint8_t *)dst;
     const uiox_uint8_t *s = (const uiox_uint8_t *)src;
     while (n--) *d++ = *s++;
     return dst;
 }
 
 static void bm_memset(void *dst, int v, uiox_size_t n)
 {
     uiox_uint8_t *d = (uiox_uint8_t *)dst;
     while (n--) *d++ = (uiox_uint8_t)v;
 }
 
 /* ── CRC32 (IEEE 802.3) ──────────────────────────────────────────── */
 static uiox_uint32_t crc32_compute(const uiox_uint8_t *data,
                                     uiox_size_t         len)
 {
     uiox_uint32_t crc = 0xFFFFFFFFu;
     while (len--) {
         crc ^= *data++;
         for (int b = 0; b < 8; b++) {
             if (crc & 1u) crc = (crc >> 1u) ^ 0xEDB88320u;
             else          crc >>= 1u;
         }
     }
     return crc ^ 0xFFFFFFFFu;
 }
 
 /* =========================================================================
  * Platform-default kernel locations
  *
  * These are compile-time defaults.  A Boot Configuration Block (BCB)
  * written to a reserved flash sector can override them at runtime.
  *
  * Defaults match QEMU virt machine conventions:
  *   ARM64  : kernel at DRAM+4MB, entry = load_addr
  *   ARM32  : kernel at DRAM+1MB
  *   x86_64 : kernel at 1MB physical (conventional x86 load address)
  *   RISC-V : kernel at DRAM+2MB
  * ====================================================================== */
 static void loader_platform_defaults(uiox_kernel_desc_t *desc)
 {
     bm_memset(desc, 0, sizeof(*desc));
 
 #if defined(__aarch64__) || defined(ARCH_ARM64)
     desc->src_flash_base = 0x40400000UL;   /* flash/eMMC after BSP image */
     desc->src_max_size   = 0x02000000UL;   /* 32 MB max kernel           */
     desc->load_addr      = 0x40400000UL;   /* load at DRAM+4MB           */
     desc->entry_addr     = 0x40400000UL;
     desc->dtb_addr       = 0x42000000UL;   /* DTB at DRAM+32MB           */
 
 #elif defined(__arm__) || defined(ARCH_ARM32)
     desc->src_flash_base = 0x60100000UL;
     desc->src_max_size   = 0x00F00000UL;   /* 15 MB max                  */
     desc->load_addr      = 0x60100000UL;
     desc->entry_addr     = 0x60100000UL;
     desc->dtb_addr       = 0x60F00000UL;
 
 #elif defined(__x86_64__) || defined(ARCH_X86_64)
     desc->src_flash_base = 0x00100000UL;   /* 1 MB — conventional        */
     desc->src_max_size   = 0x04000000UL;   /* 64 MB max                  */
     desc->load_addr      = 0x00100000UL;
     desc->entry_addr     = 0x00100000UL;
     desc->dtb_addr       = 0x00000000UL;   /* x86 uses ACPI, no DTB      */
 
 #elif defined(__riscv) || defined(ARCH_RISCV64)
     desc->src_flash_base = 0x80200000UL;
     desc->src_max_size   = 0x02000000UL;
     desc->load_addr      = 0x80200000UL;
     desc->entry_addr     = 0x80200000UL;
     desc->dtb_addr       = 0x82200000UL;
 #endif
 
     desc->verify_crc = UIOX_TRUE;
     desc->verify_sig = UIOX_FALSE;   /* set true when secboot active   */
 }
 
 /* =========================================================================
  * uiox_kernel_loader_init
  * ====================================================================== */
 uiox_soc_err_t uiox_kernel_loader_init(uiox_kernel_desc_t *desc)
 {
     if (!desc) return UIOX_SOC_ERR_INVAL;
     loader_platform_defaults(desc);
     printf("[kloader] init: load=0x%lx entry=0x%lx dtb=0x%lx\n",
            (unsigned long)desc->load_addr,
            (unsigned long)desc->entry_addr,
            (unsigned long)desc->dtb_addr);
     return UIOX_SOC_OK;
 }
 
 /* =========================================================================
  * uiox_kernel_load
  *
  * Step 1: read image from flash/eMMC into DRAM at src_flash_base.
  * Step 2: check for UIF magic header.
  *         If present → parse header, extract entry/dtb addresses.
  *         If absent  → treat as raw binary, use defaults from desc.
  * ====================================================================== */
 uiox_soc_err_t uiox_kernel_load(uiox_kernel_desc_t *desc)
 {
     if (!desc) return UIOX_SOC_ERR_INVAL;
 
     uiox_uint8_t *dst = (uiox_uint8_t *)(uiox_uintptr_t)desc->load_addr;
     const uiox_uint8_t *src =
         (const uiox_uint8_t *)(uiox_uintptr_t)desc->src_flash_base;
 
     printf("[kloader] reading up to %lu bytes from 0x%lx → 0x%lx\n",
            (unsigned long)desc->src_max_size,
            (unsigned long)desc->src_flash_base,
            (unsigned long)desc->load_addr);
 
     /*
      * On real hardware: call the storage driver to DMA/copy
      * the kernel image from flash/eMMC into DRAM.
      *
      * Here we do a direct memory copy — works for:
      *   • XIP (execute-in-place) NOR flash mapped to the address space
      *   • QEMU where flash is memory-mapped
      *   • RAM-resident images (testing)
      *
      * For eMMC/SD: replace this with uiox_soc_stor_read() calls.
      */
     bm_memcpy(dst, src, desc->src_max_size);
     desc->loaded_bytes = desc->src_max_size;
 
     /* Check for UIF header */
     uif_header_t *hdr = (uif_header_t *)dst;
     if (hdr->magic == UIF_MAGIC && hdr->version == UIF_VERSION) {
         printf("[kloader] UIF image detected: '%s'\n", hdr->name);
         printf("[kloader]   payload=%u bytes  comp=%u  arch=%u\n",
                hdr->payload_size, hdr->compression, hdr->arch);
 
         /*
          * Override entry and DTB addresses from the UIF header.
          * The payload starts immediately after the header.
          */
         desc->entry_addr =
             ((uiox_uint64_t)hdr->entry_addr_hi << 32u) |
              hdr->entry_addr_lo;
         desc->loaded_bytes = hdr->payload_size;
 
         /*
          * If compressed: decompress payload in-place or to a
          * separate buffer.  Stub: only NONE compression supported now.
          */
         if (hdr->compression != UIF_COMP_NONE) {
             printf("[kloader] ERROR: compression %u not supported\n",
                    hdr->compression);
             return UIOX_SOC_ERR_UNSUP;
         }
 
         /* Move payload to load address (skip header) */
         uiox_size_t hdr_sz = hdr->header_size;
         bm_memcpy(dst, dst + hdr_sz, hdr->payload_size);
 
     } else {
         printf("[kloader] raw binary image (no UIF header)\n");
         /* entry_addr and dtb_addr already set from defaults */
     }
 
     desc->loaded = UIOX_TRUE;
     printf("[kloader] loaded %lu bytes → entry=0x%lx\n",
            (unsigned long)desc->loaded_bytes,
            (unsigned long)desc->entry_addr);
     return UIOX_SOC_OK;
 }
 
 /* =========================================================================
  * uiox_kernel_verify
  * ====================================================================== */
 uiox_soc_err_t uiox_kernel_verify(uiox_kernel_desc_t *desc)
 {
     if (!desc || !desc->loaded) return UIOX_SOC_ERR_INVAL;
 
     /* CRC32 check */
     if (desc->verify_crc) {
         const uiox_uint8_t *data =
             (const uiox_uint8_t *)(uiox_uintptr_t)desc->load_addr;
         uiox_uint32_t crc = crc32_compute(data, desc->loaded_bytes);
         printf("[kloader] CRC32 = 0x%08x\n", (unsigned)crc);
         /* On a production build: compare against expected CRC stored
          * in the UIF header or a separate manifest.               */
     }
 
     /* Signature verification (secboot) */
     if (desc->verify_sig) {
         printf("[kloader] signature verification (stub)\n");
         /*
          * Real path:
          *   uiox_soc_secboot_verify_image(&secboot_ctx,
          *       (uif_header_t*)desc->load_addr, payload, len, &report);
          */
     }
 
     printf("[kloader] verification OK\n");
     return UIOX_SOC_OK;
 }
 
 /* =========================================================================
  * uiox_kernel_jump
  *
  * Transfer control to the kernel entry point.
  * Architecture-specific calling convention:
  *   ARM64  : x0 = dtb_pa, x1=x2=x3=0 (Linux/UIOX ABI)
  *   ARM32  : r0=0, r1=~0 (machine type), r2=dtb_pa (Linux ABI)
  *   x86_64 : rdi = dtb_pa (System V ABI — first argument)
  *   RISC-V : a0 = hartid=0, a1 = dtb_pa (Linux/UIOX ABI)
  * ====================================================================== */
 void __attribute__((noreturn))
 uiox_kernel_jump(const uiox_kernel_desc_t *desc,
                   uiox_uint64_t             dtb_pa)
 {
     printf("[kloader] jumping to kernel @ 0x%lx  dtb=0x%llx\n",
            (unsigned long)desc->entry_addr,
            (unsigned long long)dtb_pa);
 
     /* Sync barriers before jumping */
     uiox_soc_hw_dsb();
     uiox_soc_hw_isb();
 
 #if defined(__aarch64__) || defined(ARCH_ARM64)
     /*
      * ARM64 kernel ABI:
      *   x0 = physical address of FDT (DTB)
      *   x1 = 0 (reserved)
      *   x2 = 0 (reserved)
      *   x3 = 0 (reserved)
      */
     typedef void __attribute__((noreturn))
         (*kernel_entry_arm64_t)(uiox_uint64_t dtb,
                                  uiox_uint64_t r1,
                                  uiox_uint64_t r2,
                                  uiox_uint64_t r3);
 
     kernel_entry_arm64_t entry =
         (kernel_entry_arm64_t)(uiox_uintptr_t)desc->entry_addr;
     entry(dtb_pa, 0ULL, 0ULL, 0ULL);
 
 #elif defined(__arm__) || defined(ARCH_ARM32)
     /*
      * ARM32 Linux ABI:
      *   r0 = 0
      *   r1 = machine type (~0 = DT-only boot)
      *   r2 = physical address of ATAGs / DTB
      */
     typedef void __attribute__((noreturn))
         (*kernel_entry_arm32_t)(uiox_uint32_t r0,
                                  uiox_uint32_t mach_type,
                                  uiox_uint32_t dtb);
 
     kernel_entry_arm32_t entry =
         (kernel_entry_arm32_t)(uiox_uintptr_t)desc->entry_addr;
     entry(0u, ~0u, (uiox_uint32_t)dtb_pa);
 
 #elif defined(__x86_64__) || defined(ARCH_X86_64)
     /*
      * x86_64: jump to multiboot2 or direct entry.
      * Pass dtb_pa in rdi (System V first argument).
      */
     typedef void __attribute__((noreturn))
         (*kernel_entry_x86_t)(uiox_uint64_t dtb);
 
     kernel_entry_x86_t entry =
         (kernel_entry_x86_t)(uiox_uintptr_t)desc->entry_addr;
     entry(dtb_pa);
 
 #elif defined(__riscv) || defined(ARCH_RISCV64)
     /*
      * RISC-V Linux ABI:
      *   a0 = hartid (0 for boot hart)
      *   a1 = physical address of DTB
      */
     typedef void __attribute__((noreturn))
         (*kernel_entry_rv64_t)(uiox_uint64_t hartid,
                                 uiox_uint64_t dtb);
 
     kernel_entry_rv64_t entry =
         (kernel_entry_rv64_t)(uiox_uintptr_t)desc->entry_addr;
     entry(0ULL, dtb_pa);
 #endif
 
     /* Should never reach here */
     for (;;) uiox_soc_hw_barrier();
 }
 