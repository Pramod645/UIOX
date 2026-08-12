ARM64 (AArch64) — kernel_arm64.ld
Region	Physical Address	Size	Notes
BSP / DTB / boot args	0x4000_0000	2 MB	uiox_bsp_main, device tree blob, boot args struct. x0=dtb_pa x1=args_pa on entry
.text.entry	0x4020_0000	~64 KB	uiox_kernel_main must be first symbol — BSP jumps here directly
.text	0x4020_8000	~8 MB	All kernel code + all subsystem libs (libdev, libfs, libpcs, libhw, libbcache) linked in
.rodata	0x40A0_8000	~2 MB	Read-only data, string tables, const arrays. 8-byte aligned
.data	0x40C0_8000	~1 MB	Initialised globals, driver state. Copied from image by BSP
.bss	0x40D0_8000	~1 MB	Zero-init globals. Zeroed by bss_zero() at kernel entry. 8-byte aligned
.stack	0x40E0_8000	64 KB	Grows downward. _stack_top at end. 16-byte aligned (AArch64 ABI)
Free DRAM / page pool	0x40E0_C000	~49 MB	Managed by 33_PCS/02_MemMngnt physical page allocator

MMIO (all below DRAM base 0x4000_0000):

0x0200_0000 — CLINT (SMP timer), 64 KB
0x0800_0000 — GIC-400 Distributor
0x0801_0000 — GIC-400 CPU Interface
0x080A_0000 — GICv3 Redistributors
0x0808_0000 — GICv3 ITS
0x0900_0000 — PL011 UART0 (IRQ 33)
0x0904_0000 — PL011 UART1
0x0905_0000 — SMMU (ARM IOMMU)



ARM32 (ARMv7-A) — kernel_arm32.ld
Region	Physical Address	Size	Notes
BSP / DTB / boot args	0x6000_0000	2 MB	r2=dtb_pa r3=args_pa. Supports ATAGs and FDT
.text.entry	0x6020_0000	~64 KB	uiox_kernel_main. -mfloat-abi=soft, no VFP
.text	0x6020_8000	~8 MB	All subsystem libs. -ffreestanding, no libc
.rodata	0x60A0_8000	~1.5 MB	4-byte aligned
.data	0x60B8_8000	~768 KB	4-byte aligned
.bss	0x60C0_A000	~768 KB	4-byte aligned
.stack	0x60C8_C000	32 KB	Conservative for arm32. 8-byte aligned (ARM32 ABI)
Free DRAM / page pool	0x60C9_0000	~50 MB	33_PCS/02_MemMngnt

MMIO (all below DRAM base 0x6000_0000):

0x1000_9000 — UART0 PL011
0x1004_0000 — GIC Distributor
0x1004_1000 — GIC CPU Interface
0x1001_1000 — Timer SP804
0x1001_7000 — RTC PL031
0x0A00_0000 — VirtIO devices (IRQ 48, stride 0x200)


RISC-V 64 (RV64IMAFDC) — kernel_riscv64.ld
Region	Physical Address	Size	Notes
OpenSBI (M-mode)	0x8000_0000	2 MB reserved	OpenSBI firmware runs in M-mode. Kernel enters S-mode via SBI ecalls. Never overwritten
.text.entry	0x8020_0000	~64 KB	Load base. a0=dtb_pa a1=args_pa. medany code model
.text	0x8020_8000	~8 MB	-march=rv64imafdc_zicsr_zifencei. No libc
.rodata	0x80A0_8000	~2 MB	8-byte aligned
.data	0x80C0_8000	~1 MB	
.bss	0x80D0_8000	~1 MB	8-byte aligned
.stack	0x80E0_8000	64 KB	16-byte aligned. sp ← _stack_top
Free DRAM / page pool	0x80E0_C000	~47 MB	33_PCS/02_MemMngnt

MMIO (all below DRAM base 0x8000_0000):

0x0200_0000 — CLINT, 64 KB (mtime/mtimecmp)
0x0C00_0000 — PLIC, 64 MB (max IRQ 127)
0x0C00_2000 — PLIC enable, ctx×0x80
0x0C20_0000 — PLIC threshold/claim, ctx×0x1000
0x1000_0000 — NS16550A UART (IRQ 10)
0x1000_1000 — VirtIO ×8 slots, stride 0x1000 (IRQ 1)

x86-64 (AMD64) — kernel_x86_64.ld
Region	Physical Address	Size	Notes
Real-mode / legacy area	0x0000_0000	1 MB	IVT 0x0, BDA 0x400, EBDA ~0x9FC00, VGA 0xA0000, ROM BIOS 0xF0000. Kernel never touches this
BSP / DTB / boot args	0x0010_0000	1 MB	rdi=args_pa rsi=dtb_pa (SysV ABI). ACPI tables here
.text.entry	0x0020_0000	~64 KB	2 MB conventional kernel load base. -mcmodel=kernel -mno-sse -mno-sse2 -mno-red-zone
.text	0x0020_8000	~8 MB	-fno-pic. No libc. No SSE registers (kernel constraint)
.rodata	0x00A0_8000	~2 MB	16-byte aligned
.data	0x00C0_8000	~1 MB	16-byte aligned
.bss	0x00D0_8000	~1 MB	16-byte aligned
.stack	0x00E0_8000	64 KB	16-byte aligned. rsp ← _stack_top. No red-zone enforced
Free RAM / page pool	0x00E0_C000	~49 MB	33_PCS/02_MemMngnt

MMIO / I/O (legacy I/O ports + high MMIO):

I/O port 0x3F8 — COM1 / UART (early console)
I/O port 0x2F8 — COM2
I/O port 0x0060 — PS/2 keyboard/mouse
0xFEC0_0000 — I/O APIC
0xFED0_0000 — HPET (IRQ 8)
0xFEE0_0000 — Local APIC (per-core)
~0xB000_0000 — PCIe ECAM (Q35)
0xFFFF_0000 — BIOS ROM shadow



Key differences across architectures
	            ARM64	                ARM32	            RISC-V 64	            x86-64
DRAM base	    0x4000_0000	            0x6000_0000	        0x8000_0000	            0x0000_0000
Kernel load	    +2 MB	                +2 MB	            +2 MB (after SBI)	    2 MB (fixed)
Stack size	    64 KB	                32 KB	            64 KB	                64 KB
Stack align	    16 bytes	            8 bytes	            16 bytes	            16 bytes
Section align	8 bytes	                4 bytes	            8 bytes	                16 bytes
Interrupt ctrl	GIC-400/v3	            GIC	                PLIC	                LAPIC + IOAPIC
Timer	        Generic timer (ISA)	    SP804 MMIO	        CLINT mtime	            HPET
Special	        SMMU	                VirtIO	            OpenSBI 2 MB reserved	1 MB legacy hole
Float	        hardware	            -mfloat-abi=soft	hardware	            -mno-sse/-mno-sse2

=========================================================================================

┌─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│              UIOX — Kernel Physical Memory Layout  (addresses increase downward, source: linker scripts + uiox_soc_map.h)│
└─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘

        ARM64 (AArch64)              ARM32 (ARMv7-A)            RISC-V 64 (RV64)             x86-64 (AMD64)
       ─────────────────            ─────────────────          ─────────────────────         ─────────────────
       DRAM: 0x4000_0000            DRAM: 0x6000_0000          DRAM: 0x8000_0000             DRAM: 0x0000_0000
       Kernel: 0x4020_0000          Kernel: 0x6020_0000        Kernel: 0x8020_0000           Kernel: 0x0020_0000
       Stack: 64 KB / 16B           Stack: 32 KB / 8B          Stack: 64 KB / 16B            Stack: 64 KB / 16B
       ─────────────────            ─────────────────          ─────────────────────         ─────────────────


MMIO   ╔═══════════════╗            ╔═══════════════╗          ╔═══════════════════╗         ╔═══════════════╗
(below │▓ GIC-400 Dist ▓│           │▓ UART PL011   ▓│         │▓ CLINT 64 KB      ▓│        │▓ Legacy area  ▓│
 DRAM  │▓ 0x0800_0000  ▓│           │▓ 0x1000_9000  ▓│         │▓ 0x0200_0000      ▓│        │▓ IVT/BDA/VGA  ▓│
 base) │▓ GIC CPU IF   ▓│           │▓ GIC Dist     ▓│         │▓ PLIC 64 MB       ▓│        │▓ 0x0000_0000  ▓│
       │▓ 0x0801_0000  ▓│           │▓ 0x1004_0000  ▓│         │▓ 0x0C00_0000      ▓│        │▓ – 0x000F_FFFF▓│
       │▓ UART PL011   ▓│           │▓ Timer SP804  ▓│         │▓ NS16550 UART     ▓│        ╚═══════════════╝
       │▓ 0x0900_0000  ▓│           │▓ 0x1001_1000  ▓│         │▓ 0x1000_0000      ▓│
       │▓ SMMU IOMMU   ▓│           │▓ RTC PL031    ▓│         │▓ VirtIO ×8 slots  ▓│
       │▓ 0x0905_0000  ▓│           │▓ 0x1001_7000  ▓│         │▓ 0x1000_1000      ▓│
       │▓ CLINT 64 KB  ▓│           │▓ VirtIO       ▓│         ╚═══════════════════╝
       │▓ 0x0200_0000  ▓│           │▓ 0x0A00_0000  ▓│
       ╚═══════════════╝            ╚═══════════════╝


0x4000_0000 ┌───────────────┐  0x6000_0000 ┌───────────────┐  0x8000_0000 ┌───────────────────┐  0x0010_0000 ┌───────────────┐
            │  BSP / DTB    │              │  BSP / DTB    │              │  OpenSBI (M-mode) │              │  BSP / DTB    │
            │  boot args    │              │  ATAGs / FDT  │              │  2 MB reserved    │              │  ACPI tables  │
            │  2 MB         │              │  2 MB         │              │  SBI ecalls ↕ S   │              │  1 MB         │
            │               │              │               │              │                   │              │  rdi=args_pa  │
0x4020_0000 ├───────────────┤  0x6020_0000 ├───────────────┤  0x8020_0000 ├───────────────────┤  0x0020_0000 ├───────────────┤
            │ .text.entry   │              │ .text.entry   │              │ .text.entry       │              │ .text.entry   │
            │ uiox_kernel   │              │ uiox_kernel   │              │ uiox_kernel_main  │              │ uiox_kernel   │
            │ _main         │              │ _main         │              │ S-mode · medany   │              │ _main         │
            │ x0=dtb_pa     │              │ r2=dtb_pa     │              │ a0=dtb_pa         │              │ rdi=args_pa   │
            │ x1=args_pa    │              │ r3=args_pa    │              │ a1=args_pa        │              │ rsi=dtb_pa    │
0x4020_8000 ├───────────────┤  0x6020_8000 ├───────────────┤  0x8020_8000 ├───────────────────┤  0x0020_8000 ├───────────────┤
            │               │              │               │              │                   │              │               │
            │   .text       │              │   .text       │              │   .text           │              │   .text       │
            │               │              │               │              │                   │              │               │
            │  all kernel   │              │  all kernel   │              │  all kernel       │              │  all kernel   │
            │  + subsystem  │              │  + subsystem  │              │  + subsystem      │              │  + subsystem  │
            │  libs         │              │  libs         │              │  libs             │              │  libs         │
            │  ~8 MB        │              │  ~8 MB        │              │  ~8 MB            │              │  ~8 MB        │
            │               │              │  -mfloat-abi  │              │  rv64imafdc       │              │  -mcmodel=    │
            │               │              │  =soft        │              │  _zicsr_zifencei  │              │  kernel       │
            │               │              │  no VFP/NEON  │              │                   │              │  -mno-sse/sse2│
            │               │              │               │              │                   │              │  -mno-red-zone│
0x40A0_8000 ├───────────────┤  0x60A0_8000 ├───────────────┤  0x80A0_8000 ├───────────────────┤  0x00A0_8000 ├───────────────┤
            │  .rodata      │              │  .rodata      │              │  .rodata          │              │  .rodata      │
            │  ~2 MB        │              │  ~1.5 MB      │              │  ~2 MB            │              │  ~2 MB        │
            │  8-byte align │              │  4-byte align │              │  8-byte align     │              │  16-byte align│
0x40C0_8000 ├───────────────┤  0x60B8_8000 ├───────────────┤  0x80C0_8000 ├───────────────────┤  0x00C0_8000 ├───────────────┤
            │  .data        │              │  .data        │              │  .data            │              │  .data        │
            │  ~1 MB        │              │  ~768 KB      │              │  ~1 MB            │              │  ~1 MB        │
            │  init globals │              │  init globals │              │  init globals     │              │  init globals │
0x40D0_8000 ├───────────────┤  0x60C0_A000 ├───────────────┤  0x80D0_8000 ├───────────────────┤  0x00D0_8000 ├───────────────┤
            │  .bss         │              │  .bss         │              │  .bss             │              │  .bss         │
            │  ~1 MB        │              │  ~768 KB      │              │  ~1 MB            │              │  ~1 MB        │
            │  zero-init    │              │  zero-init    │              │  zero-init        │              │  zero-init    │
            │  by bss_zero()│              │  by bss_zero()│              │  by bss_zero()    │              │  by bss_zero()│
            │  8-byte align │              │  4-byte align │              │  8-byte align     │              │  16-byte align│
0x40E0_8000 ├───────────────┤  0x60C8_C000 ├───────────────┤  0x80E0_8000 ├───────────────────┤  0x00E0_8000 ├───────────────┤
            │  .stack       │              │  .stack       │              │  .stack           │              │  .stack       │
            │  64 KB        │              │  32 KB        │              │  64 KB            │              │  64 KB        │
            │  ↓ grows down │              │  ↓ grows down │              │  ↓ grows down     │              │  ↓ grows down │
            │  16-byte align│              │  8-byte align │              │  16-byte align    │              │  16-byte align│
            │  _stack_top ← │              │  _stack_top ← │              │  sp ← _stack_top  │              │  rsp←_stk_top │
0x40E0_C000 ├───────────────┤  0x60C9_0000 ├───────────────┤  0x80E0_C000 ├───────────────────┤  0x00E0_C000 ├───────────────┤
            │               │              │               │              │                   │              │               │
            │               │              │               │              │                   │              │               │
            │  Free DRAM    │              │  Free DRAM    │              │  Free DRAM        │              │  Free RAM     │
            │               │              │               │              │                   │              │               │
            │  Page pool    │              │  Page pool    │              │  Page pool        │              │  Page pool    │
            │               │              │               │              │                   │              │               │
            │  ~49 MB       │              │  ~50 MB       │              │  ~47 MB           │              │  ~49 MB       │
            │               │              │               │              │                   │              │               │
            │  managed by   │              │  managed by   │              │  managed by       │              │  managed by   │
            │  33_PCS/      │              │  33_PCS/      │              │  33_PCS/          │              │  33_PCS/      │
            │  02_MemMngnt  │              │  02_MemMngnt  │              │  02_MemMngnt      │              │  02_MemMngnt  │
            │               │              │               │              │                   │              │  no red-zone  │
            │               │              │               │              │                   │              │  enforced     │
            └───────────────┘              └───────────────┘              └───────────────────┘              └───────────────┘
         (end of 64 MB window)          (end of 64 MB window)          (end of 64 MB window)           (end of 64 MB window)


MMIO above DRAM:                                                                                x86-64 high MMIO:
  (ARM64/ARM32/RISCV: all MMIO sits                                                               0xFEC0_0000  I/O APIC
   below DRAM base — separate region)                                                             0xFED0_0000  HPET  (IRQ 8)
                                                                                                  0xFEE0_0000  Local APIC
                                                                                                  ~0xB000_0000 PCIe ECAM (Q35)
                                                                                                  port 0x3F8   COM1 UART
                                                                                                  port 0x2F8   COM2


─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
KEY DIFFERENCES
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
                      ARM64             ARM32             RISC-V 64         x86-64
                      ──────────────    ──────────────    ──────────────    ──────────────────────
