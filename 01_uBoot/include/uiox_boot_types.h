/**
 * @file  uiox_boot_types.h
 * @brief UIOX Bootloader — base integer types, magic numbers, error codes.
 *
 * This is the only header that may be included by every other bootloader
 * module without risk of circular dependencies.
 *
 * @version 1.0.0  (1.0.0 + RISC-V RV64GC additions — 2026-07-12)
 * @date    2026-07-12
 */

 #ifndef UIOX_BOOT_TYPES_H
 #define UIOX_BOOT_TYPES_H
 
 /* =========================================================================
  * Portable integer types (no libc in boot environment)
  * ====================================================================== */
 
 typedef unsigned char       uint8_t;
 typedef unsigned short      uint16_t;
 typedef unsigned int        uint32_t;
 typedef unsigned long long  uint64_t;
 typedef signed char         int8_t;
 typedef signed short        int16_t;
 typedef signed int          int32_t;
 typedef signed long long    int64_t;
 
 #if defined(__aarch64__) || defined(__x86_64__)
 typedef uint64_t uintptr_t;
 typedef int64_t  intptr_t;
 typedef uint64_t size_t;
 #else
 typedef uint32_t uintptr_t;
 typedef int32_t  intptr_t;
 typedef uint32_t size_t;
 #endif
 
 typedef _Bool bool;
 #define true  1
 #define false 0
 #define NULL  ((void *)0)
 
 /* =========================================================================
  * Architecture identifier
  * ====================================================================== */
 
 typedef enum {
     UIOX_ARCH_ARM64  = 0,
     UIOX_ARCH_ARM32  = 1,
     UIOX_ARCH_X86_64 = 2,
     UIOX_ARCH_RV64   = 3, 
 } uiox_arch_t;
 
 /* =========================================================================
  * Error codes
  * ====================================================================== */
 
 typedef enum {
     UIOX_BOOT_OK            =  0,
     UIOX_BOOT_ERR_GENERIC   = -1,
     UIOX_BOOT_ERR_NOMEM     = -2,
     UIOX_BOOT_ERR_NOTFOUND  = -3,
     UIOX_BOOT_ERR_IO        = -4,
     UIOX_BOOT_ERR_BADMAGIC  = -5,
     UIOX_BOOT_ERR_BADCSUM   = -6,
     UIOX_BOOT_ERR_OVERFLOW  = -7,
     UIOX_BOOT_ERR_INVAL     = -8,
     UIOX_BOOT_ERR_UNSUP     = -9,
 } uiox_boot_err_t;
 
 /* =========================================================================
  * Magic numbers
  * ====================================================================== */
 
 #define UIOX_IMAGE_MAGIC        0x55494F58u  /**< "UIOX"                  */
 #define UIOX_BOOT_ARGS_MAGIC    0x55415247u  /**< "UARG"                  */
 #define UIOX_DTB_MAGIC          0xD00DFEEDu /**< FDT / DTB               */
 #define UIOX_MULTIBOOT2_MAGIC   0xE85250D6u  /**< Multiboot2 header magic */
 #define UIOX_MULTIBOOT2_LOADER  0x36D76289u  /**< Bootloader magic        */
 
 /* =========================================================================
  * UIOX kernel image header (prepended to kernel binary)
  * ====================================================================== */
 
 #define UIOX_IMAGE_HDR_VERSION  1u
 #define UIOX_IMAGE_CMDLINE_MAX  256u
 
 typedef struct {
     uint32_t magic;          /**< UIOX_IMAGE_MAGIC                        */
     uint32_t version;        /**< Header version (1)                      */
     uint32_t arch;           /**< uiox_arch_t                             */
     uint32_t hdr_size;       /**< sizeof(uiox_image_hdr_t) padded to 64   */
     uint64_t load_addr;      /**< Physical load address                   */
     uint64_t entry_point;    /**< Kernel entry point (physical)           */
     uint64_t image_size;     /**< Total image size (bytes, excl. header)  */
     uint8_t  sha256[32];     /**< SHA-256 of image bytes (excl. header)   */
     char     cmdline[UIOX_IMAGE_CMDLINE_MAX]; /**< Default kernel cmdline  */
     uint8_t  _pad[60];       /**< Padding to 512-byte alignment           */
 } uiox_image_hdr_t;
 /* =========================================================================
 * Boot arguments passed to the kernel
 * ====================================================================== */
