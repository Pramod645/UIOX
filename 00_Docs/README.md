Here is the complete UIOX Developer Documentation — accurate to the actual repository structure.

UIOX Developer Documentation

Version 1.0 — July 2026
Repository: https://github.com/Pramod645/UIOX

Table of Contents
What is UIOX?
Quick Start — Build in 5 Minutes
Repository Layout
Layer Architecture
Setting Up Your Development Environment
Build System
Running UIOX in QEMU
Adding a New Device Driver
Adding a New System Call
Writing a UIOX Application
Debugging
Coding Standards
Testing
Contributing
Frequently Asked Questions

What is UIOX?

UIOX is a from-scratch operating system written in C targeting three hardware architectures simultaneously:

| Architecture | Toolchain | QEMU machine |
|---|---|---|
| ARM64 (AArch64 ARMv8-A) | aarch64-linux-gnu-gcc | virt |
| ARM32 (ARMv7-A) | arm-linux-gnueabihf-gcc | versatilepb |
| x86-64 | gcc (native) | q35 |

UIOX is structured as a layered stack, where each numbered directory is a distinct software layer. The numbering enforces dependency direction: higher-numbered layers may use lower-numbered ones, but never the reverse.

``
50UIX           ← Applications / Shell / User Programs
40SystemCallInterface  ← syscall dispatch table
34CAS           ← Context / Atomics / Synchronisation
33ProcessControlSubsystem ← Scheduler / IPC / Memory
32FileSystem    ← VFS + SCFS filesystem
31BufferCache   ← Block buffer cache
30DeviceDrivers ← Character and block drivers
20DriverInterfaces ← Hardware abstraction interfaces
10Arch          ← CPU / SoC layer (ARM64 / ARM32 / x86-64)
02FwHal         ← Firmware / HAL
01uBoot         ← Bootloader
`

Quick Start
Prerequisites

`bash
macOS
brew install make aarch64-elf-gcc arm-none-eabi-gcc gcc qemu

Ubuntu / Debian
sudo apt install build-essential \
    gcc-aarch64-linux-gnu gcc-arm-linux-gnueabihf \
    qemu-system-arm qemu-system-x86 gdb-multiarch
`

Clone and build

`bash
git clone https://github.com/Pramod645/UIOX.git
cd UIOX

Build all three architectures
make all

Build only ARM64
make ARCH=arm64

Build only ARM32
make ARCH=arm32

Build only x86-64
make ARCH=x8664
`

Run in QEMU

`bash
ARM64
make qemuarm64

ARM32
make qemuarm32