DRAM base             0x4000_0000       0x6000_0000       0x8000_0000       0x0000_0000
Kernel load           0x4020_0000       0x6020_0000       0x8020_0000       0x0020_0000
Stack size            64 KB             32 KB ◄smallest   64 KB             64 KB
Stack alignment       16 bytes          8 bytes           16 bytes          16 bytes
Section alignment     8 bytes           4 bytes           8 bytes           16 bytes ◄largest
Float                 hardware FPU      -mfloat-abi=soft  hardware FPU      -mno-sse/-mno-sse2 ◄no SSE
Code model            default           default           medany            -mcmodel=kernel
Special below kernel  nothing           nothing           OpenSBI 2 MB ◄    nothing
Special above 1 MB    nothing           nothing           nothing           1 MB legacy hole ◄
IRQ controller        GIC-400 / GICv3   GIC               PLIC              LAPIC + IOAPIC
Timer source          Generic timer     SP804 MMIO        CLINT mtime       HPET
Entry registers       x0=dtb x1=args    r2=dtb r3=args    a0=dtb a1=args    rdi=args rsi=dtb
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
┌─────────────────────────────────────────────────────────────────────────┐
│     UIOX — x86-64 (AMD64) Kernel Physical Memory Layout                │
│     Source: 30_KIX/linker/kernel_x86_64.ld  +  uiox_soc_map.h          │
│     Kernel load: 0x0020_0000  |  Stack: 64 KB  |  -mno-sse -mno-red-zone│
└─────────────────────────────────────────────────────────────────────────┘

  Physical        Size      Contents
  Address                   
  ─────────────────────────────────────────────────────────────────────────

  0x0000_0000  ┌───────────────────────────────────────────────────────┐
               │  REAL-MODE LEGACY AREA  (1 MB — kernel never touches) │
               │                                                        │
  0x0000_0000  │  IVT — Interrupt Vector Table (256 × 4 bytes)         │
  0x0000_0400  │  BDA — BIOS Data Area                                  │
  0x0000_0500  │  Conventional memory (free in real mode)               │
  0x0009_FC00  │  EBDA — Extended BIOS Data Area (~1–4 KB)              │
  0x000A_0000  │  VGA framebuffer / video memory  (128 KB)              │
  0x000C_0000  │  VGA BIOS ROM                                          │
  0x000F_0000  │  System BIOS ROM shadow                                │
               │                                                        │
  0x000F_FFFF  └───────────────────────────────────────────────────────┘
                 1 MB

  0x0010_0000  ┌───────────────────────────────────────────────────────┐
               │  BSP / DTB / BOOT ARGS  (1 MB)                        │
               │                                                        │
               │  • uiox_bsp_main output                                │
               │  • Device Tree Blob (DTB / FDT)                        │
               │  • ACPI tables (RSDP, RSDT, DSDT)                      │
               │  • uiox_boot_args_t struct                              │
               │                                                        │
               │  Entry convention (SysV ABI):                          │
               │    rdi = args_pa  (pointer to uiox_boot_args_t)        │
               │    rsi = dtb_pa   (physical address of DTB)            │
               │                                                        │
  0x001F_FFFF  └───────────────────────────────────────────────────────┘
                 1 MB

  0x0020_0000  ┌───────────────────────────────────────────────────────┐  ← KERNEL_LOAD_ADDR
               │  .text.entry  — uiox_kernel_main()               ~64KB│
               │                                                        │
               │  • FIRST symbol in image — BSP jumps here directly     │
               │  • Compiler flags:                                      │
               │      -mcmodel=kernel   (code within ±2 GB of kernel)   │
               │      -mno-sse          (no XMM registers in kernel)     │
               │      -mno-sse2         (no SSE2 — return vals in rax)   │
               │      -mno-mmx          (no MMX)                         │
               │      -mno-red-zone     (no 128-byte red-zone below rsp) │
               │      -fno-pic          (position-dependent code)        │
               │      -ffreestanding    (no libc, no runtime startup)    │
               │                                                        │
  0x0020_8000  ├───────────────────────────────────────────────────────┤
               │                                                        │
               │  .text  — all kernel code + subsystem libs      ~8 MB  │
               │                                                        │
               │  Linked in order:                                       │
               │    uiox_kernel_main.o                                   │
               │    libhw.a       (20_DriverInterfaces)                  │
               │    libdev.a      (30_DeviceDrivers)                     │
               │    libbcache.a   (31_BufferCache)                       │
               │    libfs.a       (32_FS  incl. journal + netfs)         │
               │    libpcs.a      (33_PCS incl. ksign/fboot/sec/kpatch)  │
               │    libcas.a      (34_CAS)                               │
               │    libuioxfwx86.a (02_FwHal)                            │
               │    libbsp.a      (10_BSP — static mode only)            │
               │                                                        │
  0x00A0_8000  ├───────────────────────────────────────────────────────┤
               │  .rodata  — read-only data                       ~2 MB  │
               │                                                        │
               │  • String literals, const arrays                        │
               │  • Exception/fault message tables                       │
               │  • Syscall dispatch table (40_SCI)                      │
               │  • IRQ handler table                                    │
               │  • 16-byte aligned                                      │
               │                                                        │
  0x00C0_8000  ├───────────────────────────────────────────────────────┤
               │  .data  — initialised globals                    ~1 MB  │
               │                                                        │
               │  • Driver state structures                              │
               │  • SoC descriptor (uiox_soc_desc_t)                     │
               │  • Clock context, DMA context                           │
               │  • 16-byte aligned                                      │
               │                                                        │
  0x00D0_8000  ├───────────────────────────────────────────────────────┤
               │  .bss  — zero-initialised globals                ~1 MB  │
               │                                                        │
               │  • Zeroed by bss_zero() at kernel entry                 │
               │    (before any C code runs)                             │
               │  • Process table, page descriptors                      │
               │  • Ring buffers, buffer pools                           │
               │  • _bss_start / _bss_end linker symbols                 │
               │  • 16-byte aligned                                      │
               │                                                        │
  0x00E0_8000  ├───────────────────────────────────────────────────────┤
               │  .stack  — kernel stack                          64 KB  │
               │                                                        │
               │  • Grows DOWNWARD from _stack_top                       │
               │  • rsp ← _stack_top at entry (stack_setup())           │
               │  • 16-byte aligned  (SysV x86-64 ABI)                  │
               │  • -mno-red-zone: no 128-byte scratch below rsp         │
               │    (needed because IRQ handlers use the same stack)     │
               │  • _stack_top = _bss_end + 64 KB (linker symbol)        │
               │                                                        │
  0x00E0_C000  ├───────────────────────────────────────────────────────┤
               │                                                        │
               │                                                        │
               │  FREE RAM  —  physical page pool               ~49 MB  │
               │                                                        │
               │  Managed by: 33_PCS/02_MemMngnt                         │
               │  • physalloc_page() / physfree_page()                   │
               │  • Bump allocator seeded from uiox_soc_mem_init()       │
               │  • Pages mapped by TTBR0 equivalent: CR3 (x86-64)       │
               │  • ASLR: uiox_aslr_randomise_mm() randomises            │
               │    stack/heap/mmap/vDSO base on every exec()            │
               │                                                        │
               │                                                        │
  0x03FF_FFFF  └───────────────────────────────────────────────────────┘
                 64 MB window end


  ─────────────────────────────────────────────────────────────────────────
  MMIO / I/O REGIONS  (x86-64 — separate from normal RAM)
  ─────────────────────────────────────────────────────────────────────────

  I/O Port Space  (accessed via IN/OUT instructions — not MMIO)
  ─────────────────────────────────────────────────────────────
  port 0x0020     PIC1 (legacy, not used by UIOX — APIC takes over)
  port 0x00A0     PIC2 (legacy)
  port 0x0040     PIT  Channel 0 (legacy timer)
  port 0x0060     PS/2 keyboard controller
  port 0x0064     PS/2 mouse controller
  port 0x02F8     COM2 / UART (secondary serial)
  port 0x03F8     COM1 / UART — early console (early_puts uses this)
                    early_putc() polls bit 5 of 0x3FD (LSR)
                    writes char to 0x3F8 (THR)
  port 0x0CF8     PCI config address
  port 0x0CFC     PCI config data

  MMIO Space  (memory-mapped, accessed via normal load/store)
  ─────────────────────────────────────────────────────────────
  ~0xB000_0000    PCIe ECAM  (Q35 chipset — config space)
  0xFEC0_0000     I/O APIC   (interrupt routing for all devices)
  0xFED0_0000     HPET       (High Precision Event Timer, IRQ 8)
  0xFEE0_0000     Local APIC (per-core — timer, IPI, EOI)
                    uiox_soc_irq_ack() writes EOI here after each IRQ
  0xFFFF_0000     BIOS ROM shadow (legacy, not accessed by kernel)


  ─────────────────────────────────────────────────────────────────────────
  CALL SEQUENCE AT KERNEL ENTRY  (x86-64 specific)
  ─────────────────────────────────────────────────────────────────────────

  BSP / bootloader
    → loads kernel ELF to 0x0020_0000
    → sets  rdi = args_pa  (uiox_boot_args_t *)
            rsi = dtb_pa   (DTB physical address)
    → JMP  0x0020_0000   ← uiox_kernel_main()

  uiox_kernel_main(args_pa_ptr, dtb_pa_val)  [SysV: rdi, rsi]
    → stack_setup()    movq _stack_top, %rsp
    → bss_zero()       clear _bss_start .. _bss_end
    → early_puts()     polls COM1 port 0x3FD / writes 0x3F8
    → kernel_common_init()
        → arch_init()           GDT, IDT, LAPIC, HPET, IOAPIC
        → uiox_soc_init()       Q35 SoC detect, PCIe, DMA, PSCI stub
        → uiox_ks_boot_entry()  33_PCS/03_ksign
        → uiox_fb_*()           33_PCS/04_fboot
        → uiox_proc_init()      33_PCS scheduler + sec + kpatch
        → uiox_shell_start()    50_UIX/01_shell


  ─────────────────────────────────────────────────────────────────────────
  KEY x86-64 CONSTRAINTS vs. OTHER ARCHITECTURES
  ─────────────────────────────────────────────────────────────────────────

  -mno-sse / -mno-sse2    XMM registers banned in kernel — float return
                           values must use x87/integer only; uioxthermsensor.c
                           was converted to Q16.16 fixed-point for this reason

  -mno-red-zone           IRQ handlers share the kernel stack — the 128-byte
                           ABI red-zone below rsp would be clobbered on entry

  -mcmodel=kernel          All code + data within 2 GB of load address —
                           allows RIP-relative addressing without PLT

  -fno-pic                Position-dependent code — no GOT/PLT overhead

  1 MB legacy hole        0x0000_0000–0x000F_FFFF never used by kernel;
                           unlike ARM/RISC-V where MMIO is in a clean
                           separate region, x86 mixes legacy BIOS, VGA,
                           and ROM in low physical memory

  I/O port space          x86 has a separate 64 KB I/O address space
                           (IN/OUT instructions) — COM1 UART is here,
                           not MMIO like PL011 (ARM) or NS16550 (RISC-V)
  ─────────────────────────────────────────────────────────────────────────