//typedef struct {
//    uint32_t magic;          /**< UIOX_BOOT_ARGS_MAGIC                    */
//    uint32_t version;
//    uint64_t dtb_pa;         /**< DTB physical address                    */
//    uint64_t initrd_start;
//    uint64_t initrd_size;
//    char     cmdline[UIOX_IMAGE_CMDLINE_MAX];
//} uiox_boot_args_t;


/* =========================================================================
 * Arch-specific MMIO base addresses (boot-time constants)
 *
 * ARM64  — QEMU virt (aarch64)
 *   PL011 UART @ 0x09000000  (QEMU virt serial0)
 *   GIC-v2     @ 0x08000000 / 0x08010000
 *
 * ARM32  — QEMU versatilepb
 *   PL011 UART @ 0x101F1000
 *   SP804 Timer @ 0x101E2000
 *
 * x86-64 — QEMU q35
 *   COM1 16550A @ I/O 0x3F8
 *   PIT 8254    @ I/O 0x40
 *
 * RISC-V — QEMU virt (riscv64)                               <<< NEW >>>
 *   NS16550A UART    @ 0x10000000  (DTB: uart0 node)
 *   CLINT mtime      @ 0x0200BFF8  (DTB: clint node; cpu_hw: clint_base)
 *   PLIC             @ 0x0C000000  (DTB: plic node; cpu_hw: gic_base)
 *   Test Finisher    @ 0x00100000  (QEMU virt reset/exit)
 * ====================================================================== */

/* ARM64 */
#define UIOX_PL011_BASE_ARM64   0x09000000UL /**< QEMU virt PL011        */
#define UIOX_GIC_DIST_BASE      0x08000000UL
#define UIOX_GIC_CPU_BASE       0x08010000UL

/* ARM32 */
#define UIOX_PL011_BASE_ARM32   0x101F1000UL /**< versatilepb PL011      */
#define UIOX_SP804_BASE_ARM32   0x101E2000UL /**< versatilepb SP804      */

/* x86-64 */
#define UIOX_COM1_PORT          0x3F8u       /**< COM1 I/O port base     */
#define UIOX_PIT_PORT           0x40u        /**< PIT 8254 I/O port base */

/* RISC-V RV64GC                                               <<< NEW >>> */
#define UIOX_RV_UART_BASE       0x10000000UL /**< NS16550A UART (QEMU)   */
#define UIOX_RV_CLINT_BASE      0x02000000UL /**< CLINT (cpu_hw clint_base) */
#define UIOX_RV_CLINT_MTIME     (UIOX_RV_CLINT_BASE + 0xBFF8UL)
#define UIOX_RV_CLINT_MTIMECMP0 (UIOX_RV_CLINT_BASE + 0x4000UL)
#define UIOX_RV_PLIC_BASE       0x0C000000UL /**< PLIC (cpu_hw gic_base)  */
#define UIOX_RV_TEST_BASE       0x00100000UL /**< QEMU test finisher      */

/* PL011 register offsets (ARM64 + ARM32 shared) */
#define PL011_DR                0x000u
#define PL011_FR                0x018u
#define PL011_IBRD              0x024u
#define PL011_FBRD              0x028u
#define PL011_LCR_H             0x02Cu
#define PL011_CR                0x030u
#define PL011_FR_TXFF           (1u << 5)
#define PL011_LCR_WLEN8         (3u << 5)
#define PL011_LCR_FEN           (1u << 4)
#define PL011_CR_UARTEN         (1u << 0)
#define PL011_CR_TXE            (1u << 8)
#define PL011_CR_RXE            (1u << 9)

/* x86-64 COM1 register offsets */
#define COM1_THR                0u
#define COM1_LSR                5u
#define COM1_LSR_THRE           (1u << 5)
#define COM1_IER                1u
#define COM1_FCR                2u
#define COM1_LCR                3u
#define COM1_MCR                4u
#define COM1_DLL                0u
#define COM1_DLM                1u

 /* =========================================================================
  * Utility macros
  * ====================================================================== */
 
 #define UIOX_ALIGN_UP(v, a)     (((v) + ((a) - 1u)) & ~((a) - 1u))
 #define UIOX_ALIGN_DN(v, a)     ((v) & ~((a) - 1u))
 #define UIOX_ARRAY_SIZE(a)      (sizeof(a) / sizeof((a)[0]))
 #define UIOX_UNUSED(x)          ((void)(x))
 #define UIOX_MIN(a, b)          ((a) < (b) ? (a) : (b))
 #define UIOX_MAX(a, b)          ((a) > (b) ? (a) : (b))
 
 #endif /* UIOX_BOOT_TYPES_H */
 