x86-64
make qemux8664
`

You should see UIOX boot output on the terminal. Press Ctrl-A X to quit QEMU.

Repository Layout

`
UIOX/
│
├── 00Docs/                 Documentation and design notes
│
├── 01uBoot/                Bootloader (freestanding C + Assembly)
│   ├── include/             Bootloader headers
│   ├── src/
│   │   ├── arch/
│   │   │   ├── arm64/       ARM64 entry stub + HW ops
│   │   │   ├── arm32/       ARM32 entry stub + HW ops
│   │   │   └── x8664/      x86-64 entry stub + HW ops
│   │   └── .c              Common bootloader modules
│   └── linker/              Per-arch linker scripts
│
├── 02FwHal/                Firmware / Hardware Abstraction Layer
│   ├── include/             Firmware headers
│   └── src/                 Platform init, POST, clock, power
│
├── 10Arch/                 Architecture-specific CPU / SoC layer
│   ├── arm64/
│   │   ├── include/archdefs.h    GIC, PL011, DAIF, barrier macros
│   │   └── src/archinit.c        GIC-400, UART, timer init
│   ├── arm32/
│   │   ├── include/archdefs.h
│   │   └── src/archinit.c
│   └── x8664/
│       ├── include/archdefs.h    LAPIC, IOAPIC, 8259A, PIT, COM1
│       └── src/archinit.c
│
├── 20DriverInterfaces/     Hardware abstraction interface contracts
│   ├── include/             hwtypes.h, mmio.h, irq.h, cpu.h, dma.h
│   └── src/                 Interface implementations
│
├── 30DeviceDrivers/        Concrete device driver implementations
│   ├── 00character/        UART, TTY, PTY, null, zero
│   ├── 01block/            IDE, VirtIO-blk, RAM disk
│   ├── include/             Driver headers
│   └── src/                 Shared driver infrastructure
│
├── 31BufferCache/          Block-level buffer cache (bread/bwrite)
│   ├── include/
│   └── src/
│
├── 32FileSystem/           Virtual File System + concrete filesystems
│   ├── 01fsa/              File system access layer
│   ├── 10scfs/             UIOX Simple Connected File System (SCFS)
│   ├── VirtualFileSystem.h  VFS interface
│   └── Filesystems.h
│
├── 33ProcessControlSubsystem/  Scheduler, IPC, memory management
│   ├── 00inter-process-communication/
│   ├── 01schedular/
│   ├── 02memory-managment/
│   ├── 40procStruct/
│   └── 50scps/
│
├── 34CAS/                  Context switch, atomics, synchronisation
│
├── 40SystemCallInterface/  System call dispatch table
│   ├── uixsys.h            syscall numbers and prototypes
│   └── uixarchSysCall.h    per-arch syscall entry
│
├── 50UIX/                  Userspace / applications
│   ├── 00libs/             Standard C-like library stubs
│   ├── 01shell/            UIOX interactive shell
│   ├── 20uios/             OS services (thermal, pkg manager, …)
│   └── 21apps/             Sample applications
│
├── 70buildconfig/         Build system fragments
│   ├── common.mk            Shared CFLAGS, include paths
│   ├── arm64.mk             ARM64 toolchain + QEMU target
│   ├── arm32.mk             ARM32 toolchain + QEMU target
│   ├── x8664.mk            x86-64 toolchain + QEMU target
│   └── tools.mk             Toolchain auto-detection
│
├── 71linker/               Linker scripts for each arch + component
│   ├── uioxarm64.ld
│   ├── uioxarm32.ld
│   └── uioxx8664.ld
│
├── 90CnL/                  UIOX Compiler (uioxcc) and Linker (uioxld)
│
├── Makefile                 Top-level build entry point
├── main.c                   Integration main — boots the full stack
└── uiox.md                  Per-file purpose reference
`

Layer Architecture
Dependency rule

`
Layer N may #include headers from Layer N-1 and below.
Layer N must NEVER include headers from Layer N+1 or above.
`

This is enforced by the numbered prefix. If you are writing code in 30DeviceDrivers you may include from 20DriverInterfaces and 10Arch, but never from 40SystemCallInterface.

Data flow at boot

`
Power-on
    │
    ▼
01uBoot: start (arch entry stub)
    │  Sets up EL1/SVC/long-mode, zeroes BSS, calls uioxbootmain()
    │
    ▼
01uBoot: uioxbootmain()
    │  Stage 1: HW init (UART up)
    │  Stage 2: Memory map probe (DTB / ATAG / E820)
    │  Stage 3–7: Storage, FAT32, ELF load, verify SHA-256
    │  Stage 8: jump to uioxfwmain()
    │
    ▼
02FwHal: uioxfwmain()
    │  Stage 1: Platform archregister() → UART, GIC/PIC, clocks
    │  Stage 2: Memory map
    │  Stage 3: IRQ manager
    │  Stage 4: Timers (SP804 / PIT / ARM-GT at 100 Hz)
    │  Stage 5: GPIO
    │  Stage 6: Storage (block device registration)
    │  Stage 7: Device switch table (devsw)
    │  Stage 8: → uioxkernelmain()
    │
    ▼