┌─────────────────────────────────────────────────────────────────────────┐
│     UIOX — ARM64 (AArch64) Kernel Physical Memory Layout               │
│     Source: 30_KIX/linker/kernel_arm64.ld  +  uiox_soc_map.h           │
│     Kernel load: 0x4020_0000  |  Stack: 64 KB  |  16-byte aligned       │
└─────────────────────────────────────────────────────────────────────────┘

  Physical        Size      Contents
  Address
  ─────────────────────────────────────────────────────────────────────────

  MMIO REGION  (entirely below DRAM base 0x4000_0000)
  ─────────────────────────────────────────────────────────────────────────

  0x0200_0000  ┌───────────────────────────────────────────────────────┐
               │  CLINT  — SMP timer / IPI                       64 KB  │
               │  mtime / mtimecmp registers                            │
  0x0201_0000  └───────────────────────────────────────────────────────┘

  0x0800_0000  ┌───────────────────────────────────────────────────────┐
               │  GIC-400  Distributor  (GICD_*)                        │
               │  GICD_CTLR    +0x000   enable distributor              │
               │  GICD_TYPER   +0x004   IRQ count                       │
               │  GICD_ISENABLER0 +0x100  enable bits                   │
               │  GICD_IPRIORITYR0 +0x400  priority regs                │
               │  GICD_ITARGETSR0  +0x800  CPU targets                  │
  0x0801_0000  ├───────────────────────────────────────────────────────┤
               │  GIC-400  CPU Interface  (GICC_*)                      │
               │  GICC_CTLR  +0x000   enable CPU IF                     │
               │  GICC_PMR   +0x004   priority mask (0xFF = all)        │
               │  GICC_IAR   +0x00C   interrupt acknowledge             │
               │  GICC_EOIR  +0x010   end of interrupt                  │
  0x080A_0000  ├───────────────────────────────────────────────────────┤
               │  GICv3  Redistributors  (one per core)                 │
  0x0808_0000  ├───────────────────────────────────────────────────────┤
               │  GICv3  ITS  (Interrupt Translation Service)           │
  0x080F_FFFF  └───────────────────────────────────────────────────────┘

  0x0900_0000  ┌───────────────────────────────────────────────────────┐
               │  PL011 UART0   IRQ 33                                  │
               │  UARTFR   +0x018  flag register  (bit 5 = TX full)     │
               │  UARTDR   +0x000  data register                        │
  0x0904_0000  ├───────────────────────────────────────────────────────┤
               │  PL011 UART1                                           │
  0x0905_0000  ├───────────────────────────────────────────────────────┤
               │  SMMU  (ARM IOMMU — System Memory Management Unit)     │
  0x0905_FFFF  └───────────────────────────────────────────────────────┘

  ─────────────────────────────────────────────────────────────────────────
  DRAM  (from 0x4000_0000)
  ─────────────────────────────────────────────────────────────────────────

  0x4000_0000  ┌───────────────────────────────────────────────────────┐  ← DRAM base
               │  BSP / DTB / BOOT ARGS                          2 MB   │
               │                                                        │
               │  • uiox_bsp_main  (10_BSP — static mode)               │
               │  • Device Tree Blob (DTB / FDT)                        │
               │  • uiox_boot_args_t struct                              │
               │                                                        │
               │  Entry convention:                                      │
               │    x0 = dtb_pa   (physical address of DTB)             │
               │    x1 = args_pa  (pointer to uiox_boot_args_t)         │
               │    x2 = x3 = 0   (reserved)                            │
               │                                                        │
  0x401F_FFFF  └───────────────────────────────────────────────────────┘
                 2 MB

  0x4020_0000  ┌───────────────────────────────────────────────────────┐  ← KERNEL_LOAD_ADDR
               │  .text.entry  — uiox_kernel_main()              ~64 KB │
               │                                                        │
               │  • FIRST symbol — BSP jumps here after DTB load        │
               │  • Reads x0 (dtb_pa) and x1 (args_pa) from registers   │
               │  • Calls stack_setup()  → MSR SP, MSR SP_EL0           │
               │  • Calls bss_zero()     → clears BSS                   │
               │  • Compiler flags:                                      │
               │      -march=armv8-a                                     │
               │      -mabi=lp64         (64-bit pointers/longs)         │
               │      -mlittle-endian                                    │
               │      -ffreestanding     (no libc)                       │
               │      -fno-stack-protector                               │
               │                                                        │
  0x4020_8000  ├───────────────────────────────────────────────────────┤
               │                                                        │
               │  .text  — all kernel code + subsystem libs      ~8 MB  │
               │                                                        │
               │  Linked in order:                                       │
               │    uiox_kernel_main.o                                   │
               │    libhw.a       (20_DriverInterfaces)                  │
               │    libdev.a      (30_DeviceDrivers)                     │
               │    libbcache.a   (31_BufferCache)                       │
               │    libfs.a       (32_FS — journal + netfs)              │
               │    libpcs.a      (33_PCS — ksign/fboot/sec/kpatch)      │
               │    libcas.a      (34_CAS)                               │
               │    libuioxfwarm64.a  (02_FwHal)                         │
               │    libbsp.a      (10_BSP — static mode only)            │
               │                                                        │
  0x40A0_8000  ├───────────────────────────────────────────────────────┤
               │  .rodata  — read-only data                       ~2 MB  │
               │                                                        │
               │  • String literals, const arrays                        │
               │  • Vector table (_vector_table — VBAR_EL1 target)       │
               │  • Syscall dispatch table (40_SCI)                      │
               │  • Exception syndrome tables                            │
               │  • 8-byte aligned                                       │
               │                                                        │
  0x40C0_8000  ├───────────────────────────────────────────────────────┤
               │  .data  — initialised globals                    ~1 MB  │
               │                                                        │
               │  • Driver state, SoC descriptor                         │
               │  • GIC context, clock context                           │
               │  • 8-byte aligned                                       │
               │                                                        │
  0x40D0_8000  ├───────────────────────────────────────────────────────┤
               │  .bss  — zero-initialised globals                ~1 MB  │
               │                                                        │
               │  • Zeroed by bss_zero() before any C code runs          │
               │  • Process table, page pool descriptors                 │
               │  • IRQ handler dispatch table                           │
               │  • _bss_start / _bss_end linker symbols                 │
               │  • 8-byte aligned                                       │
               │                                                        │
  0x40E0_8000  ├───────────────────────────────────────────────────────┤
               │  .stack  — kernel stack                          64 KB  │
               │                                                        │
               │  • Grows DOWNWARD from _stack_top                       │
               │  • stack_setup():                                       │
               │      MOV  sp,  _stack_top & ~0xF   (16-byte align)      │
               │      MSR  SP_EL0, xzr              (clear user SP)      │
               │  • 16-byte aligned per AArch64 ABI                      │
               │  • _stack_top linker symbol at top of .stack section    │
               │                                                        │
  0x40E0_C000  ├───────────────────────────────────────────────────────┤
               │                                                        │
               │                                                        │
               │  FREE DRAM  —  physical page pool               ~49 MB  │
               │                                                        │
               │  Managed by: 33_PCS/02_MemMngnt                         │
               │  • physalloc_page() / physfree_page()                   │
               │  • Bump allocator seeded by uiox_soc_mem_init()         │
               │  • DDR region descriptors from uiox_soc_map.h           │
               │  • TTBR0_EL1 swapped per process by scheduler           │
               │  • ASLR: uiox_aslr_randomise_mm() randomises            │
               │    stack/heap/mmap/vDSO base on every exec()            │
               │  • TLB flushed via TLBI ASIDE1IS on context switch      │
               │                                                        │
               │                                                        │
  0x43FF_FFFF  └───────────────────────────────────────────────────────┘
                 64 MB window end


  ─────────────────────────────────────────────────────────────────────────
  CALL SEQUENCE AT KERNEL ENTRY  (ARM64 specific)
  ─────────────────────────────────────────────────────────────────────────

  BSP / bootloader (01_uBoot)
    → loads kernel ELF to 0x4020_0000
    → sets  x0 = dtb_pa   (physical address of DTB)
            x1 = args_pa  (physical address of uiox_boot_args_t)
            x2 = x3 = 0   (reserved, must be zero)
    → BR    x0 → jumps to 0x4020_0000

  uiox_kernel_main()                [reads x0, x1 from registers]
    → stack_setup()    MOV sp, _stack_top & ~0xF
                       MSR SP_EL0, xzr
    → bss_zero()       clears _bss_start .. _bss_end
    → early_puts()     polls PL011 UARTFR bit5 / writes UARTDR
    → kernel_common_init()
        → arch_init()           GIC, VBAR_EL1, generic timer, cache
        → uiox_soc_init()       SoC detect, clock, DMA, PSCI, TZ
        → uiox_ks_boot_entry()  33_PCS/03_ksign — verify + PCR
        → uiox_fb_*()           33_PCS/04_fboot — timing milestones
        → uiox_proc_init()      33_PCS — scheduler + sec + kpatch
        → uiox_shell_start()    50_UIX/01_shell


  ─────────────────────────────────────────────────────────────────────────
  KEY ARM64 CONSTRAINTS
  ─────────────────────────────────────────────────────────────────────────

  -march=armv8-a        Full ARMv8-A ISA — hardware FPU/NEON available
                         but kernel code should avoid NEON (no save/restore
                         of V0–V31 on IRQ entry unless explicitly enabled)

  -mabi=lp64            64-bit longs and pointers — uintptr_t = uint64_t

  VBAR_EL1              Exception vector base — installed by arch_init()
                         all IRQ/FIQ/SVC/abort entries go through this table

  TTBR0_EL1             User page table base — swapped on every
                         context switch by arch_context_switch()
                         TLBI ASIDE1IS flushes old process TLB entries

  SP_EL0                User-mode stack pointer — zeroed at entry,
                         set per-thread by the scheduler

  DAIF                  Interrupt mask — cleared by arch_init() step 8
                         (MSR DAIFCLR, #2) to unmask IRQs globally

  SMMU                  ARM IOMMU at 0x0905_0000 — isolates DMA from
                         device buffers; used by WiFi/USB/eMMC DMA paths
  ─────────────────────────────────────────────────────────────────────────
┌─────────────────────────────────────────────────────────────────────────┐
│     UIOX — ARM32 (ARMv7-A) Kernel Physical Memory Layout               │
│     Source: 30_KIX/linker/kernel_arm32.ld  +  uiox_soc_map.h           │
│     Kernel load: 0x6020_0000  |  Stack: 32 KB  |  8-byte aligned        │
└─────────────────────────────────────────────────────────────────────────┘

  Physical        Size      Contents
  Address
  ─────────────────────────────────────────────────────────────────────────

  MMIO REGION  (entirely below DRAM base 0x6000_0000)
  ─────────────────────────────────────────────────────────────────────────

  0x0A00_0000  ┌───────────────────────────────────────────────────────┐
               │  VirtIO devices   IRQ 48   stride 0x200               │
               │  Block, net, console, etc.                             │
  0x0A00_03FF  └───────────────────────────────────────────────────────┘

  0x1000_0000  ┌───────────────────────────────────────────────────────┐
               │  VersatilePB / QEMU virt peripheral block              │
               │                                                        │
  0x1000_9000  │  PL011 UART0   — early console                        │
               │    UARTFR   +0x018  flag reg  (bit 5 = TX full)        │
               │    UARTDR   +0x000  data reg                           │
               │                                                        │
  0x1001_1000  │  Timer SP804   — dual 32-bit countdown timer           │
               │    TIMER0LOAD  +0x000                                  │
               │    TIMER0CTRL  +0x008  (enable, periodic, IRQ)         │
               │                                                        │
  0x1001_7000  │  RTC PL031    — real-time clock                        │
               │    RTCDR  +0x000  data register (seconds)              │
               │                                                        │
  0x1004_0000  │  GIC Distributor  (GICD_*)                             │
               │    GICD_CTLR      +0x000  enable distributor           │
               │    GICD_ISENABLER0 +0x100  enable bits                 │
               │    GICD_IPRIORITYR0 +0x400  priority regs              │
               │                                                        │
  0x1004_1000  │  GIC CPU Interface  (GICC_*)                           │
               │    GICC_CTLR  +0x000  enable                           │
               │    GICC_PMR   +0x004  priority mask                    │
               │    GICC_IAR   +0x00C  interrupt acknowledge            │
               │    GICC_EOIR  +0x010  end of interrupt                 │
               │                                                        │
  0x1FFF_FFFF  └───────────────────────────────────────────────────────┘

  ─────────────────────────────────────────────────────────────────────────
  DRAM  (from 0x6000_0000)
  ─────────────────────────────────────────────────────────────────────────

  0x6000_0000  ┌───────────────────────────────────────────────────────┐  ← DRAM base
               │  BSP / DTB / BOOT ARGS                          2 MB   │
               │                                                        │
               │  • uiox_bsp_main  (10_BSP — static mode)               │
               │  • Device Tree Blob (FDT) or ATAGlist                  │
               │  • uiox_boot_args_t struct                              │
               │                                                        │
               │  Entry convention (ARM Linux boot protocol):            │
               │    r0 = 0         (machine type — unused with DTB)      │
               │    r1 = 0         (reserved)                            │
               │    r2 = dtb_pa    (physical address of DTB / ATAGs)     │
               │    r3 = args_pa   (physical address of uiox_boot_args_t)│
               │                                                        │
  0x601F_FFFF  └───────────────────────────────────────────────────────┘
                 2 MB

  0x6020_0000  ┌───────────────────────────────────────────────────────┐  ← KERNEL_LOAD_ADDR
               │  .text.entry  — uiox_kernel_main()              ~64 KB │
               │                                                        │
               │  • FIRST symbol — BSP jumps here (BX or MOV PC)        │
               │  • Reads r2 (dtb_pa), r3 (args_pa) from registers      │
               │  • Calls stack_setup() → MOV sp, _stack_top & ~7       │
               │  • Calls bss_zero()    → clears BSS                    │
               │  • Compiler flags:                                      │
               │      -march=armv7-a                                     │
               │      -marm             (ARM instruction set, not Thumb) │
               │      -mfloat-abi=soft  (no VFP/NEON — soft-float ABI)  │
               │      -mlittle-endian                                    │
               │      -ffreestanding    (no libc)                        │
               │      -fno-stack-protector                               │
               │                                                        │
               │  ◄ NOTE: -mfloat-abi=soft means:                        │
               │    • All float args passed in integer registers (r0–r3) │
               │    • No VFP/NEON instructions generated                 │
               │    • uintptr_t = uint32_t  (32-bit pointers)            │
               │    • All pointer/int casts via memcpy to avoid          │
               │      -Werror=pointer-to-int-cast on arm32               │
               │                                                        │
  0x6020_8000  ├───────────────────────────────────────────────────────┤
               │                                                        │
               │  .text  — all kernel code + subsystem libs      ~8 MB  │
               │                                                        │
               │  Linked in order:                                       │
               │    uiox_kernel_main.o                                   │
               │    libhw.a       (20_DriverInterfaces)                  │
               │    libdev.a      (30_DeviceDrivers)                     │
               │    libbcache.a   (31_BufferCache)                       │
               │    libfs.a       (32_FS — journal + netfs)              │
               │    libpcs.a      (33_PCS — ksign/fboot/sec/kpatch)      │
               │    libcas.a      (34_CAS)                               │
               │    libuioxfwarm32.a  (02_FwHal)                         │
               │    libbsp.a      (10_BSP — static mode only)            │
               │                                                        │
  0x60A0_8000  ├───────────────────────────────────────────────────────┤
               │  .rodata  — read-only data                      ~1.5 MB │
               │                                                        │
               │  • String literals, const arrays                        │
               │  • Vector table (at 0x0000_0000 or 0xFFFF_0000 — VBAR) │
               │  • Syscall dispatch table                               │
               │  • Exception mode tables                                │
               │  • 4-byte aligned                                       │
               │                                                        │
  0x60B8_8000  ├───────────────────────────────────────────────────────┤
               │  .data  — initialised globals                   ~768 KB │
               │                                                        │
               │  • Driver state, SoC descriptor                         │
               │  • GIC context, timer context                           │
               │  • 4-byte aligned                                       │
               │                                                        │
  0x60C0_A000  ├───────────────────────────────────────────────────────┤
               │  .bss  — zero-initialised globals               ~768 KB │
               │                                                        │
               │  • Zeroed by bss_zero() at kernel entry                 │
               │  • Process table, page pool descriptors                 │
               │  • _bss_start / _bss_end linker symbols                 │
               │  • 4-byte aligned                                       │
               │                                                        │
  0x60C8_C000  ├───────────────────────────────────────────────────────┤
               │  .stack  — kernel stack                          32 KB  │
               │                                                        │
               │  • Grows DOWNWARD from _stack_top                       │
               │  • stack_setup():                                       │
               │      MOV  sp, _stack_top & ~7   (8-byte align)          │
               │  • 8-byte aligned per ARM32 ABI (AAPCS)                 │
               │  • Smallest stack of all four arches — conservative     │
               │    for arm32 with limited register width                │
               │  • _stack_top linker symbol at top of .stack section    │
               │                                                        │
  0x60C9_0000  ├───────────────────────────────────────────────────────┤
               │                                                        │
               │                                                        │
               │  FREE DRAM  —  physical page pool               ~50 MB  │
               │                                                        │
               │  Managed by: 33_PCS/02_MemMngnt                         │
               │  • physalloc_page() / physfree_page()                   │
               │  • Bump allocator seeded by uiox_soc_mem_init()         │
               │  • TTBR0 swapped per process by scheduler               │
               │  • Max physical address 32-bit (4 GB) — no LPAE         │
               │  • Page size 4 KB  (UIOX_PAGE_SIZE = 4096)              │
               │  • ASLR limited to 32-bit address space                 │
               │                                                        │
               │                                                        │
  0x63FF_FFFF  └───────────────────────────────────────────────────────┘
                 64 MB window end


  ─────────────────────────────────────────────────────────────────────────
  CALL SEQUENCE AT KERNEL ENTRY  (ARM32 specific)
  ─────────────────────────────────────────────────────────────────────────

  BSP / bootloader (01_uBoot)
    → loads kernel ELF to 0x6020_0000
    → sets  r0 = 0        (machine type — unused with DTB)
            r1 = 0        (reserved)
            r2 = dtb_pa   (physical address of DTB)
            r3 = args_pa  (physical address of uiox_boot_args_t)
    → MOV PC, 0x6020_0000

  uiox_kernel_main()                [reads r2, r3 from registers]
    → stack_setup()    MOV sp, _stack_top & ~7
    → bss_zero()       clears _bss_start .. _bss_end
    → early_puts()     polls PL011 UARTFR bit5 / writes UARTDR
    → kernel_common_init()
        → arch_init()           GIC, VBAR, SP804 timer, cache
        → uiox_soc_init()       SoC detect, clock, DMA, power
        → uiox_ks_boot_entry()  33_PCS/03_ksign
        → uiox_fb_*()           33_PCS/04_fboot
        → uiox_proc_init()      33_PCS — scheduler + sec + kpatch
        → uiox_shell_start()    50_UIX/01_shell


  ─────────────────────────────────────────────────────────────────────────
  KEY ARM32 CONSTRAINTS
  ─────────────────────────────────────────────────────────────────────────

  -mfloat-abi=soft      No VFP/NEON — all float args in integer regs
                         uioxthermsensor.c uses Q16.16 fixed-point anyway
                         so this is not a limitation in practice

  uintptr_t = uint32_t  32-bit pointers — all pointer/integer casts
                         must go through memcpy to avoid
                         -Werror=pointer-to-int-cast:
                           PTRTOUINTPTR(dst, src) macro
                           UINTPTRTOPTR(dst, src) macro
                         This was a major source of build errors fixed
                         across all 31_BufferCache pool files

  Stack 32 KB           Half of all other architectures — most
                         conservative for the 32-bit register file

  4-byte section align  vs 8-byte (ARM64/RISCV) and 16-byte (x86-64)

  Max physical address  4 GB without LPAE — UIOX does not enable LPAE,
                         so physical memory is limited to 4 GB flat

  Timer: SP804          ARM Versatile dual timer MMIO at 0x1001_1000
                         unlike ARM64 (generic timer ISA registers) and
                         RISC-V (CLINT mtime) which are ISA-defined
  ─────────────────────────────────────────────────────────────────────────
┌─────────────────────────────────────────────────────────────────────────┐
│     UIOX — RISC-V 64 (RV64IMAFDC) Kernel Physical Memory Layout        │
│     Source: 30_KIX/linker/kernel_riscv64.ld  +  uiox_soc_map.h         │
│     Kernel load: 0x8020_0000  |  Stack: 64 KB  |  16-byte aligned       │
└─────────────────────────────────────────────────────────────────────────┘

  Physical        Size      Contents
  Address
  ─────────────────────────────────────────────────────────────────────────

  MMIO REGION  (entirely below DRAM base 0x8000_0000)
  ─────────────────────────────────────────────────────────────────────────

  0x0200_0000  ┌───────────────────────────────────────────────────────┐
               │  CLINT  — Core Local Interruptor               64 KB   │
               │                                                        │
               │  mtime        +0x0000_BFF8  (memory-mapped timer)      │
               │  mtimecmp[0]  +0x0000_4000  (per-hart compare)         │
               │  mtimecmp[1]  +0x0000_4008  (hart 1)                   │
               │  msip[0]      +0x0000_0000  (software IPI per hart)     │
               │                                                        │
               │  ◄ M-mode only — OpenSBI handles mtime on behalf       │
               │    of S-mode kernel via SBI_SET_TIMER ecall             │
               │                                                        │
  0x0201_0000  └───────────────────────────────────────────────────────┘

  0x0C00_0000  ┌───────────────────────────────────────────────────────┐
               │  PLIC  — Platform-Level Interrupt Controller   64 MB   │
               │                                                        │
               │  Priority regs   +0x000004 × IRQ_n  (1–127)           │
               │  Pending bits    +0x001000                             │
               │  Enable          +0x002000 + ctx×0x80                 │
               │  Threshold/Claim +0x200000 + ctx×0x1000               │
               │                                                        │
               │  Max IRQ: 127    Contexts: 2 (M + S per hart)         │
               │  uiox_soc_irq_ack() writes PLIC claim register         │
               │                                                        │
  0x0FFF_FFFF  └───────────────────────────────────────────────────────┘

  0x1000_0000  ┌───────────────────────────────────────────────────────┐
               │  NS16550A UART   IRQ 10                                │
               │                                                        │
               │  THR/RBR  +0x00  transmit/receive                      │
               │  IER      +0x01  interrupt enable                      │
               │  FCR/IIR  +0x02  FIFO control / interrupt ID           │
               │  LCR      +0x03  line control (8N1)                    │
               │  LSR      +0x05  line status  (bit5 = TX empty)        │
               │                                                        │
               │  early_putc(): polls LSR bit5, writes THR              │
               │                                                        │
  0x1000_0FFF  └───────────────────────────────────────────────────────┘

  0x1000_1000  ┌───────────────────────────────────────────────────────┐
               │  VirtIO devices   ×8 slots   IRQ 1   stride 0x1000    │
               │  Slot 0: 0x1000_1000                                   │
               │  Slot 1: 0x1000_2000  ...  Slot 7: 0x1000_8000        │
  0x1000_8FFF  └───────────────────────────────────────────────────────┘

  ─────────────────────────────────────────────────────────────────────────
  DRAM  (from 0x8000_0000)
  ─────────────────────────────────────────────────────────────────────────

  0x8000_0000  ┌───────────────────────────────────────────────────────┐  ← DRAM base
               │  OpenSBI  — M-mode firmware               2 MB RESERVED│
               │                                                        │
               │  • Runs in Machine mode (M-mode / privilege level 3)   │
               │  • Provides SBI (Supervisor Binary Interface) to kernel │
               │  • Kernel runs in Supervisor mode (S-mode / level 1)   │
               │  • NEVER overwritten — reserved for entire system life  │
               │                                                        │
               │  SBI ecalls used by UIOX kernel:                        │
               │    SBI_SET_TIMER    — set next timer interrupt          │
               │    SBI_CONSOLE_PUTCHAR — early console output           │
               │    SBI_SHUTDOWN     — system power off                  │
               │    SBI_REMOTE_FENCE_I — I-cache flush across harts      │
               │    SBI_HSM_HART_START — bring up secondary hart (SMP)   │
               │                                                        │
               │  ◄ This 2 MB reservation is UNIQUE to RISC-V among     │
               │    the four UIOX architectures                          │
               │                                                        │
  0x801F_FFFF  └───────────────────────────────────────────────────────┘
                 2 MB

  0x8020_0000  ┌───────────────────────────────────────────────────────┐  ← KERNEL_LOAD_ADDR
               │  .text.entry  — uiox_kernel_main()              ~64 KB │
               │                                                        │
               │  • FIRST symbol — OpenSBI/BSP jumps here in S-mode     │
               │  • Reads a0 (dtb_pa), a1 (args_pa) from registers      │
               │  • Calls stack_setup() → MV sp, _stack_top & ~0xF      │
               │  • Calls bss_zero()    → clears BSS                    │
               │  • Compiler flags:                                      │
               │      -march=rv64imafdc_zicsr_zifencei                   │
               │        I  = base integer ISA                            │
               │        M  = integer multiply/divide                     │
               │        A  = atomic instructions                         │
               │        F  = single-precision float                      │
               │        D  = double-precision float                      │
               │        C  = compressed instructions (16-bit)            │
               │        Zicsr    = CSR instructions (REQUIRED)           │
               │        Zifencei = instruction fence (REQUIRED)          │
               │      -mabi=lp64d  (64-bit, double FPU ABI)              │
               │      -mcmodel=medany  (code within ±2 GB of any addr)   │
               │      -ffreestanding   (no libc)                         │
               │                                                        │
  0x8020_8000  ├───────────────────────────────────────────────────────┤
               │                                                        │
               │  .text  — all kernel code + subsystem libs      ~8 MB  │
               │                                                        │
               │  Linked in order:                                       │
               │    uiox_kernel_main.o                                   │
               │    libhw.a        (20_DriverInterfaces)                 │
               │    libdev.a       (30_DeviceDrivers)                    │
               │    libbcache.a    (31_BufferCache)                      │
               │    libfs.a        (32_FS — journal + netfs)             │
               │    libpcs.a       (33_PCS — ksign/fboot/sec/kpatch)     │
               │    libcas.a       (34_CAS)                              │
               │    libuioxfwriscv64.a  (02_FwHal)                       │
               │    libbsp.a       (10_BSP — static mode only)           │
               │                                                        │
  0x80A0_8000  ├───────────────────────────────────────────────────────┤
               │  .rodata  — read-only data                       ~2 MB  │
               │                                                        │
               │  • String literals, const arrays                        │
               │  • Trap vector table (_vector_table — stvec target)     │
               │  • Syscall dispatch table (40_SCI)                      │
               │  • Exception cause tables                               │
               │  • 8-byte aligned                                       │
               │                                                        │
  0x80C0_8000  ├───────────────────────────────────────────────────────┤
               │  .data  — initialised globals                    ~1 MB  │
               │                                                        │
               │  • Driver state, SoC descriptor                         │
               │  • PLIC context, CLINT context                          │
               │  • 8-byte aligned                                       │
               │                                                        │
  0x80D0_8000  ├───────────────────────────────────────────────────────┤
               │  .bss  — zero-initialised globals                ~1 MB  │
               │                                                        │
               │  • Zeroed by bss_zero() at kernel entry                 │
               │  • Process table, page pool descriptors                 │
               │  • _bss_start / _bss_end linker symbols                 │
               │  • 8-byte aligned                                       │
               │                                                        │
  0x80E0_8000  ├───────────────────────────────────────────────────────┤
               │  .stack  — kernel stack                          64 KB  │
               │                                                        │
               │  • Grows DOWNWARD from _stack_top                       │
               │  • stack_setup():                                       │
               │      MV   sp, _stack_top & ~0xF  (16-byte align)        │
               │  • 16-byte aligned per RV64 ABI                         │
               │  • _stack_top linker symbol at top of .stack section    │
               │                                                        │
  0x80E0_C000  ├───────────────────────────────────────────────────────┤
               │                                                        │
               │                                                        │
               │  FREE DRAM  —  physical page pool               ~47 MB  │
               │                                                        │
               │  Managed by: 33_PCS/02_MemMngnt                         │
               │  • physalloc_page() / physfree_page()                   │
               │  • Bump allocator seeded by uiox_soc_mem_init()         │
               │  • satp CSR swapped per process by scheduler            │
               │    (satp = mode[63:60] | ASID[59:44] | PPN[43:0])      │
               │  • SFENCE.VMA flushes TLB on context switch             │
               │  • ASLR: uiox_aslr_randomise_mm() randomises            │
               │    stack/heap/mmap/vDSO base on every exec()            │
               │  • ~47 MB — slightly less than ARM64/x86 due to         │
               │    OpenSBI 2 MB reservation at DRAM base                │
               │                                                        │
               │                                                        │
  0x83FF_FFFF  └───────────────────────────────────────────────────────┘
                 64 MB window end


  ─────────────────────────────────────────────────────────────────────────
  CALL SEQUENCE AT KERNEL ENTRY  (RISC-V 64 specific)
  ─────────────────────────────────────────────────────────────────────────

  OpenSBI (M-mode firmware at 0x8000_0000)
    → performs M-mode hardware init
    → sets  a0 = dtb_pa   (physical address of DTB)
            a1 = args_pa  (physical address of uiox_boot_args_t)
    → MRET  → drops to S-mode → jumps to 0x8020_0000

  uiox_kernel_main()                [reads a0, a1 from registers]
    → stack_setup()    MV sp, _stack_top & ~0xF
    → bss_zero()       clears _bss_start .. _bss_end
    → early_puts()     SBI_CONSOLE_PUTCHAR ecall (or polls NS16550A)
    → kernel_common_init()
        → arch_init()           stvec, PLIC, CLINT, cache
        → uiox_soc_init()       SoC detect, clock, DMA, PSCI via SBI
        → uiox_ks_boot_entry()  33_PCS/03_ksign
        → uiox_fb_*()           33_PCS/04_fboot
        → uiox_proc_init()      33_PCS — scheduler + sec + kpatch
        → uiox_shell_start()    50_UIX/01_shell


  ─────────────────────────────────────────────────────────────────────────
  KEY RISC-V 64 CONSTRAINTS
  ─────────────────────────────────────────────────────────────────────────

  OpenSBI reservation   2 MB at DRAM base is permanently reserved —
                         no other arch has this; results in ~47 MB free
                         vs ~49 MB on ARM64/x86-64

  M-mode / S-mode split Kernel runs in S-mode only. M-mode operations
                         (timer, IPI, fence, power) go through SBI ecalls
                         to OpenSBI — cannot access CLINT directly

  Zicsr / Zifencei      These extensions are REQUIRED in the -march
                         string — they provide CSR instructions and
                         instruction-fence respectively; omitting them
                         causes build errors on newer GCC versions

  -mcmodel=medany       Code can be placed anywhere in the address space
                         (±2 GB from any address) — required for a kernel
                         at 0x8020_0000 to address data at arbitrary PAs

  satp CSR              Equivalent of ARM64 TTBR0_EL1 — holds S-mode
                         page table base; swapped on context switch
                         SFENCE.VMA clears TLB (like TLBI on ARM64)

  stvec CSR             Supervisor trap vector — equivalent of VBAR_EL1
                         on ARM64; all traps/interrupts/ecalls land here

  PLIC vs GIC           PLIC is memory-mapped (unlike GIC which is also
                         MMIO but with a fixed architecture-defined layout)
                         Max 127 external IRQs; two contexts per hart
                         (M-mode + S-mode)

  Hardware FPU          Unlike ARM32 (-mfloat-abi=soft), RISC-V 64 has
                         hardware FPU (F + D extensions) — but kernel code
                         should save/restore fp regs on context switch
  ─────────────────────────────────────────────────────────────────────────
=================================================================================
 can to user pace address i n abobe generated layout, why ig not can include that as well?


Good point — the physical layout is only half the picture. Here are all four architectures with both physical and virtual address spaces side by side:

┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│   UIOX — ARM64 (AArch64)  Physical + Virtual Memory Layout                                  │
│   Kernel load PA: 0x4020_0000  |  Kernel VA: 0xFFFF_0000_0020_0000  |  4KB pages / 48-bit  │
└─────────────────────────────────────────────────────────────────────────────────────────────┘

  PHYSICAL ADDRESS SPACE              VIRTUAL ADDRESS SPACE
  (uintptr_t — 48-bit PA)             (64-bit VA — two halves split at 0x8000_0000_0000_0000)
  ─────────────────────────────────   ─────────────────────────────────────────────────────────

                                       0x0000_0000_0000_0000
                                       ┌─────────────────────────────────────────────────────┐
                                       │  USER SPACE  (TTBR0_EL1 — per-process page table)   │
                                       │                                                      │
                                       │  0x0000_0000_0000_0000  NULL / unmapped (guard)      │
                                       │  0x0000_0000_0001_0000  .text  (executable)          │
                                       │  0x0000_xxxx_xxxx_xxxx  heap  (grows up)             │
                                       │  0x0000_xxxx_xxxx_xxxx  mmap / shared libs           │
                                       │  0x0000_7FFF_xxxx_xxxx  stack (grows down)           │
                                       │  0x0000_7FFF_FFFF_FFFF  vDSO / sigreturn trampoline  │
                                       │                                                      │
                                       │  ASLR: uiox_aslr_randomise_mm() randomises           │
                                       │    stack base, heap base, mmap base, vDSO            │
                                       │    on every exec() — 33_PCS/05_sec                  │
                                       │                                                      │
                                       │  Max user VA: 0x0000_7FFF_FFFF_FFFF  (128 TB)        │
                                       └─────────────────────────────────────────────────────┘
                                       0x0000_8000_0000_0000
                                       ┌─────────────────────────────────────────────────────┐
                                       │  CANONICAL HOLE  (hardware enforced — fault on access)│
                                       │  0x0000_8000_0000_0000 – 0xFFFF_0000_0000_0000        │
                                       │  ~128 TB unmapped — non-canonical addresses           │
                                       │  Any access here → EL1 synchronous fault             │
                                       └─────────────────────────────────────────────────────┘
                                       0xFFFF_0000_0000_0000
  0x0200_0000 ┌────────────────────┐   ┌─────────────────────────────────────────────────────┐
  CLINT 64KB  │▓ CLINT (SMP timer) │   │  KERNEL SPACE  (TTBR1_EL1 — global kernel page table)│
  0x0800_0000 │▓ GIC-400 Dist      │   │                                                      │
  0x0801_0000 │▓ GIC CPU IF        │   │  0xFFFF_0000_0000_0000  kernel image base VA          │
  0x0808_0000 │▓ GICv3 ITS         │   │                                                      │
  0x080A_0000 │▓ GICv3 Redist      │   │  0xFFFF_0000_0020_0000 ┌──────────────────────────┐ │
  0x0900_0000 │▓ PL011 UART0 IRQ33 │   │  (.text.entry)         │ .text.entry              │ │
  0x0904_0000 │▓ PL011 UART1       │   │                        │ uiox_kernel_main         │ │
  0x0905_0000 │▓ SMMU IOMMU        │   │  0xFFFF_0000_0020_8000 ├──────────────────────────┤ │
              └────────────────────┘   │                        │ .text  (all code ~8 MB)  │ │
                                       │  0xFFFF_0000_00A0_8000 ├──────────────────────────┤ │
  0x4000_0000 ┌────────────────────┐   │                        │ .rodata  (~2 MB)         │ │
              │  BSP / DTB         │   │  0xFFFF_0000_00C0_8000 ├──────────────────────────┤ │
              │  boot args  2 MB   │←──┼──PA identity mapped    │ .data   (~1 MB)          │ │
  0x4020_0000 ├────────────────────┤   │  0xFFFF_0000_00D0_8000 ├──────────────────────────┤ │
              │ .text.entry        │←──┼──PA→VA offset mapped   │ .bss    (~1 MB)          │ │
              │ ~64 KB             │   │  0xFFFF_0000_00E0_8000 ├──────────────────────────┤ │
  0x4020_8000 ├────────────────────┤   │                        │ .stack  (64 KB ↓)        │ │
              │ .text  ~8 MB       │   │  0xFFFF_0000_00E0_C000 ├──────────────────────────┤ │
              │                    │   │                        │                          │ │
  0x40A0_8000 ├────────────────────┤   │                        │  Kernel heap /           │ │
              │ .rodata  ~2 MB     │   │                        │  vmalloc area            │ │
  0x40C0_8000 ├────────────────────┤   │                        │                          │ │
              │ .data  ~1 MB       │   │  0xFFFF_0000_03FF_FFFF └──────────────────────────┘ │
  0x40D0_8000 ├────────────────────┤   │                                                      │
              │ .bss  ~1 MB        │   │  MMIO virtual mappings (ioremap equivalent):          │
  0x40E0_8000 ├────────────────────┤   │  0xFFFF_FF00_0800_0000  GIC Distributor              │
              │ .stack  64 KB ↓    │   │  0xFFFF_FF00_0801_0000  GIC CPU Interface            │
  0xFFFF_C000 ├────────────────────┤   │  0xFFFF_FF00_0900_0000  PL011 UART0                  │
              │                    │   │                                                      │
              │  Free DRAM         │   │  Max kernel VA: 0xFFFF_FFFF_FFFF_FFFF               │
              │  ~49 MB            │   └─────────────────────────────────────────────────────┘
              │  page pool         │
              └────────────────────┘
  ─────────────────────────────────────────────────────────────────────────────────────────────
  TRANSLATION:  PA ↔ VA mapping
  ─────────────────────────────────────────────────────────────────────────────────────────────
  Kernel sections   PA 0x4020_0000  →  VA 0xFFFF_0000_0020_0000   (fixed offset mapping)
  MMIO (GIC)        PA 0x0800_0000  →  VA 0xFFFF_FF00_0800_0000   (ioremap — device memory)
  MMIO (UART)       PA 0x0900_0000  →  VA 0xFFFF_FF00_0900_0000   (ioremap — device memory)
  User space        PA varies       →  VA 0x0000_xxxx_xxxx_xxxx   (TTBR0_EL1 per-process)
  TTBR0_EL1  = user page table base  (swapped on context switch)
  TTBR1_EL1  = kernel page table base (fixed for entire system lifetime)
  ─────────────────────────────────────────────────────────────────────────────────────────────


┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│   UIOX — ARM32 (ARMv7-A)  Physical + Virtual Memory Layout                                  │
│   Kernel load PA: 0x6020_0000  |  Kernel VA: 0xC020_0000  |  4KB pages / 32-bit / 3G+1G    │
└─────────────────────────────────────────────────────────────────────────────────────────────┘

  PHYSICAL ADDRESS SPACE              VIRTUAL ADDRESS SPACE
  (uint32_t — 32-bit PA, no LPAE)     (32-bit VA — classic 3G user / 1G kernel split)
  ─────────────────────────────────   ──────────────────────────────────────────────────────────

                                       0x0000_0000
                                       ┌─────────────────────────────────────────────────────┐
                                       │  USER SPACE  (TTBR0 — per-process page table)        │
                                       │                                                      │
                                       │  0x0000_0000  NULL guard / unmapped                  │
                                       │  0x0000_8000  .text  (typical ELF load base)         │
                                       │  0x????_????  heap   (grows up from .bss end)         │
                                       │  0x????_????  mmap / shared libs                     │
                                       │  0xBFFF_????  stack  (grows down from 3 GB)          │
                                       │                                                      │
                                       │  Max user VA:  0xBFFF_FFFF  (3 GB)                  │
                                       │  ASLR: 33_PCS/05_sec randomises bases                │
                                       │  Address space limited to 3 GB — no canonical hole   │
                                       └─────────────────────────────────────────────────────┘
                                       0xC000_0000   ← KERNEL/USER SPLIT
  0x0A00_0000 ┌────────────────────┐   ┌─────────────────────────────────────────────────────┐
  VirtIO IRQ48│▓ VirtIO devices    │   │  KERNEL SPACE  (TTBR1 — fixed kernel page table)     │
  0x1000_9000 │▓ UART0 PL011       │   │                                                      │
  0x1001_1000 │▓ Timer SP804       │   │  0xC000_0000 ┌──────────────────────────────────────┐│
  0x1001_7000 │▓ RTC PL031         │   │              │ BSP / DTB  (mapped at boot)          ││
  0x1004_0000 │▓ GIC Distributor   │   │  0xC020_0000 ├──────────────────────────────────────┤│
  0x1004_1000 │▓ GIC CPU Interface │   │              │ .text.entry  uiox_kernel_main        ││
              └────────────────────┘   │              │ r2=dtb_pa  r3=args_pa                ││
                                       │  0xC020_8000 ├──────────────────────────────────────┤│
  0x6000_0000 ┌────────────────────┐   │              │ .text  (all code ~8 MB)              ││
              │  BSP / DTB  2 MB   │←──┼──mapped      │ -mfloat-abi=soft  no VFP/NEON        ││
  0x6020_0000 ├────────────────────┤   │  0x60A0_8000 ├──────────────────────────────────────┤│
              │ .text.entry ~64 KB │←──┼──mapped      │ .rodata  (~1.5 MB)  4-byte align     ││
  0x6020_8000 ├────────────────────┤   │  0x60B8_8000 ├──────────────────────────────────────┤│
              │ .text  ~8 MB       │   │              │ .data   (~768 KB)   4-byte align      ││
  0x60A0_8000 ├────────────────────┤   │  0x60C0_A000 ├──────────────────────────────────────┤│
              │ .rodata  ~1.5 MB   │   │              │ .bss    (~768 KB)   4-byte align      ││
  0x60B8_8000 ├────────────────────┤   │  0x60C8_C000 ├──────────────────────────────────────┤│
              │ .data  ~768 KB     │   │              │ .stack  (32 KB ↓)  8-byte align      ││
  0x60C0_A000 ├────────────────────┤   │  0x60C9_0000 ├──────────────────────────────────────┤│
              │ .bss  ~768 KB      │   │              │                                      ││
  0x60C8_C000 ├────────────────────┤   │              │  Kernel heap / vmalloc               ││
              │ .stack  32 KB ↓    │   │              │                                      ││
  0x60C9_0000 ├────────────────────┤   │  0xEFFF_FFFF ├──────────────────────────────────────┤│
              │                    │   │              │ MMIO virtual mappings:               ││
              │  Free DRAM ~50 MB  │   │  0xF000_0000 │  0xF000_9000  UART0 PL011           ││
              │  page pool         │   │              │  0xF004_0000  GIC Distributor        ││
              │                    │   │              │  0xF004_1000  GIC CPU IF             ││
              └────────────────────┘   │  0xFFFF_FFFF └──────────────────────────────────────┘│
  0xFFFF_FFFF  (4 GB PA ceiling)       └─────────────────────────────────────────────────────┘
  ─────────────────────────────────────────────────────────────────────────────────────────────
  TRANSLATION:  PA ↔ VA mapping
  ─────────────────────────────────────────────────────────────────────────────────────────────
  Kernel sections   PA 0x6020_0000  →  VA 0xC020_0000   (PA + 0x6000_0000 offset)
  MMIO (UART)       PA 0x1000_9000  →  VA 0xF000_9000   (ioremap — device memory)
  MMIO (GIC)        PA 0x1004_0000  →  VA 0xF004_0000   (ioremap — device memory)
  User space        PA varies       →  VA 0x0000_0000 – 0xBFFF_FFFF  (TTBR0 per-process)
  TTBR0  = user page table base  (swapped on context switch — 32-bit ASID)
  TTBR1  = kernel page table base (fixed — upper 1 GB 0xC000_0000–0xFFFF_FFFF)
  Max PA = 4 GB  (no LPAE — uintptr_t is 32-bit on arm32)
  ─────────────────────────────────────────────────────────────────────────────────────────────


┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│   UIOX — RISC-V 64 (RV64IMAFDC)  Physical + Virtual Memory Layout                          │
│   Kernel load PA: 0x8020_0000  |  Kernel VA: 0xFFFF_FFE0_0020_0000  |  4KB / Sv48 / S-mode │
└─────────────────────────────────────────────────────────────────────────────────────────────┘

  PHYSICAL ADDRESS SPACE              VIRTUAL ADDRESS SPACE
  (56-bit PA max — Sv48 mode)         (48-bit VA — Sv48 two-level split)
  ─────────────────────────────────   ──────────────────────────────────────────────────────────

                                       0x0000_0000_0000_0000
                                       ┌─────────────────────────────────────────────────────┐
                                       │  USER SPACE  (satp — per-process page table)         │
                                       │                                                      │
                                       │  0x0000_0000_0000_0000  NULL / unmapped              │
                                       │  0x0000_0000_0001_0000  .text (ELF load base)        │
                                       │  0x0000_xxxx_xxxx_xxxx  heap  (grows up)             │
                                       │  0x0000_xxxx_xxxx_xxxx  mmap / shared libs           │
                                       │  0x0000_7FFF_xxxx_xxxx  stack (grows down)           │
                                       │  0x0000_7FFF_FFFF_FFFF  vDSO / signal trampoline     │
                                       │                                                      │
                                       │  Max user VA: 0x0000_7FFF_FFFF_FFFF  (128 TB)        │
                                       │  ASLR: 33_PCS/05_sec on every exec()                │
                                       └─────────────────────────────────────────────────────┘
                                       0x0000_8000_0000_0000
                                       ┌─────────────────────────────────────────────────────┐
                                       │  CANONICAL HOLE                                      │
                                       │  0x0000_8000_0000_0000 – 0xFFFF_0000_0000_0000        │
                                       │  Non-canonical — any access → instruction trap       │
                                       └─────────────────────────────────────────────────────┘
                                       0xFFFF_0000_0000_0000
  0x0200_0000 ┌────────────────────┐   ┌─────────────────────────────────────────────────────┐
  CLINT 64 KB │▓ CLINT mtime/cmp   │   │  KERNEL SPACE  (satp global — fixed kernel PTs)      │
  0x0C00_0000 │▓ PLIC  64 MB       │   │                                                      │
  0x1000_0000 │▓ NS16550A UART     │   │  0xFFFF_FFE0_0000_0000  kernel image base VA         │
  0x1000_1000 │▓ VirtIO ×8 IRQ1    │   │                                                      │
              └────────────────────┘   │  0xFFFF_FFE0_0020_0000 ┌──────────────────────────┐ │
                                       │  (.text.entry)         │ .text.entry              │ │
  0x8000_0000 ┌────────────────────┐   │                        │ uiox_kernel_main         │ │
              │  OpenSBI  2 MB     │   │                        │ S-mode entry             │ │
              │  M-mode reserved   │   │                        │ a0=dtb_pa a1=args_pa     │ │
              │  SBI ecalls ↕      │   │  0xFFFF_FFE0_0020_8000 ├──────────────────────────┤ │
  0x8020_0000 ├────────────────────┤   │                        │ .text  (~8 MB)           │ │
              │ .text.entry ~64 KB │←──┼──mapped                │ rv64imafdc_zicsr         │ │
  0x8020_8000 ├────────────────────┤   │  0xFFFF_FFE0_00A0_8000 ├──────────────────────────┤ │
              │ .text  ~8 MB       │   │                        │ .rodata  (~2 MB)         │ │
  0x80A0_8000 ├────────────────────┤   │  0xFFFF_FFE0_00C0_8000 ├──────────────────────────┤ │
              │ .rodata  ~2 MB     │   │                        │ .data   (~1 MB)          │ │
  0x80C0_8000 ├────────────────────┤   │  0xFFFF_FFE0_00D0_8000 ├──────────────────────────┤ │
              │ .data  ~1 MB       │   │                        │ .bss    (~1 MB)          │ │
  0x80D0_8000 ├────────────────────┤   │  0xFFFF_FFE0_00E0_8000 ├──────────────────────────┤ │
              │ .bss  ~1 MB        │   │                        │ .stack  (64 KB ↓)        │ │
  0x80E0_8000 ├────────────────────┤   │  0xFFFF_FFE0_00E0_C000 ├──────────────────────────┤ │
              │ .stack  64 KB ↓    │   │                        │                          │ │
  0x80E0_C000 ├────────────────────┤   │                        │  Kernel heap /           │ │
              │                    │   │                        │  vmalloc area            │ │
              │  Free DRAM ~47 MB  │   │                        │                          │ │
              │  page pool         │   │  0xFFFF_FFE0_03FF_FFFF └──────────────────────────┘ │
              │                    │   │                                                      │
              └────────────────────┘   │  MMIO virtual mappings:                             │
                                       │  0xFFFF_FFFF_0C00_0000  PLIC                        │
                                       │  0xFFFF_FFFF_1000_0000  NS16550A UART               │
                                       │  0xFFFF_FFFF_0200_0000  CLINT                       │
                                       └─────────────────────────────────────────────────────┘
  ─────────────────────────────────────────────────────────────────────────────────────────────
  TRANSLATION:  PA ↔ VA mapping
  ─────────────────────────────────────────────────────────────────────────────────────────────
  Kernel sections   PA 0x8020_0000  →  VA 0xFFFF_FFE0_0020_0000   (fixed offset mapping)
  MMIO (PLIC)       PA 0x0C00_0000  →  VA 0xFFFF_FFFF_0C00_0000   (ioremap — device memory)
  MMIO (UART)       PA 0x1000_0000  →  VA 0xFFFF_FFFF_1000_0000   (ioremap — device memory)
  User space        PA varies       →  VA 0x0000_xxxx_xxxx_xxxx   (satp per-process)
  satp (S-mode)   = mode[63:60] | ASID[59:44] | PPN[43:0]
                    mode=8 → Sv39 (3-level)  mode=9 → Sv48 (4-level)
  SFENCE.VMA      = TLB flush (equivalent of TLBI on ARM64)
  stvec CSR       = trap vector base (equivalent of VBAR_EL1 on ARM64)
  OpenSBI PA      = 0x8000_0000 — NOT mapped in kernel VA (M-mode only)
  ─────────────────────────────────────────────────────────────────────────────────────────────


┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│   UIOX — x86-64 (AMD64)  Physical + Virtual Memory Layout                                   │
│   Kernel load PA: 0x0020_0000  |  Kernel VA: 0xFFFF_8000_0020_0000  |  4KB / PML4 / 48-bit  │
└─────────────────────────────────────────────────────────────────────────────────────────────┘

  PHYSICAL ADDRESS SPACE              VIRTUAL ADDRESS SPACE
  (52-bit PA max — MAXPHYADDR)        (48-bit VA — PML4 canonical hole split)
  ─────────────────────────────────   ──────────────────────────────────────────────────────────

                                       0x0000_0000_0000_0000
                                       ┌─────────────────────────────────────────────────────┐
                                       │  USER SPACE  (CR3 — per-process page table)          │
                                       │                                                      │
                                       │  0x0000_0000_0000_0000  NULL / unmapped (guard page) │
                                       │  0x0000_0000_0040_0000  .text  (typical load base)   │
                                       │  0x0000_xxxx_xxxx_xxxx  heap  (grows up)             │
                                       │  0x0000_xxxx_xxxx_xxxx  mmap / shared libs           │
                                       │  0x0000_7FFF_xxxx_xxxx  stack (grows down)           │
                                       │  0x0000_7FFF_FFFF_F000  vDSO (signal trampoline)     │
                                       │                                                      │
                                       │  Max user VA: 0x0000_7FFF_FFFF_FFFF  (128 TB)        │
                                       │  ASLR: 33_PCS/05_sec randomises all bases            │
                                       │  -mno-red-zone enforced for all kernel code           │
                                       └─────────────────────────────────────────────────────┘
                                       0x0000_8000_0000_0000
                                       ┌─────────────────────────────────────────────────────┐
                                       │  CANONICAL HOLE  (hardware PML4 enforcement)          │
                                       │  0x0000_8000_0000_0000 – 0xFFFF_7FFF_FFFF_FFFF        │
                                       │  ~16 EB non-canonical — GP fault on any access       │
                                       └─────────────────────────────────────────────────────┘
                                       0xFFFF_8000_0000_0000
  0x0000_0000 ┌────────────────────┐   ┌─────────────────────────────────────────────────────┐
              │  Legacy area  1 MB │   │  KERNEL SPACE  (CR3 upper half — global mappings)    │
  0x0010_0000 ├────────────────────┤   │                                                      │
              │  BSP / DTB  1 MB   │←──┼──0xFFFF_8000_0010_0000  BSP / DTB mapped            │
  0x0020_0000 ├────────────────────┤   │                                                      │
              │ .text.entry ~64 KB │←──┼──0xFFFF_8000_0020_0000 ┌──────────────────────────┐ │
  0x0020_8000 ├────────────────────┤   │                        │ .text.entry              │ │
              │ .text  ~8 MB       │   │                        │ uiox_kernel_main         │ │
              │ -mcmodel=kernel    │   │                        │ SysV: rdi=args rsi=dtb   │ │
              │ -mno-sse/-mno-sse2 │   │  0xFFFF_8000_0020_8000 ├──────────────────────────┤ │
  0x00A0_8000 ├────────────────────┤   │                        │ .text  (~8 MB)           │ │
              │ .rodata  ~2 MB     │   │                        │ -mcmodel=kernel          │ │
  0x00C0_8000 ├────────────────────┤   │                        │ -mno-sse -mno-red-zone   │ │
              │ .data  ~1 MB       │   │  0xFFFF_8000_00A0_8000 ├──────────────────────────┤ │
  0x00D0_8000 ├────────────────────┤   │                        │ .rodata  (~2 MB)         │ │
              │ .bss  ~1 MB        │   │  0xFFFF_8000_00C0_8000 ├──────────────────────────┤ │
  0x00E0_8000 ├────────────────────┤   │                        │ .data   (~1 MB)          │ │
              │ .stack  64 KB ↓    │   │  0xFFFF_8000_00D0_8000 ├──────────────────────────┤ │
  0x00E0_C000 ├────────────────────┤   │                        │ .bss    (~1 MB)          │ │
              │                    │   │  0xFFFF_8000_00E0_8000 ├──────────────────────────┤ │
              │  Free RAM  ~49 MB  │   │                        │ .stack  (64 KB ↓)        │ │
              │  page pool         │   │  0xFFFF_8000_00E0_C000 ├──────────────────────────┤ │
              │  no red-zone       │   │                        │                          │ │
              │  enforced          │   │                        │  Kernel heap /           │ │
              │                    │   │                        │  vmalloc area            │ │
  0xFEC0_0000 ├────────────────────┤   │                        │                          │ │
              │▓ I/O APIC MMIO     │   │  0xFFFF_8000_03FF_FFFF └──────────────────────────┘ │
  0xFED0_0000 │▓ HPET   IRQ 8      │   │                                                      │
  0xFEE0_0000 │▓ Local APIC        │   │  MMIO virtual mappings:                             │
  0xFFFF_FFFF └────────────────────┘   │  0xFFFF_C000_FEC0_0000  I/O APIC                    │
  (4 GB low — high PA above 4 GB       │  0xFFFF_C000_FED0_0000  HPET                        │
   for additional RAM if present)       │  0xFFFF_C000_FEE0_0000  Local APIC                  │
                                       │                                                      │
                                       │  I/O PORT SPACE (IN/OUT — not VA mapped):            │
                                       │  port 0x3F8  COM1 UART  (early_puts)                │
                                       │  port 0x3FD  COM1 LSR   (TX poll)                   │
                                       │  port 0x2F8  COM2 UART                               │
                                       └─────────────────────────────────────────────────────┘
  ─────────────────────────────────────────────────────────────────────────────────────────────
  TRANSLATION:  PA ↔ VA mapping
  ─────────────────────────────────────────────────────────────────────────────────────────────
  Kernel sections   PA 0x0020_0000  →  VA 0xFFFF_8000_0020_0000   (PA + 0xFFFF_8000_0000_0000)
  MMIO (IOAPIC)     PA 0xFEC0_0000  →  VA 0xFFFF_C000_FEC0_0000   (ioremap — device memory)
  MMIO (LAPIC)      PA 0xFEE0_0000  →  VA 0xFFFF_C000_FEE0_0000   (ioremap — device memory)
  User space        PA varies       →  VA 0x0000_xxxx_xxxx_xxxx   (CR3 per-process)
  CR3             = physical address of PML4 page table
                    swapped on every context switch by scheduler
  CR4.SMEP        = prevents kernel from executing user-mode pages
  CR4.SMAP        = prevents kernel from accessing user-mode pages
  PCID            = Process-Context Identifier (TLB tag — like ASID on ARM)
  ─────────────────────────────────────────────────────────────────────────────────────────────


┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│   COMPARISON — All Four Architectures                                                        │
└─────────────────────────────────────────────────────────────────────────────────────────────┘

                        ARM64              ARM32              RISC-V 64          x86-64
                        ──────────────     ──────────────     ──────────────     ──────────────
VA width                48-bit             32-bit             48-bit (Sv48)      48-bit (PML4)
PA width                48-bit             32-bit (no LPAE)   56-bit             52-bit
User VA top             0x0000_7FFF_FFFF   0xBFFF_FFFF        0x0000_7FFF_FFFF   0x0000_7FFF_FFFF
Kernel VA base          0xFFFF_0000_0000   0xC000_0000        0xFFFF_FFE0_0000   0xFFFF_8000_0000
Canonical hole          yes (hardware)     no (32-bit)        yes (hardware)     yes (hardware)
Kernel/user split       TTBR0/TTBR1_EL1   TTBR0/TTBR1        satp (per-proc)    CR3 (per-proc)
Page table switch       MSR TTBR0_EL1      MCR TTBR0          CSR satp           MOV CR3
TLB flush               TLBI ASIDE1IS      TLBIMVA/TLBIALL    SFENCE.VMA         INVLPG / MOV CR3
TLB identifier          ASID (16-bit)      ASID (8-bit)       ASID (16-bit)      PCID (12-bit)
MMIO access             MMIO (ioremap)      MMIO (ioremap)     MMIO (ioremap)     MMIO + I/O ports
Kernel load PA          0x4020_0000        0x6020_0000        0x8020_0000        0x0020_0000
Kernel load VA          0xFFFF_0000_       0xC020_0000        0xFFFF_FFE0_       0xFFFF_8000_
                        0020_0000                             0020_0000          0020_0000
Special                 SMMU IOMMU         none               OpenSBI M-mode     I/O port space
                                                              2 MB PA reserved   legacy 1 MB hole
------------------
┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│   UIOX — USERSPACE VIRTUAL ADDRESS LAYOUT  (all four architectures)                         │
│   How a user process sees its own address space after exec()                                 │
└─────────────────────────────────────────────────────────────────────────────────────────────┘

  ARM64 / RISC-V 64 / x86-64                     ARM32
  (48-bit VA — 128 TB user space)                 (32-bit VA — 3 GB user space)
  ──────────────────────────────────────────────  ────────────────────────────────────────────

  0x0000_0000_0000_0000                           0x0000_0000
  ┌──────────────────────────────────────────┐    ┌──────────────────────────────────────┐
  │  NULL guard page  (unmapped)             │    │  NULL guard page  (unmapped)         │
  │  any deref of NULL → page fault          │    │  any deref of NULL → page fault      │
  ├──────────────────────────────────────────┤    ├──────────────────────────────────────┤
  │                                          │    │                                      │
  │  .text   (rx — execute only)             │    │  .text   (rx)                        │
  │  Loaded by ELF loader                    │    │  Loaded by ELF loader                │
  │  ASLR: base randomised on exec()         │    │  ASLR: 33_PCS/05_sec randomises      │
  │  → uiox_aslr_randomise_mm()             │    │  → uiox_aslr_randomise_mm()         │
  │                                          │    │                                      │
  ├──────────────────────────────────────────┤    ├──────────────────────────────────────┤
  │  .rodata  (r -- read only)               │    │  .rodata  (r--)                      │
  ├──────────────────────────────────────────┤    ├──────────────────────────────────────┤
  │  .data    (rw)                           │    │  .data    (rw)                       │
  ├──────────────────────────────────────────┤    ├──────────────────────────────────────┤
  │  .bss     (rw — zero init)               │    │  .bss     (rw — zero init)           │
  ├──────────────────────────────────────────┤    ├──────────────────────────────────────┤
  │                                          │    │                                      │
  │  heap                                    │    │  heap                                │
  │  grows ↑ upward from .bss end            │    │  grows ↑ upward                      │
  │  managed by brk() / SYS_BRK             │    │  managed by brk() / SYS_BRK         │
  │  ASLR: heap base randomised              │    │  ASLR: heap base randomised          │
  │                                          │    │                                      │
  ├──────────────────────────────────────────┤    ├──────────────────────────────────────┤
  │                                          │    │                                      │
  │  mmap region                             │    │  mmap region                         │
  │  shared libs (.so)                       │    │  shared libs (.so)                   │
  │  anonymous mappings                      │    │  anonymous mappings                  │
  │  file mappings                           │    │  file mappings                       │
  │  grows ↓ downward                        │    │  grows ↓ downward                    │
  │  ASLR: mmap base randomised              │    │  ASLR: mmap base randomised          │
  │                                          │    │                                      │
  ├──────────────────────────────────────────┤    ├──────────────────────────────────────┤
  │                                          │    │                                      │
  │  thread stacks                           │    │  thread stacks                       │
  │  each thread gets its own stack          │    │  each thread gets its own stack      │
  │  guard page below each stack             │    │  guard page below each stack         │
  │                                          │    │                                      │
  ├──────────────────────────────────────────┤    ├──────────────────────────────────────┤
  │                                          │    │                                      │
  │  main stack                              │    │  main stack                          │
  │  grows ↓ downward                        │    │  grows ↓ downward                    │
  │  ASLR: stack base randomised             │    │  ASLR: stack base randomised         │
  │  argc / argv / envp at top               │    │  argc / argv / envp at top           │
  │                                          │    │                                      │
  ├──────────────────────────────────────────┤    ├──────────────────────────────────────┤
  │  vDSO  (virtual dynamic shared object)   │    │  vDSO  (signal trampoline)           │
  │  signal return trampoline                │    │  mapped just below stack             │
  │  fast syscalls (gettimeofday etc.)        │    │                                      │
  │  ASLR: vDSO base randomised              │    │                                      │
  ├──────────────────────────────────────────┤    ├──────────────────────────────────────┤
  │  [kernel space — inaccessible from user] │    │  [kernel space — inaccessible]       │
  │  any access → fault → SIGSEGV           │    │  any access → fault → SIGSEGV       │
  └──────────────────────────────────────────┘    └──────────────────────────────────────┘
  0x0000_7FFF_FFFF_FFFF                           0xBFFF_FFFF
  (128 TB user ceiling)                           (3 GB user ceiling)


┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│   SYSCALL PATH — how user space reaches kernel space  (all four architectures)               │
└─────────────────────────────────────────────────────────────────────────────────────────────┘

  ARM64                        ARM32                        RISC-V 64              x86-64
  ─────────────────────────    ─────────────────────────    ──────────────────     ──────────────────
  User issues:                 User issues:                 User issues:           User issues:
    SVC #0                       SVC #0  (or SWINT)           ECALL                  SYSCALL
    syscall nr in x8             syscall nr in r7             syscall nr in a7       syscall nr in rax
    args in x0–x5                args in r0–r6                args in a0–a5          args in rdi,rsi
                                                                                     rdx,r10,r8,r9
         │                            │                            │                      │
         ▼                            ▼                            ▼                      ▼
  EL0 → EL1 transition         USR → SVC mode               U-mode → S-mode        Ring3 → Ring0
  VBAR_EL1 + 0x400             vector table entry            stvec (trap vec)       LSTAR MSR
  (sync exception from EL0)    (software interrupt)          (ecall trap)           (syscall entry)
         │                            │                            │                      │
         ▼                            ▼                            ▼                      ▼
  arch_syscall_entry()          arch_syscall_entry()          arch_syscall_entry()   arch_syscall_entry()
  save x0–x30, SP, ELR          save r0–r15, SPSR             save a0–a7, pc         save rax,rdi..
  read syscall nr from x8       read syscall nr from r7       read nr from a7        read nr from rax
         │                            │                            │                      │
         ▼                            ▼                            ▼                      ▼
  syscall_dispatch(nr, args)    syscall_dispatch(nr, args)    syscall_dispatch()     syscall_dispatch()
  → 40_SystemCallInterface      → 40_SystemCallInterface      → 40_SCI               → 40_SCI
         │                            │                            │                      │
         ▼                            ▼                            ▼                      ▼
  return value in x0            return value in r0            return in a0           return in rax
  restore registers             restore registers             restore registers       restore registers
  ERET → EL0                    MOVS PC, LR (or ERET)         SRET → U-mode          SYSRET → Ring3


┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│   PAGE FAULT PATH — how MMU faults reach 33_PCS/02_MemMngnt  (all four architectures)       │
└─────────────────────────────────────────────────────────────────────────────────────────────┘

  ARM64                        ARM32                        RISC-V 64              x86-64
  ─────────────────────────    ─────────────────────────    ──────────────────     ──────────────────
  Process accesses             Process accesses             Process accesses       Process accesses
  unmapped/protected VA        unmapped/protected VA        unmapped VA            unmapped VA
         │                            │                            │                      │
         ▼                            ▼                            ▼                      ▼
  Data/Instr Abort             Prefetch/Data Abort           Instruction/Load/      #PF exception
  EL1 sync exception           enters Abort mode            Store page fault       IDT entry 14
         │                            │                            │                      │
         ▼                            ▼                            ▼                      ▼
  Read ESR_EL1                 Read IFSR / DFSR             Read scause            Read CR2
  (exception syndrome)         (fault status regs)          (trap cause CSR)       (fault address)
  Read FAR_EL1                 Read IFAR / DFAR             Read stval             (on stack)
  (fault address)              (fault address regs)         (fault address CSR)
         │                            │                            │                      │
         ▼                            ▼                            ▼                      ▼
  uiox_mm_page_fault()         uiox_mm_page_fault()         uiox_mm_page_fault()   uiox_mm_page_fault()
  → 33_PCS/02_MemMngnt         → 33_PCS/02_MemMngnt         → 33_PCS/02_MemMngnt   → 33_PCS/02_MemMngnt
         │                            │                            │                      │
    ┌────┴────┐                  ┌────┴────┐                 ┌────┴────┐             ┌────┴────┐
    ▼         ▼                  ▼         ▼                 ▼         ▼             ▼         ▼
  valid VA?  bad VA?           valid VA?  bad VA?           valid VA? bad VA?      valid VA? bad VA?
    │           │                │           │                │          │            │          │
    ▼           ▼                ▼           ▼                ▼          ▼            ▼          ▼
  demand    SIGSEGV           demand    SIGSEGV           demand    SIGSEGV       demand    SIGSEGV
  page        sent            page        sent            page        sent        page        sent
  in          to              in          to              in          to          in          to
  map         process         map         process         map         process     map         process


┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│   CONTEXT SWITCH — register + page table swap  (all four architectures)                     │
└─────────────────────────────────────────────────────────────────────────────────────────────┘

  ARM64                        ARM32                        RISC-V 64              x86-64
  ─────────────────────────    ─────────────────────────    ──────────────────     ──────────────────
  Save outgoing process:       Save outgoing process:       Save outgoing:         Save outgoing:
    X0–X30                       R0–R15                       ra,sp,gp,tp            rax–r15
    SP_EL0                       CPSR                         s0–s11                 rflags
    ELR_EL1                      PC                           a0–a7                  rip
    SPSR_EL1                                                  t0–t6                  cs,ss
    FPSR / FPCR (FP state)       VFP regs (if enabled)        f0–f31 (FP)            xmm0-15
                                                                                     (if used)
         │                            │                            │                      │
         ▼                            ▼                            ▼                      ▼
  Swap page tables:            Swap page tables:            Swap page tables:      Swap page tables:
    MSR TTBR0_EL1, new_pt        MCR TTBR0, new_pt            CSR satp, new_pt       MOV CR3, new_pt
    ISB barrier                  TLBIMVA / TLBIALL             SFENCE.VMA             INVLPG / flush
    TLBI ASIDE1IS                (flush TLB)                   (flush TLB)            (or PCID tag)
    (ASID-scoped TLB flush)
         │                            │                            │                      │
         ▼                            ▼                            ▼                      ▼
  Update thread pointer:        Update thread pointer:       Update tp register:    Update FS.base:
    MSR TPIDR_EL0, tp            MCR TPIDRURW, tp             tp ← new thread ptr    WRGSBASE / MSR
    (TLS pointer)                (TLS pointer)                (TLS — dedicated reg)  (TLS pointer)
         │                            │                            │                      │
         ▼                            ▼                            ▼                      ▼
  Restore incoming process:    Restore incoming process:    Restore incoming:      Restore incoming:
    LDR X0–X30                   LDR R0–R15                   restore ra–t6          restore all regs
    LDR SP_EL0                   LDR CPSR                     restore FP regs        restore xmm regs
    MSR ELR_EL1, pc              MOV PC, saved_pc
    MSR SPSR_EL1, spsr
    ERET → resumes process       MOVS PC, LR                  SRET → resumes         IRETQ → resumes


┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│   ASLR — what 33_PCS/05_sec randomises on every exec()  (all four architectures)            │
└─────────────────────────────────────────────────────────────────────────────────────────────┘

  Region          ARM64              ARM32              RISC-V 64          x86-64
  ─────────────   ──────────────     ──────────────     ──────────────     ──────────────
  .text base      randomised         randomised         randomised         randomised
  heap base       randomised         randomised         randomised         randomised
  mmap base       randomised         randomised         randomised         randomised
  stack base      randomised         randomised         randomised         randomised
  vDSO base       randomised         randomised         randomised         randomised
  kernel stack    randomised         randomised         randomised         randomised
                  per-thread         per-thread         per-thread         per-thread
                  (uiox_aslr_kstack) (uiox_aslr_kstack) (uiox_aslr_kstack)(uiox_aslr_kstack)

  Entropy source  TRNG (hardware)    TRNG (hardware)    TRNG (hardware)    TRNG (hardware)
                  seeded at          seeded at          seeded at          seeded at
                  uiox_sec_init()    uiox_sec_init()    uiox_sec_init()    uiox_sec_init()
                  33_PCS/05_sec      33_PCS/05_sec      33_PCS/05_sec      33_PCS/05_sec

  VA bits         48-bit space       32-bit space       48-bit space       48-bit space
  Entropy bits    ~28 bits           ~16 bits ◄low      ~28 bits           ~28 bits
                                     (32-bit limit)


┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│   COMPLETE MEMORY SUMMARY TABLE — all four architectures                                     │
└─────────────────────────────────────────────────────────────────────────────────────────────┘

  Property                  ARM64               ARM32               RISC-V 64           x86-64
  ──────────────────────    ────────────────    ────────────────    ────────────────    ────────────────
  Physical addr width       48-bit              32-bit (no LPAE)    56-bit              52-bit
  Virtual addr width        48-bit              32-bit              48-bit (Sv48)       48-bit (PML4)
  Page size                 4 KB                4 KB                4 KB                4 KB
  Page table levels         4 (PGD/PUD/PMD/PT)  2 (PGD/PT)          4 (PGD/PUD/PMD/PT)  4 (PML4/PDP/PD/PT)
  User VA range             0 – 128 TB          0 – 3 GB            0 – 128 TB          0 – 128 TB
  Kernel VA base            0xFFFF_0000_0000    0xC000_0000         0xFFFF_FFE0_0000    0xFFFF_8000_0000
  Canonical hole            yes                 no (32-bit)         yes                 yes
  Kernel/user reg split     TTBR0/TTBR1_EL1     TTBR0/TTBR1         satp (single reg)   CR3 (single reg)
  TLB identifier            ASID 16-bit         ASID 8-bit          ASID 16-bit         PCID 12-bit
  TLB flush instruction     TLBI ASIDE1IS       TLBIMVA/ALL         SFENCE.VMA          INVLPG/MOV CR3
  Thread-local storage      TPIDR_EL0 (MSR)     TPIDRURW (MCR)      tp register         FS.base (MSR)
  Syscall mechanism         SVC #0 / EL0→EL1    SVC #0 / USR→SVC    ECALL / U→S mode    SYSCALL / R3→R0
  Syscall nr register       x8                  r7                  a7                  rax
  Page fault addr reg       FAR_EL1             IFAR / DFAR         stval CSR           CR2
  Page fault cause reg      ESR_EL1             IFSR / DFSR         scause CSR          error code (IDT)
  DRAM base (QEMU)          0x4000_0000         0x6000_0000         0x8000_0000         0x0000_0000
  Kernel load PA            0x4020_0000         0x6020_0000         0x8020_0000         0x0020_0000
  Kernel load VA            0xFFFF_0000_         0xC020_0000         0xFFFF_FFE0_         0xFFFF_8000_
                            0020_0000                               0020_0000           0020_0000
  Stack size                64 KB               32 KB ◄             64 KB               64 KB
  Stack alignment           16-byte             8-byte              16-byte             16-byte
  Section alignment         8-byte              4-byte              8-byte              16-byte ◄
  Float in kernel           hardware FPU        soft-float ◄        hardware FPU        -mno-sse ◄
  Code model                default             default             medany              kernel
  Special PA reservation    none                none                OpenSBI 2 MB ◄      1 MB legacy ◄
  MMIO location             below 0x4000_0000   below 0x6000_0000   below 0x8000_0000   0xFEC0_0000+
  IOMMU                     SMMU 0x0905_0000    none                none                IOAPIC routing
  Interrupt controller      GIC-400 / GICv3     GIC                 PLIC                LAPIC + IOAPIC
  Timer source              Generic timer (ISA)  SP804 MMIO          CLINT mtime         HPET
  ASLR entropy              ~28 bits            ~16 bits ◄          ~28 bits            ~28 bits
  Free DRAM                 ~49 MB              ~50 MB              ~47 MB ◄            ~49 MB
  ──────────────────────────────────────────────────────────────────────────────────────────────
  ◄ marks the most constrained / notable value for that property
==============================================
data from kernel space/ SOC space/FwHal Sape can be copied to user space

This is a large topic — let me break it into three parts: how the copy works, what UIOX needs to add, and which files to update with the actual code.

Part 1 — How kernel/SoC/FwHal data reaches userspace
There are exactly four mechanisms by which kernel-space data crosses the privilege boundary to userspace. UIOX currently has none of them implemented — it has the kernel subsystems but no copy path to userspace yet.
┌─────────────────────────────────────────────────────────────────────┐
│   Kernel / SoC / FwHal data                                         │
│   (privilege level: EL1 / S-mode / Ring0)                           │
└──────────────────────┬──────────────────────────────────────────────┘
                       │  Four crossing mechanisms
         ┌─────────────┼─────────────┬─────────────┐
         ▼             ▼             ▼              ▼
    [1] syscall   [2] copy_to_user  [3] mmap     [4] vDSO
    read()/ioctl  explicit copy     shared page  mapped r/o
    write()/...   kernel→user buf   no copy      page in user VA


[1] Syscall — read() / ioctl() / sysfs-style
User calls read(fd, buf, len) → SVC #0 / ECALL / SYSCALL → kernel reads data from driver/SoC → copies bytes into user buffer using copy_to_user().
User:    read(fd, user_buf, 64)
           │
           ▼  SVC/ECALL/SYSCALL
Kernel:  sys_read(fd, user_buf, 64)
           → file→ops→read()
               → driver reads from SoC MMIO / FwHal
               → copy_to_user(user_buf, kernel_buf, 64)
                   → validates user_buf is in user VA range
                   → copies bytes across privilege boundary
           ← return 64
           │
           ▼  ERET/SRET/SYSRET
User:    buf now contains SoC data


[2] copy_to_user / copy_from_user — the core primitive
This is the fundamental building block. It must:

Validate the user pointer is within the user VA range (not pointing into kernel space)
Copy the bytes
Handle faults — if the user pointer is bad, return an error instead of panicking


[3] mmap — zero-copy shared memory
For high-bandwidth data (camera frames, GPU buffers, audio) — the kernel maps its buffer directly into user VA space. No copy needed.
User:   mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0)
          → SYS_MMAP → kernel
          → kernel maps physical frame into user page table
          → returns user VA pointing at same physical page
          → user reads directly — zero copy


[4] vDSO — fastest path, read-only kernel data
For data the kernel updates continuously (time, cpu tick) — a read-only page is mapped into every process's VA. User reads it directly without a syscall at all.


Part 2 — What UIOX needs to add
UIOX currently has:

✅ Kernel subsystems (drivers, buffers, FS, PCS)
✅ Syscall table defined (40_SystemCallInterface — SYS numbers)
✅ Process/memory management (33_PCS/02_MemMngnt)
❌ No copy_to_user / copy_from_user implementation
❌ No syscall dispatch table with actual handlers
❌ No userspace API layer (40_SystemCallInterface stubs only)
❌ No mmap path from kernel buffers to user VA
❌ No vDSO page



Part 3 — Files to update with actual code
Here are all the files, what to add, and the complete source for each:
File 1 — 30_KIX/33_PCS/include/uiox_uaccess.h (new file)
File 2 — 30_KIX/33_PCS/src/uiox_uaccess.c (new file)
File 3 — 30_KIX/33_PCS/include/uiox_syscall.h (new file)
File 4 — 30_KIX/33_PCS/src/uiox_syscall.c (new file)
File 5 — 10_BSP/10_Arch/arm64/src/arch_init.c — update syscall entry hook
Add this block inside arch_init.c after the existing IRQ handler registration (step 7). This wires the vector table's SVC path to uiox_syscall_dispatch():
/* Add to arch_init.c — after irq_enable(UART0_IRQ) */