10Arch + 20..50: uioxkernelmain()
    │  archinit()         — platform HW init
    │  bufinit()          — buffer cache
    │  inodecacheinit()  — inode cache
    │  sbinit()           — superblock
    │  fsmkfs()           — format filesystem
    │  clistinit()        — character device lists
    │  devswinit()        — device switch table
    │  ttyinit()          — TTY layer
    │  schedulerinit()    — process scheduler
    │  syscallinit()      — syscall dispatch table
    │  userinit()         — first user process
    │
    ▼
50UIX: Shell / Applications
`

Development Environment
Recommended tools

| Tool | Purpose | Install |
|---|---|---|
| aarch64-linux-gnu-gcc | ARM64 cross-compiler | apt install gcc-aarch64-linux-gnu |
| arm-linux-gnueabihf-gcc | ARM32 cross-compiler | apt install gcc-arm-linux-gnueabihf |
| gcc | x86-64 native compiler | built-in |
| qemu-system-aarch64 | ARM64 emulator | apt install qemu-system-arm |
| qemu-system-arm | ARM32 emulator | (same package) |
| qemu-system-x8664 | x86-64 emulator | apt install qemu-system-x86 |
| gdb-multiarch | Debugger for all arches | apt install gdb-multiarch |
| ctags | Code navigation | apt install universal-ctags |

Check your toolchains

`bash
make checktools
`

Expected output:

`
=== UIOX Toolchain Check ===
ARM64  CC : aarch64-linux-gnu-gcc  ... OK (gcc 13.x)
ARM32  CC : arm-linux-gnueabihf-gcc ... OK (gcc 13.x)
x8664 CC : gcc                    ... OK (gcc 13.x)
===========================
`

IDE setup

For VS Code, add this to .vscode/ccppproperties.json:

`json
{
    "configurations": [
        {
            "name": "UIOX ARM64",
            "includePath": [
                "${workspaceFolder}/10Arch/arm64/include",
                "${workspaceFolder}/20DriverInterfaces/include",
                "${workspaceFolder}/30DeviceDrivers/include",
                "${workspaceFolder}/31BufferCache/include",
                "${workspaceFolder}/32FileSystem",
                "${workspaceFolder}/33ProcessControlSubsystem",
                "${workspaceFolder}/40SystemCallInterface",
                "${workspaceFolder}/50UIX/00libs"
            ],
            "defines": [
                "UIOXARCHARM64=1",
                "UIOXBITS=64"
            ],
            "compilerPath": "/usr/bin/aarch64-linux-gnu-gcc",
            "cStandard": "c99"
        }
    ]
}
`

Build System
Top-level targets

`bash
make                   # build all three architectures
make ARCH=arm64        # build ARM64 only
make ARCH=arm32        # build ARM32 only
make ARCH=x8664       # build x86-64 only
make clean             # remove all build artefacts
make checktools       # verify toolchain availability
make qemuarm64        # build ARM64 + launch QEMU
make qemuarm32        # build ARM32 + launch QEMU
make qemux8664       # build x86-64 + launch QEMU
`

Build outputs

`
build/
├── arm64/
│   ├── uioxarm64.elf   — ELF with debug symbols
│   └── uioxarm64.bin   — flat binary for QEMU -kernel
├── arm32/
│   ├── uioxarm32.elf
│   └── uioxarm32.bin
└── x8664/
    ├── uioxx8664.elf
    └── uioxx8664.bin
`

How the build works

The top-level Makefile reads 70buildconfig/tools.mk (auto-detects toolchains) then includes the appropriate arch .mk file. Each arch .mk file:

Sets CC, AR, LD, OBJCOPY for the target toolchain
Sets CFLAGS including -march=, -DUIOXARCH, and all include paths
Globs source files from every subsystem directory
Derives object paths under build/$(ARCH)/
Links with the arch-specific linker script from 71linker/

Adding a source file

Simply place a .c file anywhere under its correct numbered subsystem directory. The wildcard globs in 70buildconfig/common.mk pick it up automatically on the next make.

Running UIOX in QEMU
ARM64

`bash
qemu-system-aarch64 \
    -machine virt \
    -cpu cortex-a53 \
    -m 64M \
    -nographic \
    -serial mon:stdio \
    -kernel build/arm64/uioxarm64.bin