#include "uiox_syscall.h"   /* uiox_syscall_dispatch, uiox_syscall_frame_t */

/*
 * arch_syscall_entry — called from the EL0 sync exception vector
 * when the instruction class in ESR_EL1 is SVC.
 *
 * On entry (set by vector table stub in assembly):
 *   x0–x5  = syscall arguments
 *   x8     = syscall number
 *   sp     = kernel stack (set up by vector table entry)
 *
 * Returns: value placed back in x0 before ERET to EL0.
 */
long arch_syscall_entry(unsigned long nr,
                        unsigned long a0, unsigned long a1,
                        unsigned long a2, unsigned long a3,
                        unsigned long a4, unsigned long a5)
{
    uiox_syscall_frame_t frame = {
        .nr = nr,
        .a0 = a0, .a1 = a1, .a2 = a2,
        .a3 = a3, .a4 = a4, .a5 = a5,
    };
    return (long)uiox_syscall_dispatch(&frame);
}
The same arch_syscall_entry() pattern applies to the other three arch files (arch_init.c for arm32, riscv64, x86_64) — only the register names differ (r7/r0–r5 for arm32, a7/a0–a5 for riscv64, rax/rdi–r9 for x86-64).

File 6 — 30_KIX/33_PCS/include/uiox_ioctl.h (new file)
This defines the ioctl commands that expose SoC/FwHal/driver data to userspace:

File 7 — 30_KIX/33_PCS/src/uiox_ioctl_soc.c (new file)
This implements the actual SoC/FwHal → userspace data path for the most important ioctls:

File 8 — 30_KIX/33_PCS/src/uiox_mmap.c (new file)
This implements the zero-copy mmap path — maps kernel/device buffer physical pages directly into user page tables:

File 9 — update 30_KIX/33_PCS/Makefile
Add the three new source files to the 33_PCS build:

makefile
# In 33_PCS/Makefile — add to SRCS list alongside existing files:

SRCS += $(MFDIR)src/uiox_uaccess.c
SRCS += $(MFDIR)src/uiox_syscall.c
SRCS += $(MFDIR)src/uiox_ioctl_soc.c
SRCS += $(MFDIR)src/uiox_mmap.c


File 10 — update 30_KIX/uiox_kernel_main.c
Add the syscall include to KERNEL_CFLAGS_BASE (already in Makefile) and add one line in kernel_common_init() to register the syscall dispatch hook with the arch layer:
/* Add near the top of uiox_kernel_main.c includes: */
#include "uiox_syscall.h"
#include "uiox_uaccess.h"

/* In kernel_common_init(), after uiox_proc_init(): */
early_puts("[kernel] uiox_syscall_dispatch ready\r\n");
/*
 * The arch vector table calls uiox_syscall_dispatch() directly
 * via arch_syscall_entry() — no registration needed here.
 * This line confirms the symbol is linked and the table is live.
 */



Complete picture — data flow for every crossing type
┌─────────────────────────────────────────────────────────────────────┐
│  SoC / FwHal / Driver data  (kernel privilege)                      │
│                                                                      │
│  uiox_soc_get_desc()     → uiox_soc_arm64_detect()                  │
│  uiox_clk_get_hz()       → uiox_soc_clk.c → SoC PLL MMIO           │
│  uiox_bms_read_data()    → 30_DeviceDrivers → FwHal → I2C → chip   │
│  uiox_therm_read_zone()  → 30_DeviceDrivers → FwHal → ADC → sensor │
│  uiox_wifi_get_status()  → 30_DeviceDrivers → uiox_wifi_if_stats   │
│  uiox_cam_get_frame()    → 31_BufferCache → DMA PA                  │
│  uiox_fw_dma_transfer()  → 03_SoC → PL080/PL330 MMIO               │
└──────────────────┬──────────────────────────────────────────────────┘
                   │
          Four crossing paths
                   │
    ┌──────────────┼─────────────────┬────────────────┐
    ▼              ▼                 ▼                 ▼
[ioctl]       [read/write]       [mmap]            [vDSO]
uiox_ioctl    sys_read/write     uiox_mm_map       kernel page
_soc_dispatch copy_to_user       _user_phys        mapped r/o
              copy_from_user     zero copy         into user VA
    │              │                 │                 │
    ▼              ▼                 ▼                 ▼
uiox_copy_to   uiox_copy_to     PTE inserted      user reads
_user()        _user()          into TTBR0/        directly
checks         copies bytes     satp/CR3           no syscall
user VA        kernel→user      TLB flushed        needed
valid first    VA boundary      user VA→same PA


seesm these file looks like system files, if these are the kernel files then still can tell me why these belongs to PCS(process control subsystems ) not


Great question — the placement in 33_PCS is deliberate and architectural, not arbitrary. Here's the full reasoning:

Why these files belong in 33_PCS and not somewhere else
The short answer
These files are not system call infrastructure in the Unix sense — they are the process control subsystem's interface to the rest of the kernel. Every one of them touches the process model directly:

uiox_uaccess.c — validates and crosses the per-process privilege boundary
uiox_syscall.c — dispatches on behalf of the current process
uiox_ioctl_soc.c — checks process permissions before exposing hardware
uiox_mmap.c — inserts PTEs into the current process's page table
The process is the unit of isolation. None of these make sense without a process context.



The dependency chain — why PCS owns this
uiox_uaccess_ok()
    │
    └── needs to know the user VA ceiling
        which is set by: TTBR0_EL1 / satp / CR3
        which is owned by: 33_PCS/02_MemMngnt
        which is managed per-process by: 33_PCS/40_procStruct


uiox_copy_to_user()
    │
    └── uses: current process's page table
        to validate the destination VA
        page table lives in: 33_PCS/02_MemMngnt
        current process ptr lives in: 33_PCS/40_procStruct


uiox_syscall_dispatch()
    │
    └── dispatches to handlers that call:
        uiox_proc_exit()     → 33_PCS/40_procStruct
        uiox_sched_tick()    → 33_PCS/01_schedular
        uiox_mm_page_fault() → 33_PCS/02_MemMngnt
        uiox_sec_check()     → 33_PCS/05_sec
        uiox_kp_engine()     → 33_PCS/06_kpatch


uiox_mmap()
    │
    └── directly modifies:
        proc->mm.pgd         → 33_PCS/02_MemMngnt
        proc->vma_list       → 33_PCS/40_procStruct
        calls: uiox_tlb_flush_user() → arch via 33_PCS






The correct layering diagram
┌─────────────────────────────────────────────────────────────┐
│  User space processes                                        │
│  (own VA: TTBR0/satp/CR3 — per process)                     │
└──────────────────────┬──────────────────────────────────────┘
                       │  SVC / ECALL / SYSCALL
                       ▼
┌─────────────────────────────────────────────────────────────┐
│  33_PCS  — Process Control Subsystem                        │
│                                                              │
│  uiox_uaccess.c    ← validates user VA per process          │
│  uiox_syscall.c    ← dispatches for current process         │
│  uiox_ioctl_soc.c  ← checks process MAC before hw access   │
│  uiox_mmap.c       ← writes PTEs into process page table   │
│                                                              │
│  40_procStruct/    ← process descriptor, file table         │
│  02_MemMngnt/      ← page tables, physical allocator        │
│  01_schedular/     ← current process context                │
│  05_sec/           ← MAC policy per process                 │
│  06_kpatch/        ← live patch engine                      │
└──────────┬──────────────────────────────────────────────────┘
           │  calls into (never the reverse)
           ▼
┌──────────────────┐  ┌──────────────┐  ┌──────────────────┐
│  32_FS           │  │ 30_DevDrv    │  │ 31_BufferCache   │
│  02_journal      │  │ WiFi/BMS/Cam │  │ frame pools      │
│  03_netfs        │  │ thermal/USB  │  │ DMA buffers      │
└──────────────────┘  └──────────────┘  └──────────────────┘
           │                │                    │
           ▼                ▼                    ▼
┌─────────────────────────────────────────────────────────────┐
│  02_FwHal  +  03_SoC                                        │
│  hardware registers, MMIO, I2C, DMA, clocks                 │
│  no process concept — pure hardware abstraction             │
└─────────────────────────────────────────────────────────────┘

-------------
The one case where 32_FS participates in the path
sys_read() and sys_write() are in 33_PCS/uiox_syscall.c but they call into 32_FS to get the actual data:

33_PCS/uiox_syscall.c
    sys_read(fd, ubuf, count)
        │
        ├── uiox_uaccess_ok(ubuf)         ← 33_PCS validates user ptr
        ├── fd_lookup(fd)                  ← 33_PCS/40_procStruct
        │       returns file*
        ├── file->ops->read(file, kbuf)   ← 32_FS provides this vtable
        │       reads from journal/inode
        └── uiox_copy_to_user(ubuf, kbuf) ← 33_PCS copies to user