`

What you will see:

`
============================================
  UIOX Firmware v1.0
============================================
Stage 1: HW init  arch=ARM64
Stage 2: Memory
  Memory map (1 regions):
    base=0000000040000000  size=0000000004000000  USABLE
  Usable: 64 MB
Stage 3: IRQ
Stage 4: Timers
  Timer: 100 Hz tick
...
[UIOX] kernelmain entered
`

ARM32

`bash
qemu-system-arm \
    -machine versatilepb \
    -cpu arm926 \
    -m 16M \
    -nographic \
    -serial mon:stdio \
    -kernel build/arm32/uioxarm32.bin
`

x86-64

`bash
qemu-system-x8664 \
    -machine q35 \
    -m 64M \
    -nographic \
    -serial mon:stdio \
    -kernel build/x8664/uioxx8664.bin
`

Quit QEMU

Press Ctrl-A then X.

Connect a GDB debugger

Add -S -gdb tcp::1234 to the QEMU command, then in another terminal:

`bash
gdb-multiarch build/arm64/uioxarm64.elf
(gdb) target remote :1234
(gdb) break archinit
(gdb) continue
`

Adding a Device Driver

This walkthrough adds a fictional uioxmydev character driver.

Step 1 — Create the header in 20DriverInterfaces/include/

`c
/ 20DriverInterfaces/include/uioxmydevif.h /
#ifndef UIOXMYDEVIFH
#define UIOXMYDEVIFH
#include "hwtypes.h"

/ Driver ops vtable /
typedef struct {
    int  (init)  (void);
    void (deinit)(void);
    int  (read)  (uint8t buf, uint32t len);
    int  (write) (const uint8t buf, uint32t len);
} uioxmydevopst;

/ Register your driver implementation /
void uioxmydevregister(const uioxmydevopst ops);

#endif / UIOXMYDEVIFH /
`

Step 2 — Implement in 30DeviceDrivers/00character/

`c
/ 30DeviceDrivers/00character/uioxmydev.c /
#include "uioxmydevif.h"
#include <stdio.h>

static const uioxmydevopst gops;

void uioxmydevregister(const uioxmydevopst ops)
{
    gops = ops;
    if (gops && gops->init)
        gops->init();
    printf("[mydev] registered\n");
}

int uioxmydevread(uint8t buf, uint32t len)
{
    if (!gops || !gops->read) return -1;
    return gops->read(buf, len);
}

int uioxmydevwrite(const uint8t buf, uint32t len)
{
    if (!gops || !gops->write) return -1;
    return gops->write(buf, len);
}
`

Step 3 — Provide a platform implementation in 10Arch/

`c
/ 10Arch/arm64/src/mydevarm64.c /
#include "uioxmydevif.h"
#include "archdefs.h"

static int mydevinit(void)
{
    / configure MMIO registers /
    return 0;
}

static int mydevread(uint8t buf, uint32t len)
{
    (void)buf; (void)len;
    return 0;
}

static int mydevwrite(const uint8t buf, uint32t len)
{
    (void)buf; (void)len;
    return 0;
}

static const uioxmydevopst mydevops = {
    .init  = mydevinit,
    .read  = mydevread,
    .write = mydevwrite,
};

void mydevarm64init(void)
{
    uioxmydevregister(&mydevops);
}
`

Step 4 — Register in the device switch table

In main.c or the arch archinit.c, add:

`c
#include "uioxmydevif.h"