======================================================================
The complete gap analysis — syscall by syscall
┌─────────────────────────────────────────────────────────────────────────────┐
│  SYSCALL        │  Algorithm exists?  │  Freestanding?  │  Wired to arch?   │
├─────────────────┼─────────────────────┼─────────────────┼───────────────────┤
│  PROCESS LIFECYCLE                                                           │
├─────────────────┼─────────────────────┼─────────────────┼───────────────────┤
│  sys_fork       │  ✅ 50_scps/fork.c  │  ❌ uses libc   │  ❌ not wired     │
│  sys_exec       │  ✅ 50_scps/exec.c  │  ❌ uses libc   │  ❌ not wired     │
│  sys_exit       │  ✅ 50_scps/exit.c  │  ❌ uses libc   │  ❌ not wired     │
│  sys_wait       │  ✅ 50_scps/wait.c  │  ❌ uses libc   │  ❌ not wired     │
│  sys_getpid     │  ⚠️  stub only      │  ✅             │  ❌ not wired     │
│  sys_getppid    │  ❌ missing         │  —              │  —                │
│  sys_kill       │  ✅ 50_scps/signal.c│  ❌ uses libc   │  ❌ not wired     │
│  sys_signal     │  ✅ 50_scps/signal.c│  ❌ uses libc   │  ❌ not wired     │
├─────────────────┼─────────────────────┼─────────────────┼───────────────────┤
│  SCHEDULING                                                                  │
├─────────────────┼─────────────────────┼─────────────────┼───────────────────┤
│  sys_sched_yield│  ✅ 01_schedular    │  ❌ uses libc   │  ❌ not wired     │
│  sys_setpriority│  ⚠️  partial        │  ❌             │  ❌               │
│  sys_getpriority│  ⚠️  partial        │  ❌             │  ❌               │
│  sys_nanosleep  │  ❌ missing         │  —              │  —                │
│  sys_clock_get  │  ❌ missing         │  —              │  —                │
├─────────────────┼─────────────────────┼─────────────────┼───────────────────┤
│  MEMORY                                                                      │
├─────────────────┼─────────────────────┼─────────────────┼───────────────────┤
│  sys_brk        │  ⚠️  stub — hardcoded 0x10000        │  ❌ no page tables │
│  sys_mmap       │  ⚠️  stub — returns addr, no PTEs    │  ❌               │
│  sys_munmap     │  ❌ missing         │  —              │  —                │
│  sys_mprotect   │  ❌ missing         │  —              │  —                │
│  page_fault     │  ✅ COW + demand-zero│ ❌ uses calloc  │  ❌ static table  │
├─────────────────┼─────────────────────┼─────────────────┼───────────────────┤
│  IPC                                                                         │
├─────────────────┼─────────────────────┼─────────────────┼───────────────────┤
│  sys_msgget     │  ✅ 00_IPC/msg.c    │  ❌ uses libc   │  ❌ not wired     │
│  sys_msgsnd     │  ✅ 00_IPC/msg.c    │  ❌ uses libc   │  ❌ not wired     │
│  sys_msgrcv     │  ✅ 00_IPC/msg.c    │  ❌ uses libc   │  ❌ not wired     │
│  sys_msgctl     │  ✅ 00_IPC/msg.c    │  ❌ uses libc   │  ❌ not wired     │
│  sys_shmget     │  ✅ 00_IPC/shm.c    │  ❌ uses libc   │  ❌ not wired     │
│  sys_shmat      │  ✅ 00_IPC/shm.c    │  ❌ uses libc   │  ❌ not wired     │
│  sys_shmdt      │  ✅ 00_IPC/shm.c    │  ❌ uses libc   │  ❌ not wired     │
│  sys_shmctl     │  ✅ 00_IPC/shm.c    │  ❌ uses libc   │  ❌ not wired     │
│  sys_semget     │  ✅ 00_IPC/sem.c    │  ❌ uses libc   │  ❌ not wired     │
│  sys_semop      │  ✅ 00_IPC/sem.c    │  ❌ uses libc   │  ❌ not wired     │
│  sys_semctl     │  ✅ 00_IPC/sem.c    │  ❌ uses libc   │  ❌ not wired     │
├─────────────────┼─────────────────────┼─────────────────┼───────────────────┤
│  SECURITY                                                                    │
├─────────────────┼─────────────────────┼─────────────────┼───────────────────┤
│  sys_getlabel   │  ✅ 05_sec          │  ✅             │  ❌ not wired     │
│  sys_setlabel   │  ✅ 05_sec          │  ✅             │  ❌ not wired     │
│  sys_getpolicy  │  ✅ 05_sec          │  ✅             │  ❌ not wired     │
│  sys_setpolicy  │  ✅ 05_sec          │  ✅             │  ❌ not wired     │
│  sys_aslr_status│  ✅ 05_sec          │  ✅             │  ❌ not wired     │
├─────────────────┼─────────────────────┼─────────────────┼───────────────────┤
│  MISSING ENTIRELY — not in repo at all                                       │
├─────────────────┼─────────────────────┼─────────────────┼───────────────────┤
│  sys_read       │  ❌                 │  —              │  —                │
│  sys_write      │  ❌                 │  —              │  —                │
│  sys_open       │  ❌                 │  —              │  —                │
│  sys_close      │  ❌                 │  —              │  —                │
│  sys_ioctl      │  ❌                 │  —              │  —                │
│  sys_pipe       │  ❌                 │  —              │  —                │
│  sys_dup/dup2   │  ❌                 │  —              │  —                │
│  sys_poll/select│  ❌                 │  —              │  —                │
│  copy_to_user   │  ❌                 │  —              │  —                │
│  copy_from_user │  ❌                 │  —              │  —                │
│  uiox_sys_call_init│  ❌ called but not found in repo  │  —                │
└─────────────────┴─────────────────────┴─────────────────┴───────────────────┘
=======================================

data from kernel space/ DRAM/RAm Region space can be copied to user space

Part 1 — Current state of 32_FS
32_FS/
├── src/uiox_fs_init.c    ✅ Clean skeleton — correct weak stub pattern
│                            calls vfs_init(), scfs_init(), vfs_mount_root()
│                            uses printf() ← needs replacing with early_puts
├── 01_fsa/               ❌ Empty — no VFS layer at all
├── 10_scfs/              ❌ Empty — no filesystem implementation
├── 02_journal/           ❌ Empty — no journal (moved from 50_UIX but not written)
├── 03_netfs/             ❌ Empty — no NFS client (moved from 50_UIX but not written)
└── include/              ❓ Not fetched — likely empty


Part 2 — How DRAM/kernel data reaches userspace through the FS path
DRAM (physical pages)
│
│  33_PCS/02_MemMngnt owns physical pages
│  32_FS/01_fsa owns the page cache (file data in DRAM)
│
▼
┌─────────────────────────────────────────────────────────┐
│  Page cache  (32_FS/01_fsa)                             │
│  Physical DRAM pages backing file content               │
│  inode → block map → physical pages in DRAM             │
└──────────────────────┬──────────────────────────────────┘
                       │
        Three paths to userspace
                       │
    ┌──────────────────┼───────────────┐
    ▼                  ▼               ▼
[read()]          [mmap()]        [sendfile()]
copy_to_user      map PA into     kernel→kernel
kernel page       user PTE        zero-copy
→ user buffer     zero copy       (future)


Part 3 — Complete gap analysis
┌──────────────────────┬─────────────────┬──────────────┬────────────────┐
│ Component            │ Exists?         │ Freestanding?│ Wired?         │
├──────────────────────┼─────────────────┼──────────────┼────────────────┤
│ VFS layer            │ ❌ missing      │ —            │ —              │
│ inode / dentry cache │ ❌ missing      │ —            │ —              │
│ file descriptor table│ ❌ missing      │ —            │ —              │
│ vfs_open/read/write  │ ❌ missing      │ —            │ —              │
│ vfs_mount_root       │ ⚠️  stub only   │ ✅ (stub)    │ ❌             │
│ SCFS implementation  │ ❌ missing      │ —            │ —              │
│ Journal              │ ❌ missing      │ —            │ —              │
│ NetFS/NFS            │ ❌ missing      │ —            │ —              │
│ page cache           │ ❌ missing      │ —            │ —              │
│ sys_read → FS        │ ❌ not wired    │ —            │ —              │
│ sys_write → FS       │ ❌ not wired    │ —            │ —              │
│ sys_open → FS        │ ❌ not wired    │ —            │ —              │
│ sys_close → FS       │ ❌ not wired    │ —            │ —              │
│ copy_to_user in FS   │ ❌ missing      │ —            │ —              │
│ printf() in init     │ ⚠️  libc banned │ ❌           │ —              │
└──────────────────────┴─────────────────┴──────────────┴────────────────┘


Part 4 — All files to create or update, with full source
File 1 — 32_FS/include/uiox_vfs.h (new)
.....


===========
┌──────────────────────────────────────────────────────────────────────────┐
│  Files to CREATE                                                          │
├──────────────────────────────────────────────────────────────────────────┤
│  32_FS/include/uiox_vfs.h          VFS types + public API               │
│  32_FS/01_fsa/vfs.c                VFS implementation (path_lookup,      │
│                                     open/close/read/write/stat/readdir)  │
│  32_FS/10_scfs/scfs.c              SCFS implementation (page cache,      │
│                                     block read, mmap_page zero-copy)     │
│  33_PCS/src/uiox_sys_fd.c          sys_read/write/open/close/stat/       │
│                                     lseek/getdents — the crossing point  │
│  33_PCS/src/uiox_sys_call_init.c   uiox_sys_call_init() body             │
├──────────────────────────────────────────────────────────────────────────┤
│  Files to UPDATE                                                          │
├──────────────────────────────────────────────────────────────────────────┤
│  32_FS/src/uiox_fs_init.c          Replace printf → early_puts,         │
│                                     add uiox_jr_init() call              │
│  33_PCS/src/uiox_syscall.c         Wire FS syscalls to dispatch table    │
│  33_PCS/Makefile                   Add uiox_sys_fd.c,                    │
│                                     uiox_sys_call_init.c to SRCS         │
│  32_FS/Makefile                    Add 01_fsa/vfs.c,                     │
│                                     10_scfs/scfs.c to SRCS               │
└──────────────────────────────────────────────────────────────────────────┘

Data path — DRAM to userspace in one line per function:

  DRAM page  →  scfs_get_page()  →  scfs_read() memcpy  →  kbuf
  kbuf       →  vfs_read()       →  sys_read()
  kbuf       →  uiox_copy_to_user(ubuf, kbuf, n)  ←  CROSSING POINT
  ubuf       →  user process  ← data arrives

Zero-copy path (mmap):

  DRAM page  →  scfs_mmap_page() returns PA
  PA         →  uiox_mm_map_user_phys() inserts PTE
  user VA    →  user reads directly  ← no copy at all
============================================
┌────────────────────────────────────────────────────────────────────┐
│  DRAM  (physical pages)                                             │
│  ├─ Kernel image    (.text/.data/.bss — static, never copied)      │
│  ├─ Page cache      (32_FS/10_scfs — file data blocks)             │
│  ├─ Journal log     (32_FS/02_journal — write-ahead log)           │
│  └─ Anonymous pages (33_PCS/02_MemMngnt — heap/stack/mmap)         │
└───────────────────────────┬────────────────────────────────────────┘
                            │
             Three paths to userspace
                            │
    ┌───────────────────────┼─────────────────────┐
    ▼                       ▼                      ▼
[Path 1: read()]      [Path 2: mmap()]       [Path 3: ioctl()]
scfs_get_page()       vfs_mmap_page()        driver reads SoC
→ kbuf (kernel)       → PA of DRAM page     → kbuf (kernel)
→ vfs_read()          → uiox_mm_map         → uiox_copy_to
→ sys_read()            _user_phys()           _user()
→ copy_to_user()      → PTE in user PT      → user buffer
→ user buffer         → user reads VA       no copy needed
  (memcpy cross)        (zero copy)         for mmap
    │                       │                      │
    ▼                       ▼                      ▼
  EFAULT if           Page fault if          EFAULT if
  bad user ptr        bad PA/prot            bad user ptr

[Write path: write()]
user buffer
→ copy_from_user()    ← CROSSING POINT
→ kbuf (kernel)
→ vfs_write()
→ uiox_jr_start()     ← begin transaction
→ modify page cache   ← DRAM write
→ uiox_jr_stop()      ← end transaction
→ [SYS_SYNC]
→ uiox_jr_force_commit() → journal durable
→ uiox_jr_checkpoint()   → data durable


===========================================
Page cache files as mentioned above are written here https://github.com/Pramod645/UIOX/tree/main/30_KIX/31_BufferCache/00_FileBuff, is not the currect or what is the gap for page cache or even for cache implmenetetion ?