/ inside archinit() or uioxkernelmain() /
mydevarm64init();
`

Step 5 — Expose via system call (optional)

Add an entry to 40SystemCallInterface/uixsys.h:

`c
#define SYSMYDEVREAD   200
#define SYSMYDEVWRITE  201
`

And register a handler in the syscall dispatch table.

Adding a System Call
Step 1 — Assign a number

In 40SystemCallInterface/uixsys.h:

`c
#define SYSMYNEWCALL  100   / pick an unused number /
`

Step 2 — Write the handler

`c
/ 40SystemCallInterface/src/sysmynewcall.c /
#include "uixsys.h"

long sysmynewcall(long arg0, long arg1, long arg2)
{
    (void)arg2;
    / implement your syscall logic here /
    return arg0 + arg1;
}
`

Step 3 — Register in the dispatch table

In 40SystemCallInterface/uixarchSysCall.h add your handler to the function pointer table:

`c
[SYSMYNEWCALL] = sysmynewcall,
`

Step 4 — Call from userspace

`c
/ 50UIX/21apps/myapp.c /
#include "uixsys.h"

long result = syscall(SYSMYNEWCALL, 10, 20, 0);
`

Writing a UIOX Application

Applications live in 50UIX/21apps/. They are linked into the kernel binary as part of the init process until a proper ELF loader is available.

Minimal application

`c
/ 50UIX/21apps/hello.c /
#include <stdio.h>

int hellomain(void)
{
    printf("Hello from UIOX!\n");
    return 0;
}
`

Register your app in main.c

`c
extern int hellomain(void);

/ inside the integration test section of main.c /
hellomain();
`

Using the UIOX shell

The shell lives in 50UIX/01shell/. To add a built-in command:

`c
/ 50UIX/01shell/commands.c /
static int cmdhello(int argc, char argv[])
{
    (void)argc; (void)argv;
    printf("Hello, UIOX!\n");
    return 0;
}

/ Register in the command table: /
{ "hello", cmdhello, "print a greeting" },
`

Debugging
Serial output

All UIOX debug output goes to UART0. The QEMU -nographic -serial mon:stdio flags route it to your terminal.

GDB workflow

`bash
Terminal 1 — run QEMU with GDB stub
qemu-system-aarch64 \
    -machine virt -cpu cortex-a53 -m 64M \
    -nographic -serial mon:stdio \
    -kernel build/arm64/uioxarm64.bin \
    -S -gdb tcp::1234

Terminal 2 — connect GDB
gdb-multiarch build/arm64/uioxarm64.elf
(gdb) target remote :1234
(gdb) set architecture aarch64
(gdb) break uioxkernelmain
(gdb) continue
(gdb) info registers
(gdb) x/20i $pc      # disassemble current position
(gdb) bt             # backtrace
`

Useful GDB commands for UIOX

| Command | What it shows |
|---|---|
| info registers | All CPU registers |
| x/20xw 0x40000000 | Memory dump at address |
| p gcpuid | Print CPU ID struct |
| break archinit | Break at platform init |
| break uioxfwmain | Break at firmware entry |
| break syswrite | Break at write syscall |

Adding debug prints

Use the firmware printf which works before libc is available:

`c
/ Available in 02FwHal and above /
uioxfwprintf("value = %u  addr = 0x%08x\n", val, addr);