What 00_FileBuff actually is — and what it is NOT
The 00_FileBuff implementation is a classic Unix buffer cache (the getblk/bread/bwrite/brelse algorithm from Bach's "The Design of the Unix Operating System"). It is not a page cache. These are two different things and UIOX needs both.


┌─────────────────────────────────────────────────────────────────────┐
│  What 00_FileBuff HAS (Block Buffer Cache — ✅ well implemented)    │
├─────────────────────────────────────────────────────────────────────┤
│  Unit:     512-byte disk BLOCK (BLOCK_SIZE = 512)                   │
│  Identity: (dev, blkno) — which device, which disk block            │
│  Lookup:   hash table  (blkno % NUM_HASH_QUEUES)                    │
│  Eviction: LRU free list with dummy head sentinel                   │
│  Algorithms:                                                        │
│    getblk  — all 5 scenarios (lock, wait, delayed-write flush)      │
│    brelse  — release + LRU requeue                                  │
│    bread   — read block (cache hit = immediate, miss = I/O + sleep) │
│    breada  — read + read-ahead                                      │
│    bwrite  — write (sync / async / delayed)                         │
│    bdwrite — mark delayed-write + release                           │
│    bflush  — flush all delayed-write buffers (called by SYS_SYNC)  │
│  Stats:    hits, misses, delayed_writes, free_waits, busy_waits     │
├─────────────────────────────────────────────────────────────────────┤
│  What 00_FileBuff DOES NOT HAVE (Page Cache — ❌ missing entirely)  │
├─────────────────────────────────────────────────────────────────────┤
│  Unit:     4096-byte PAGE (not 512-byte block)                      │
│  Identity: (inode, file_offset) — not (dev, blkno)                 │
│  mmap:     no PA→VA zero-copy mapping support                       │
│  VFS link: no connection to inode / uiox_file_t                    │
│  readpage: no "fill page from block device" operation               │
│  writeback: no dirty page writeback to block device                 │
│  truncate: no page eviction on file truncation                      │
│  User mmap:no physical page address exposed for PTE insertion       │
└─────────────────────────────────────────────────────────────────────┘




The conceptual difference — block cache vs page cache
BLOCK BUFFER CACHE (00_FileBuff — what exists)
═══════════════════════════════════════════════
  Disk block 7 of device 0
  ┌───────────────────────────┐
  │  BufHdr                   │
  │  dev   = 0                │
  │  blkno = 7                │ ← identity is (dev, blkno)
  │  status = BUF_VALID       │
  │  data[512]                │ ← 512 bytes, not a page
  └───────────────────────────┘
  Used by: filesystem code that knows disk layout
  Not used by: mmap(), VFS read/write, userspace

PAGE CACHE (missing — what is needed)
══════════════════════════════════════
  Page 2 of file /etc/config (inode 42)
  ┌───────────────────────────────────┐
  │  page_cache_entry                 │
  │  inode  = 42                      │
  │  offset = 8192  (page 2 × 4096)  │ ← identity is (inode, offset)
  │  pa     = 0x44010000              │ ← physical DRAM address
  │  data[4096]                       │ ← 4096 bytes = one page
  │  dirty  = 0                       │
  │  uptodate = 1                     │
  └───────────────────────────────────┘
  Used by: vfs_read(), vfs_write(), sys_mmap() zero-copy
  Exposed via: mmap_page() → PA → PTE insertion


The gap analysis — exactly what is missing
┌──────────────────────────┬─────────────────┬──────────────────────────────┐
│ Feature                  │ 00_FileBuff      │ Page Cache (needed)          │
├──────────────────────────┼─────────────────┼──────────────────────────────┤
│ Block read from device   │ ✅ bread()       │ needs readpage() wrapper     │
│ Block write to device    │ ✅ bwrite()      │ needs writepage() wrapper    │
│ Delayed write / sync     │ ✅ bdwrite/flush │ needs writeback_page()       │
│ LRU eviction             │ ✅ free list     │ needs page reclaim           │
│ Read-ahead               │ ✅ breada()      │ needs readahead_pages()      │
│ Page-sized unit (4096)   │ ❌ 512 bytes     │ needs page_cache_entry       │
│ Inode-based identity     │ ❌ (dev,blkno)   │ needs (inode, offset)        │
│ Physical address exposed │ ❌ no PA         │ needs pa field for mmap      │
│ VFS inode link           │ ❌ no inode ptr  │ needs inode→page_cache       │
│ mmap zero-copy           │ ❌ impossible    │ needs mmap_page() → PA       │
│ Dirty page tracking      │ ⚠️ BUF_DELWRITE  │ needs per-page dirty bit    │
│ Truncation eviction      │ ❌ missing       │ needs truncate_page_cache()  │
│ Freestanding (no libc)   │ ❓ unknown       │ must be -ffreestanding       │
│ BLOCK_SIZE               │ 512 bytes        │ must be 4096 (PAGE_SIZE)     │
│ NUM_BUFFERS              │ 16 (too small)   │ needs ~128+ for real use     │
│ NUM_DISK_BLOCKS          │ 64 (simulated)   │ needs real device size       │
└──────────────────────────┴─────────────────┴──────────────────────────────┘



The correct architecture — how both layers work together
┌─────────────────────────────────────────────────────────────────────────┐
│  32_FS / VFS                                                             │
│  vfs_read(file, kbuf, n)  →  needs page at (inode, offset)              │
└──────────────────────────────────┬──────────────────────────────────────┘
                                   │  calls
                                   ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  PAGE CACHE  (to be added to 31_BufferCache/00_FileBuff)                │
│                                                                          │
│  uiox_page_cache_read(inode, offset, kbuf, len)                         │
│    → lookup (inode, offset) in page cache                               │
│    → HIT:  memcpy from cached page → kbuf  (DRAM→kbuf)                 │
│    → MISS: call readpage(inode, offset)                                  │
│              → translate offset → (dev, blkno)                          │
│              → call bread(dev, blkno)  ← 00_FileBuff block cache        │
│              → assemble 4096-byte page from 8 × 512-byte blocks         │
│              → store in page cache                                       │
│              → memcpy to kbuf                                            │
│    → return kbuf to vfs_read()                                          │
└──────────────────────────────────┬──────────────────────────────────────┘
                                   │  calls
                                   ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  BLOCK BUFFER CACHE  (00_FileBuff — already exists ✅)                  │
│                                                                          │
│  bread(dev, blkno) → BufHdr with 512 bytes of disk data                │
│  bwrite(buf)       → write 512 bytes to disk                            │
│  bflush(dev)       → flush all delayed writes (SYS_SYNC)               │
└─────────────────────────────────────────────────────────────────────────┘
                                   │  calls
                                   ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  BLOCK DEVICE  (30_DeviceDrivers/03_NonSensors/emmc or 31_BufferCache)  │
│  Physical read/write to eMMC / SSD / virtual disk in DRAM               │
└─────────────────────────────────────────────────────────────────────────┘



What needs to be fixed in 00_FileBuff first
Before adding the page cache, two things in 00_FileBuff need fixing:

Fix 1 — BLOCK_SIZE must be configurable

/* bcache_types.h — current (wrong for page-cache use): */
#define BLOCK_SIZE  512

/* Needs to become: */
#define BCACHE_BLOCK_SIZE   512u    /* physical sector size — never change */
#define BCACHE_PAGE_SIZE    4096u   /* logical page size = 8 blocks        */
#define BCACHE_BLOCKS_PER_PAGE  (BCACHE_PAGE_SIZE / BCACHE_BLOCK_SIZE)  /* = 8 */



Fix 2 — NUM_BUFFERS is too small
/* current: */
#define NUM_BUFFERS  16    /* only 16 × 512 = 8 KB total — useless         */

/* needs to be: */
#define NUM_BUFFERS  256   /* 256 × 512 = 128 KB — reasonable for boot      */
                           /* 1024 × 512 = 512 KB — good for general use    */



Fix 3 — src/ directory is empty
The 00_FileBuff/src/ directory returned an empty listing. The implementation of getblk, bread, bwrite, brelse, bflush exists only as declarations in the header — the .c file is missing. It needs to be written.



Files to create or update
File 1 — 31_BufferCache/00_FileBuff/include/bcache_types.h (update)

File 2 — 31_BufferCache/00_FileBuff/src/bcache.c (new — implements the missing .c)
File 3 — 31_BufferCache/00_FileBuff/include/uiox_page_cache.h (new)
File 4 — 31_BufferCache/00_FileBuff/src/uiox_page_cache.c (new)
File 5 — Update 32_FS/10_scfs/scfs.c — replace private page cache with uiox_pc_*
File 6 — Update 32_FS/src/uiox_fs_init.c — init page cache
File 7 — Update 31_BufferCache/Makefile
# Add to 31_BufferCache/Makefile SRCS:
SRCS += $(MFDIR)00_FileBuff/src/bcache.c
SRCS += $(MFDIR)00_FileBuff/src/uiox_page_cache.c

# Add to COMMON_CFLAGS include paths:
COMMON_CFLAGS += -I$(MFDIR)00_FileBuff/include


Complete architecture — all layers in one diagram

┌────────────────────────────────────────────────────────────────────────┐
│  USER SPACE                                                             │
│  read(fd, ubuf, n)  /  mmap(fd, ...)  /  write(fd, ubuf, n)           │
└──────────────────────────────┬─────────────────────────────────────────┘
                               │  SVC / ECALL / SYSCALL
                               ▼
┌────────────────────────────────────────────────────────────────────────┐
│  33_PCS  (process control — privilege crossing)                        │
│                                                                         │
│  sys_read()  →  copy_to_user(ubuf, kbuf, n)   ← crossing point        │
│  sys_write() →  copy_from_user(kbuf, ubuf, n) ← crossing point        │
│  sys_mmap()  →  uiox_pc_get_page_pa()          ← zero copy PA         │
│               →  uiox_mm_map_user_phys()        ← PTE insert           │
└──────────────────────────────┬─────────────────────────────────────────┘
                               │
                               ▼
┌────────────────────────────────────────────────────────────────────────┐
│  32_FS  (filesystem)                                                   │
│                                                                         │
│  vfs_read(file, kbuf, n)   →  file->f_ops->read()                     │
│  vfs_write(file, kbuf, n)  →  file->f_ops->write()                    │
│  vfs_mmap_page(file, off)  →  file->f_ops->mmap_page()                │
│  scfs_read()  /  scfs_write()  /  scfs_mmap_page()                    │
└──────────────────────────────┬─────────────────────────────────────────┘
                               │
                               ▼
┌────────────────────────────────────────────────────────────────────────┐
│  31_BufferCache/00_FileBuff  — PAGE CACHE  (new)                       │
│                                                                         │
│  uiox_pc_read(ino, offset, kbuf, len)                                  │
│    HIT:  memcpy page->data → kbuf  (DRAM → kernel buf, no device I/O) │
│    MISS: readpage() → bread() × 8 → fill page, then memcpy            │
│                                                                         │
│  uiox_pc_get_page_pa(ino, offset)                                      │
│    → ensure uptodate → return PA  (for zero-copy mmap)                │
│                                                                         │
│  uiox_pc_write(ino, offset, kbuf, len)                                 │
│    → copy kbuf → page->data, mark dirty                               │
│    → writeback on SYS_SYNC / SYS_FSYNC / pressure                     │
└──────────────────────────────┬─────────────────────────────────────────┘
                               │  on cache miss / writeback
                               ▼
┌────────────────────────────────────────────────────────────────────────┐
│  31_BufferCache/00_FileBuff  — BLOCK BUFFER CACHE  (existing ✅)       │
│                                                                         │
│  bread(dev, blkno)   → 512-byte sector from device / cache             │
│  bwrite(buf)         → write 512-byte sector to device                 │
│  bflush(dev)         → flush all delayed writes  (SYS_SYNC)           │
│  breada(dev, blkno, ra) → read + read-ahead                            │
└──────────────────────────────┬─────────────────────────────────────────┘
                               │
                               ▼
┌────────────────────────────────────────────────────────────────────────┐
│  Physical block device  (DRAM region / eMMC / SSD)                     │
│  plat_read_block() / plat_write_block()                                │
│  → override from 10_BSP for real hardware                              │
└────────────────────────────────────────────────────────────────────────┘





Complete file summary
┌─────────────────────────────────────────────────────────────────────────┐
│  FILES CREATED                                                           │
├─────────────────────────────────────────────────────────────────────────┤
│  31_BufferCache/00_FileBuff/src/bcache.c                                │
│    Implements getblk/brelse/bread/breada/bwrite/bdwrite/bflush          │
│    — the missing .c file for the existing header                        │
│                                                                          │
│  31_BufferCache/00_FileBuff/include/uiox_page_cache.h                   │
│    Page cache types + public API                                         │
│    (inode,offset) keyed, 4096-byte pages, PA exposure for mmap          │
│                                                                          │
│  31_BufferCache/00_FileBuff/src/uiox_page_cache.c                       │
│    Page cache implementation — LRU eviction, readpage (8×bread),        │
│    writepage (8×bwrite), get_page_pa for zero-copy mmap                 │
├─────────────────────────────────────────────────────────────────────────┤
│  FILES UPDATED                                                           │
├─────────────────────────────────────────────────────────────────────────┤
│  31_BufferCache/00_FileBuff/include/bcache_types.h                      │
│    BLOCK_SIZE → BCACHE_SECTOR_SIZE + BCACHE_PAGE_SIZE                   │
│    NUM_BUFFERS 16 → 256                                                  │
│    NUM_HASH_QUEUES 4 → 64                                                │
│                                                                          │
│  32_FS/10_scfs/scfs.c                                                   │
│    Replace private page cache with uiox_pc_read/write/get_page_pa       │
│    Add scfs_map_block() callback registered at mount                    │
│                                                                          │
│  32_FS/src/uiox_fs_init.c                                               │
│    Add uiox_pc_init() as step 0 (before vfs_init)                       │
│                                                                          │
│  31_BufferCache/Makefile                                                 │
│    Add bcache.c + uiox_page_cache.c to SRCS                             │
│    Add 00_FileBuff/include to COMMON_CFLAGS                             │
└─────────────────────────────────────────────────────────────────────────┘

Gap summary — what 00_FileBuff had vs what was needed:

  00_FileBuff (block buffer cache)    Page cache (now added)
  ──────────────────────────────      ──────────────────────────────
  ✅ getblk / brelse / bread          ✅ uiox_pc_read  (DRAM→kbuf)
  ✅ bwrite / bdwrite / bflush        ✅ uiox_pc_write (kbuf→DRAM)
  ✅ LRU free list + hash table       ✅ LRU eviction + hash table
  ✅ delayed write / BUF_DELWRITE     ✅ dirty page + writeback
  ✅ read-ahead (breada)              ✅ readpage (8×bread per page)
  ❌ .c file was missing              ✅ bcache.c now written
  ❌ 512-byte unit (not 4096)         ✅ 4096-byte page unit
  ❌ (dev,blkno) identity             ✅ (inode,offset) identity
  ❌ no PA exposure                   ✅ get_page_pa → zero-copy mmap
  ❌ not connected to VFS             ✅ scfs_read/write use uiox_pc_*
  ❌ NUM_BUFFERS=16 (too small)       ✅ NUM_BUFFERS=256


File 8 — 31_BufferCache/00_FileBuff/src/uiox_page_cache_sync.c (new — SYS_SYNC integration)
File 9 — 31_BufferCache/00_FileBuff/include/uiox_page_cache.h (add sync API)
File 10 — 33_PCS/src/uiox_syscall.c (update SYS_SYNC handlers)
File 11 — 33_PCS/01_schedular/src/sched.c (update — wire journal tick)
Add uiox_jr_tick() and uiox_pc_writeback_pressure() to the scheduler tick so the journal commits automatically every 5 seconds under load:
File 12 — 32_FS/10_scfs/scfs.c (update — wire close to page cache invalidate)
Add uiox_pc_invalidate() to the SCFS close path so pages are evicted when the last reference to a file is dropped:




Complete end-to-end data flow — every path from DRAM to userspace
PHYSICAL DRAM
│
│  Regions:
│  ├─ Kernel image    0x4020_0000  (.text/.data/.bss)
│  ├─ Block cache     0x46000000   (bcache — 256 × 512 B = 128 KB)
│  ├─ Page cache      0x00_FileBuff/s_pages[]  (128 × 4096 B = 512 KB)
│  ├─ Journal log     0x45000000   (256 × 4096 B = 1 MB)
│  └─ SCFS data       0x44000000   (flat filesystem image)
│
├─────────────────────────────────────────────────────────────────┐
│  PATH 1 — sys_read()  (copy — kbuf intermediate)                │
│                                                                  │
│  User:    read(fd, ubuf, 4096)                                   │
│             ↓ SVC/ECALL/SYSCALL                                  │
│  33_PCS:  sys_read(fd, ubuf, 4096)                              │
│             → uiox_uaccess_ok(ubuf, 4096)  ✓                    │
│             → fd_lookup(fd)  → file                             │
│             → vfs_read(file, kbuf, 4096)                        │
│                 → scfs_read()                                    │
│                     → uiox_pc_read(ino, pos, kbuf, 4096)       │
│                         HIT:  memcpy page->data → kbuf          │
│                         MISS: readpage()                         │
│                               → bread() × 8                     │
│                               → memcpy sectors → page->data     │
│                               → memcpy page->data → kbuf        │
│             → uiox_copy_to_user(ubuf, kbuf, n)  ← CROSSING     │
│             → return n                                           │
│             ↑ ERET/SRET/SYSRET                                   │
│  User:    ubuf now contains file data from DRAM                  │
└──────────────────────────────────────────────────────────────────┘

├─────────────────────────────────────────────────────────────────┐
│  PATH 2 — sys_mmap()  (zero copy — no kbuf at all)              │
│                                                                  │
│  User:    ptr = mmap(NULL, 4096, PROT_READ, MAP_SHARED, fd, 0)  │
│             ↓ SVC/ECALL/SYSCALL                                  │
│  33_PCS:  sys_mmap(NULL, 4096, PROT_READ, MAP_SHARED, fd, 0)   │
│             → fd_lookup(fd) → file                              │
│             → vfs_mmap_page(file, 0)                            │
│                 → scfs_mmap_page()                               │
│                     → uiox_pc_get_page_pa(ino, 0)               │
│                         ensure uptodate (readpage if needed)     │
│                         → return page->pa  (DRAM physical addr) │
│             → uiox_mm_map_user_phys(proc, 0, PA, 4096, RO)      │
│                 → insert PTE: user_VA → PA  (same DRAM page)    │
│             → return user_VA                                     │
│             ↑ ERET/SRET/SYSRET                                   │
│  User:    *ptr reads directly from DRAM — NO copy at all         │
└──────────────────────────────────────────────────────────────────┘

├─────────────────────────────────────────────────────────────────┐
│  PATH 3 — sys_write()  (copy — kbuf intermediate)               │
│                                                                  │
│  User:    write(fd, ubuf, 4096)                                  │
│             ↓ SVC/ECALL/SYSCALL                                  │
│  33_PCS:  sys_write(fd, ubuf, 4096)                             │
│             → uiox_uaccess_ok(ubuf, 4096)  ✓                    │
│             → copy_from_user(kbuf, ubuf, 4096)  ← CROSSING      │
│             → vfs_write(file, kbuf, 4096)                       │
│                 → scfs_write()  (or journal-aware write)         │
│                     → uiox_jr_start(1)  ← begin transaction     │
│                     → uiox_jr_get_write_access(h, blk)          │
│                     → uiox_pc_write(ino, pos, kbuf, 4096)       │
│                         memcpy kbuf → page->data  (DRAM write)  │
│                         page marked dirty                        │
│                     → uiox_jr_stop(h)   ← end transaction       │
│             → return n                                           │
│             ↑ ERET/SRET/SYSRET                                   │
│  [later] SYS_SYNC / timer tick:                                  │
│             → uiox_pc_sync_all()                                 │
│                 → uiox_pc_writeback_all()                        │
│                     → writepage() → bwrite() × 8                │
│                 → uiox_jr_force_commit()  (journal durable)      │
│                 → uiox_jr_checkpoint()    (data durable)         │
└──────────────────────────────────────────────────────────────────┘

├─────────────────────────────────────────────────────────────────┐
│  PATH 4 — SYS_SYNC / SYS_FSYNC                                  │
│                                                                  │
│  User:    sync() / fsync(fd)                                     │
│             ↓ SVC/ECALL/SYSCALL                                  │
│  33_PCS:  _sys_sync() / _sys_fsync()                            │
│             → uiox_pc_sync_all() / uiox_pc_sync_inode()         │
│                 → uiox_pc_writeback_all()                        │
│                     → writepage() → bwrite() → bflush()         │
│                 → uiox_jr_force_commit()                         │
│                 → uiox_jr_checkpoint()                           │
│             → return 0                                           │
│             ↑ ERET/SRET/SYSRET                                   │
│  All dirty data now durable on device                            │
└──────────────────────────────────────────────────────────────────┘



The one-line explanation for every gap that was found and fixed
WHAT WAS THERE          WHAT WAS MISSING          WHAT WAS ADDED
──────────────────────  ─────────────────────────  ──────────────────────────
00_FileBuff header      bcache.c (.c was missing)  bcache.c — all 5 getblk
  getblk/bread/bwrite     ← linker error without     scenarios, brelse,
  algorithm declared       this                       bread, breada, bwrite,
  but NOT implemented                                 bdwrite, bflush

00_FileBuff             Page cache                 uiox_page_cache.c
  512-byte block cache    (inode, offset) keyed       (inode,offset) LRU
  (dev, blkno) keyed      4096-byte pages             readpage → 8×bread
  no PA exposure          PA exposed for mmap          get_page_pa → PA
  no VFS connection       dirty tracking               writeback → 8×bwrite
                          writeback on sync

SCFS private cache      Proper page cache use      scfs_read → pc_read
  flat array in scfs.c    connected to VFS           scfs_mmap → pc_get_pa
  not reusable            + block cache              scfs_map_block callback

VFS                     Entire VFS layer missing   uiox_vfs.h + vfs.c
  uiox_fs_init.c stubs    path_lookup, open/close    open/close/read/write
                          read/write/stat/readdir     stat/mkdir/readdir
                          mmap_page                   mmap_page vtable

Journal                 Missing (moved from        uiox_journal.c
  50_UIX moved to         50_UIX but never written)  WAL, commit, checkpoint
  32_FS but no .c                                     recovery, auto-tick

Syscall dispatch        uiox_sys_call_init()       uiox_sys_call_init.c
  called but not written  never implemented           fd_init + fs_init

copy_to/from_user       Entirely missing           uiox_uaccess.c
  no crossing primitive   no VA validation            uiox_uaccess_ok()
                          no safe copy                copy_to/from_user

sys_read/write          Missing entirely           uiox_sys_fd.c
  sys_open/close          no FS syscall path          copy_from_user
  not written             no fd table                 → vfs_write
                                                      vfs_read
                                                      → copy_to_user

sys_mmap                Missing entirely           uiox_sys_mmap.c
  no zero-copy path       no PTE insertion            vfs_mmap_page → PA
                          no file-backed mmap         → mm_map_user_phys

SYS_SYNC → page cache   Not connected              uiox_page_cache_sync.c
  bflush existed but       journal not called          sync_all:
  page cache writeback     page writeback missing       pc_writeback_all
  not wired to sync                                     jr_force_commit
                                                        jr_checkpoint

Arch syscall entry      arch_syscall_entry()       Added to all 4 arches
  VBAR/stvec/LSTAR set    never called               arm64: SVC/EL0→EL1
  but vector stub          uiox_syscall_dispatch       arm32: SVC/USR→SVC
  did not call dispatch    never reached               riscv64: ECALL/stvec
                                                       x86_64: SYSCALL/LSTAR

libc in algorithms      -ffreestanding violation   Fix all 11 files:
  00_IPC, 50_scps,        won't compile under         #include <string.h>
  01_schedular,           kernel build flags           → uiox_soc_string.h
  02_MemMngnt all use                                  malloc → uiox_kmalloc
  malloc/free/memcpy                                   free   → uiox_kfree


Build order dependency — which must compile before which
Compile order (bottom → top):

  1. 02_FwHal          → libuioxfwarm64.a
  2. 10_BSP            → libbsp.a
  3. 20_DriverInterfaces → libhw.a
  4. 30_DeviceDrivers  → libdev.a
  5. 31_BufferCache    → libbcache.a
       00_FileBuff/bcache.c
       00_FileBuff/uiox_page_cache.c
       00_FileBuff/uiox_page_cache_sync.c
       01_DrvBuff/*.c
  6. 32_FS             → libfs.a
       src/uiox_fs_init.c
       01_fsa/vfs.c
       10_scfs/scfs.c      ← needs uiox_page_cache.h from step 5
       02_journal/uiox_journal.c
  7. 33_PCS            → libpcs.a
       src/uiox_uaccess.c
       src/uiox_syscall.c
       src/uiox_sys_fd.c   ← needs uiox_vfs.h from step 6
       src/uiox_sys_mmap.c ← needs uiox_page_cache.h from step 5
       src/uiox_ioctl_soc.c
       src/uiox_sys_call_init.c ← calls uiox_fs_init from step 6
       02_MemMngnt/mm.c
       01_schedular/sched.c ← calls uiox_jr_tick from step 6
       00_IPC/*.c
       50_scps/*.c
       03_ksign/uiox_ksign.c
       04_fboot/uiox_fboot.c
       05_sec/uiox_sec.c
       06_kpatch/uiox_kpatch.c
  8. 34_CAS            → libcas.a
  9. 50_UIX/01_shell   → libshell.a
  10. uiox_kernel_main.c → uiox_kernel.elf