/ Available from 20DriverInterfaces and above /
printf("[mymodule] initialised at 0x%llx\n", (unsigned long long)base);
`

Checking the build log

`bash
make ARCH=arm64 2>&1 | head -50   # see first 50 lines of build
make ARCH=arm64 V=1                # verbose build (shows exact commands)
`

Coding Standards

UIOX follows a consistent C99 style. These rules apply to all layers.

Naming

| Thing | Convention | Example |
|---|---|---|
| Files | uiox<subsystem><module>.c | uioxfwuart.c |
| Types | snakecaset | uioxfwuartt |
| Functions | uiox<module><verb>() | uioxfwuartinit() |
| Macros | UIOX<MODULE><NAME> | UIOXFWUARTBAUD |
| Struct members | snakecase | uart->baudrate |
| Arch-specific | prefix with arch | arm64uartputc() |

Types
• Use uint8t, uint16t, uint32t, uint64t — never int for hardware values
• Use bool + true/false for boolean fields
• Define your own types with typedef struct { … } mytypet;
• Never use char  for binary data — use uint8t 

Memory
• No heap (malloc) in layers 01–20. Use static storage only.
• Layers 30 and above may use a simple bump allocator or the kernel slab allocator.
• Always check pointer arguments: if (!ptr) return -EINVAL;
• Zero-initialise structs: memset(&s, 0, sizeof(s));

ARM32 special rule — no 32-bit division operators

ARM926 / Cortex-A9 have no hardware divide instruction. Using / or % on any integer generates a call to aeabiuidivmod which lives in libgcc (not linked in freestanding builds). Instead, use the software divide helper:

`c
/ In any file that targets ARM32 bare-metal /
static uint32t udiv32(uint32t n, uint32t d, uint32t rem)
{
    uint32t q = 0, r = 0;
    for (int i = 31; i >= 0; i--) {
        r = (r << 1) | ((n >> (uint32t)i) & 1);
        if (r >= d) { r -= d; q |= 1u << (uint32t)i; }
    }
    if (rem) rem = r;
    return q;
}
`

Header guards

Every header must have:

`c
#ifndef UIOXMYHEADERH
#define UIOXMYHEADERH
/ content /
#endif / UIOXMYHEADERH /
`

C++ compatibility

Shared headers that may be included from C++ code must wrap declarations:

`c
#ifdef _cplusplus
extern "C" {
#endif

/ declarations /

#ifdef cplusplus
}
#endif
`

Error handling

All functions that can fail return int (0 = success, negative = error) or a typed errt. Never return a raw boolean for error conditions.

`c
/ Good /
int rc = uioxfwuartinit(&uart, base, true, irq, &cfg);
if (rc != 0) {
    uioxfwprintf("uartinit failed: %d\n", rc);
    return rc;
}

/ Bad /
if (!uioxfwuartinit(&uart, base, true, irq, &cfg)) { … }
`

Testing
Integration test

The main.c file at the repository root is an 8-stage integration test. It boots the full stack from archinit() through filesystem, device drivers, TTY, IRQ, DMA, context switch, PTY, sync, and teardown:

`bash
make ARCH=arm64
make qemuarm64
Watch the output — all 8 stages should print OK
`

Unit testing individual subsystems

Each subsystem has a main.c or demo.c that can be compiled standalone:

`bash
Test the thermal subsystem
cd 50UIX/20uios/thermal
make
./build/uioxthermdemo

Test the package manager
cd 50UIX/21apps/uioxpkg
make test
./build/uiox-pkg stats
./build/uiox-pkg list
`

Checking for undefined references

After adding new code, always verify the ARM32 build links cleanly since it is the strictest (no libgcc, no hardware divide):

`bash
make ARCH=arm32 2>&1 | grep "undefined reference"
`

If you see aeabi symbols, you have a division operator in bare-metal ARM32 code. See the coding standards section for the fix.

Contributing
Workflow

`
Fork https://github.com/Pramod645/UIOX
Create a feature branch: git checkout -b feature/my-driver
Write code following the coding standards above
Verify all three architectures build: make all
Run the integration test on at least ARM64 and x86-64
Submit a pull request with a clear description
`

Pull request checklist
• [ ] make all succeeds with zero errors
• [ ] No new -Wcast-function-type warnings
• [ ] No new _aeabi undefined references on ARM32
• [ ] New .c files placed in the correct numbered layer directory
• [ ] New headers have include guards
• [ ] Public functions have / @brief ... / doc comments
• [ ] No malloc() / free() in layers 01–20
• [ ] Code follows the naming conventions

Where to add new things

| What you are adding | Where it goes |
|---|---|
| New CPU / SoC support | 10Arch/ — new subdirectory |
| New hardware driver | 30DeviceDrivers/ |
| New hardware abstraction interface | 20DriverInterfaces/include/ |
| New filesystem | 32FileSystem/ |
| New IPC mechanism | 33ProcessControlSubsystem/00inter-process-communication/ |
| New system call | 40SystemCallInterface/ |
| New shell command | 50UIX/01shell/ |
| New OS service | 50UIX/20uios/ |
| New application | 50UIX/21apps/ |
| Build system change | 70buildconfig/ |
| Linker script change | 71linker/ |

FAQ

Q: Why does the ARM32 build fail with undefined reference to _aeabiuidiv?

A: You have a % or / operator on an integer in bare-metal ARM32 code. ARM926/Cortex-A9 has no hardware divide. Replace all integer division with the udiv32() software helper shown in the coding standards. Also add $(LIBGCCARM32) to the ARM32 link rule in your Makefile as a safety net.

Q: Why are there duplicate #define PL011FRTXFF warnings?

A: The same macro is defined in both uioxfwuart.h (the shared header, correct location) and a per-arch header. Delete the definitions from the arch header and keep only the one in the shared header. The shared header is the single source of truth.

Q: What does cannot find entry symbol start mean?

A: The firmware linker script uses ENTRY(start) but no assembly stub exports a start symbol. Each arch needs an entry stub (e.g. uioxfwentryarm64.S) that exports .global start, sets up the stack, zeroes BSS, and calls uioxfwmain().

Q: How do I add support for a new ARM64 board?

A: Create 10Arch/arm64/include/archdefsmyboard.h with your board's UART base, GIC base, IRQ numbers, and memory map. Then create 10Arch/arm64/src/archinitmyboard.c that implements uioxfwarchregister() and archinit() for your board. Select it at build time with make ARCH=arm64 BOARD=myboard.

Q: The build says Resource temporarily unavailable on macOS.

A: macOS ships BSD make (from Xcode), which hits a process limit when $(MAKE) is called recursively. Fix: brew install make then use gmake instead of make, or add export PATH="$(brew --prefix make)/libexec/gnubin:$PATH" to your shell profile.

Q: Where is the kernel entry point?

A: The C kernel entry point is uioxkernelmain() in main.c at the repository root. It is called by uioxfwmain() in 02FwHal/ after all firmware stages complete. Before that, the arch-specific assembly entry stub in 01uBoot/src/arch/<arch>/ is the very first code that runs on the CPU.

Q: How do I add a new architecture (e.g. RISC-V)?

A: Follow these steps:

Create 10Arch/riscv64/include/archdefs.h with CSR addresses, PLIC base, CLINT base, UART base, IRQ numbers
Create 10Arch/riscv64/src/archinit.c implementing archinit() and uioxfwarchregister()
Add 01uBoot/src/arch/riscv64/uioxbootentryriscv.S with a start stub
Add 70buildconfig/riscv64.mk selecting riscv64-linux-gnu-gcc
Add 71linker/uioxriscv64.ld with the correct memory map
Add ARCH=riscv64 handling to the top-level Makefile

Q: Can I use C++ in UIOX?

A: Yes, in the upper layers (50UIX and above). The 90CnL compiler is written in C++17. Layers 01–40 must remain in C99 for freestanding compatibility. Use extern "C" wrappers at C/C++ boundaries.

Q: How do I format the code?

`bash
Format all .c and .h files with clang-format
find . -name ".c" -o -name ".h" | \
    grep -v build | \
    xargs clang-format -i --style="{BasedOnStyle: LLVM, IndentWidth: 4}"
`

This documentation is maintained in 00Docs/. To suggest improvements, open a pull request against the main` branch.