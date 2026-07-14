Let me fetch the UIOX repository to understand the MMU/TLB implementation across all modules before updating the document.
Let me fetch all the MMU/TLB files from the repository first.
Let me fetch the remaining MMU/TLB files and the arch-specific page table implementations before updating the document.
Let me fetch the remaining MMU/TLB files before building the document.
Let me fetch the remaining source files before building the updated document.
I have all the content needed. Now building the updated document with the new MMU/TLB section added.
I have all the content from the repository. Now building the updated document with the new MMU/TLB section added to the existing ISA reference.
I have all the UIOX MMU/TLB content. Let me build the updated document by appending the new MMU/TLB section to the existing build script.
Now I have everything. Let me write the updated build script with the complete MMU/TLB section added as §8, then regenerate all three files.
Running the MMU/TLB document build now.
Here is the updated document with the complete MMU/TLB section:

uioxmmutlbreference.docx

Here is a summary of everything covered in the new §8 MMU/TLB section, mapped directly to the UIOX source files:

§8 — MMU / TLB Reference: What's Covered
8.1 Core Concepts
The MMU sits between the CPU and memory bus, translating every virtual address (VA) to a physical address (PA). Three outcomes: isolation (separate address spaces per process), protection (hardware-enforced R/W/X/U bits), and flexibility (physical memory can be fragmented while presenting a contiguous virtual view).

The full VA → PA translation flow is documented step by step — TLB hit fast path, TLB miss → page table walk, valid/invalid entry, permission check, and final bus routing.

8.2 Page Table Structure
Drawn from pagefault.h and PhyMemLayout.h:

| Flag | Meaning |
|---|---|
| PTEVALID | Page present in RAM |
| PTEWRITE | Writable |
| PTEUSER | User-mode accessible |
| PTECOW | Copy-on-write |
| PTEDEMANDZERO | Zero-fill on first access (BSS/stack) |
| PTEINSWAP | Page is on swap device |

The five-state page model (On Swap, Free List, In Exec File, Demand Zero, In Core) and the ptet struct are fully documented.

8.3 Page Fault Handling (pagefault.c)
Two algorithms:
• vfault() — validity fault: allocates a physical frame, reads from swap/file/zero-fills, sets PTEVALID, wakes waiting processes
• pfault() — protection fault: COW copy-then-remap, or SIGSEGV if no COW

8.4 TLB Management
Why software flushes are needed, all six flush operations (flushtlball, flushtlbmm, flushtlbrange, flushtlbpage, etc.) with the exact instruction for each architecture, plus ready-to-use inline assembly patterns for all four targets.

8.5 Architecture Page Table Formats

| Arch | Levels | VA bits | Enable register | Fault addr |
|---|---|---|---|---|
| ARM64 | 4 (PGD→PUD→PMD→PTE) | 48-bit | SCTLREL1[0] | FAREL1 |
| ARM32 | 2 (Short Desc) | 32-bit | SCTLR[0] via CP15 | DFAR |
| x86-64 | 4 (PML4→PDPT→PD→PT) | 48-bit | CR0[31] PG bit | CR2 |
| RISC-V | 3/4 (Sv39/Sv48) | 39/48-bit | satp CSR MODE | stval |

8.6 Physical Memory Layout
The PhyMemLayout.h address map across all four architectures — DDR, MMIO, UART, VirtIO, kernel .text/.data, boot-args struct — all in one table.

8.7 Virtual Memory Syscalls (mm.c)
kernelmmap, kernelmunmap, kernelmprotect, kernelbrk — with PROT and MAP flag tables.

8.8 Swapper (swapper.c)
The full swap-out/swap-in algorithm: candidate scoring (priority + residencetime), pickswapoutcandidate, swapinprocess, wakeupswapper on memory pressure below 25%.

8.9 Cross-Architecture MMU/TLB Summary
One comparison table across all four architectures covering page levels, huge pages, ASID width, MMU enable, page table base, fault address register, and both full and single-page TLB flush instructions.

8.10 Source File Map
Every relevant file in the project mapped to its role — 17 files across 33ProcessControlSubsystem/, 10Arch/, 02FwHal/, and 33ProcessControlSubsystem/14sec/.
=================

Here is a thorough hardware-level description of how the MMU works — physical circuits, pipeline integration, and the TLB microarchitecture — across all four UIOX target architectures.

MMU Hardware Details
1 · What the MMU Is Physically

The MMU is a dedicated hardware block integrated inside the CPU core pipeline, not an external chip. On modern SoCs it sits between the execute stage and the L1 data / instruction cache, intercepting every memory reference before it reaches the cache or the memory bus.

``
┌──────────────────────────────────────────────────────────────┐
│  CPU Core Pipeline                                           │
│                                                              │
│  Fetch → Decode → Execute → ┌─────────────┐ → L1 Cache     │
│                              │   MMU / TLB │                │
│                              │  (VA → PA)  │ → L2 Cache    │
│                              └─────────────┘ → SoC Bus     │
│                                    ↑                        │
│                              Page Table Walker              │
│                              (hardware state machine)       │
└──────────────────────────────────────────────────────────────┘
`

2 · TLB — Translation Lookaside Buffer (Hardware Cache)

The TLB is a small, fully-associative or set-associative SRAM cache inside the MMU. It stores recently used VA→PA mappings so the CPU does not have to walk the page table in RAM on every access.

2.1 Physical Structure

| Property | Typical Value | Notes |
|---|---|---|
| Type | Fully-associative (small) or set-associative (larger) | Fully-assoc gives best hit rate; set-assoc scales |
| Entries | 48–2048 entries | Cortex-A76: L1-ITLB 48 entries, L1-DTLB 48 entries, L2-TLB 2048 entries |
| Entry width | ~100–130 bits | Holds: VA tag + PA + ASID + flags (Valid/Write/User/XN/NG) |
| Access latency | 1–2 cycles | Parallel lookup; all entries compared simultaneously |
| Replacement policy | Pseudo-LRU or random | Hardware-managed; software cannot control it |
| ASID field | 8–16 bits | Allows multiple processes' translations to coexist without full flush |

2.2 TLB Entry Fields

`
┌──────┬────────┬────────────────┬──────────┬───────────────────────┐
│ ASID │  VPN   │      PPN       │  Flags   │      Description      │
│ 16b  │  36b   │     36b        │   8b     │                       │
├──────┼────────┼────────────────┼──────────┼───────────────────────┤
│ 0x03 │ 0x4008 │ 0x8001         │ V+R+W+U  │ User data page        │
│ 0x00 │ 0xFFFF │ 0x0010         │ V+R+X+G  │ Kernel text (Global)  │
└──────┴────────┴────────────────┴──────────┴───────────────────────┘
VPN = Virtual Page Number  |  PPN = Physical Page Number
`

2.3 Two-Level TLB Hierarchy (Modern Cores)

Most modern CPUs implement a two-level TLB hierarchy — exactly like the L1/L2 cache hierarchy:

`
L1 ITLB (Instruction)        L1 DTLB (Data)
  48–64 entries                48–64 entries
  Fully associative            Fully associative
  1 cycle latency              1 cycle latency
         ↓ miss                       ↓ miss
         └───────────┬────────────────┘
                     ↓
              L2 Unified TLB
                2048 entries
                8-way set-associative
                ~5 cycle latency
                     ↓ miss
              Hardware Page Table Walker
                ~50–200 cycle latency (DRAM walk)
`

3 · Hardware Page Table Walker

On a TLB miss the MMU does not trap to software. The hardware page table walker — a dedicated state machine inside the MMU — performs the multi-level walk autonomously in background while the pipeline stalls (or speculatively continues on out-of-order cores).

3.1 Walk State Machine

`
TLB Miss Detected
      │
      ▼
Read TTBR0/TTBR1 (ARM64) │ CR3 (x86) │ satp (RISC-V)
      │  (base PA of root page table)
      ▼
Level 0 / PML4 / PGD Read from L1/L2/L3 cache or RAM
      │  PA = base + VA[47:39] × 8
      ▼
Level 1 / PDPT / PUD Read
      │  PA = L0.PPN + VA[38:30] × 8
      ▼
Level 2 / PD / PMD Read
      │  PA = L1.PPN + VA[29:21] × 8
      ▼
Level 3 / PT / PTE Read
      │  PA = L2.PPN + VA[20:12] × 8
      ▼
Check flags: V=1?  ──NO──▶  Page Fault Exception → CPU
      │YES
      ▼
Permission OK?     ──NO──▶  Permission Fault Exception → CPU
      │YES
      ▼
Install in TLB (evict LRU entry if full)
      │
      ▼
Retry original load/store → now hits TLB → ~1 cycle
`

Each level read may itself be a cache hit (L1/L2/L3) or require a DRAM access. Modern cores use page walk caches — small caches that store partial walk results (e.g. the L1 entry) to avoid re-reading upper levels on sequential accesses within the same 1 GB region.

4 · Architecture-Specific MMU Hardware
4.1 ARM64 — ARMv8-A MMU

`
┌──────────────────────────────────────────────────────────────┐
│  ARMv8-A MMU Block                                           │
│                                                              │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────────┐  │
│  │  L1 I-TLB  │    │  L1 D-TLB  │    │   L2 Unified    │  │
│  │  48 entries │    │  48 entries │    │   TLB 2048 ent  │  │
│  │  Full assoc │    │  Full assoc │    │   8-way set-a   │  │
│  └──────┬──────┘    └──────┬──────┘    └────────┬────────┘  │
│         └──────────────────┴─────────────────────┘          │
│                                    │ miss                    │
│                         ┌──────────▼──────────┐             │
│                         │  HW Page Table Walker│            │
│                         │  reads TTBR0/TTBR1  │            │
│                         │  4-level walk        │            │
│                         └──────────────────────┘             │
│                                                              │
│  Key control registers:                                      │
│    SCTLREL1[0]  = MMU enable                               │
│    TTBR0EL1     = user PGD base + ASID                     │
│    TTBR1EL1     = kernel PGD base                          │
│    TCREL1       = T0SZ/T1SZ (VA size), TG (granule),       │
│                    ORGN/IRGN (shareability), A1 (ASID src)   │
│    MAIREL1      = memory attribute index table              │
└──────────────────────────────────────────────────────────────┘
`

ARM64 MMU hardware features:
• Two independent translation table bases — TTBR0 for user space (VA < 2^(64-T0SZ)), TTBR1 for kernel (VA > 2^(64-T1SZ)). No page table switching on syscall entry — both spaces coexist.
• Hardware ASID tagging — 8- or 16-bit ASID embedded in the TLB entry. Process switch just updates TTBR0 + ASID; non-global entries for other ASIDs remain valid and don't need flushing.
• Three configurable granule sizes — 4 KB, 16 KB, 64 KB (set in TCREL1.TG0/TG1). 4 KB is UIOX default.
• Block entries — a level-2 PTE can describe a 2 MB contiguous block (no L3 table needed). A level-1 entry can describe 1 GB. These are "huge pages."
• Stage-2 translation (EL2/hypervisor) — adds a second VA→IPA→PA translation for virtualisation; not used in base UIOX.
• Dirty/Access bit management — hardware sets AF (Access Flag) and DBM (Dirty Bit Management) bits in PTEs; eliminates the need for software access tracking on modern cores.

ARM64 TLB invalidation instructions in UIOX:

| Instruction | Scope | Used for |
|---|---|---|
| TLBI VMALLE1IS | All EL1 non-global entries, Inner Sharable | flushtlbmm — process switch |
| TLBI VAE1IS, Xt | Single VA, all ASIDs, IS | flushtlbpage |
| TLBI ASIDE1IS, Xt | All entries with given ASID | Process-specific flush |
| TLBI VAAE1IS, Xt | Single VA, all ASIDs, no ASID match | Kernel mapping change |
| TLBI ALLE1IS | All EL1 entries including global | flushtlball |

The IS suffix means Inner Sharable domain — the invalidation is broadcast to all cores in the cluster, essential for SMP correctness.

4.2 ARM32 — ARMv7-A MMU

`
┌──────────────────────────────────────────────────────────────┐
│  ARMv7-A MMU (CP15)                                          │
│                                                              │
│  ┌──────────┐  ┌──────────┐  ┌──────────────────────────┐  │
│  │ ITLB     │  │ DTLB     │  │  Unified TLB (optional)  │  │
│  │ 32 ent   │  │ 32 ent   │  │  256–512 entries         │  │
│  └────┬─────┘  └────┬─────┘  └───────────┬──────────────┘  │
│       └─────────────┴────────────────────┘                  │
│                           │ miss                             │
│                  ┌────────▼────────┐                        │
│                  │  HW Walker      │                        │
│                  │  reads TTBR0    │                        │
│                  │  2-level walk   │                        │
│                  └─────────────────┘                        │
│                                                              │
│  CP15 control registers:                                     │
│    c1  (SCTLR): M[0]=MMU, C[2]=DCache, I[12]=ICache        │
│    c2  (TTBR0/TTBR1/TTBCR): page table base, boundary      │
│    c3  (DACR): domain access control (deprecated in v7)     │
│    c6  (DFAR): data fault address                           │
│    c8  (TLB ops): MCR p15 invalidate instructions           │
└──────────────────────────────────────────────────────────────┘
`

ARM32 MMU hardware features:
• Short Descriptor format — 2-level (L1 section table + L2 page table) or 1-level (1 MB sections).
• LPAE (Large Physical Address Extension) — 3-level table, 40-bit PA, up to 1 TB physical memory.
• Domain access control (DACR) — 16 domains with No Access / Client / Manager permissions (largely replaced by PTE-level permissions in ARMv7).
• 8-bit ASID in CONTEXTIDR register; smaller ASID space than ARM64 so flushes are more frequent.
• No hardware dirty bit — software must emulate dirty tracking via write-protect + fault.

4.3 x86-64 — Intel/AMD MMU

`
┌──────────────────────────────────────────────────────────────┐
│  x86-64 MMU (Intel / AMD)                                    │
│                                                              │
│  ┌────────────┐    ┌────────────┐    ┌──────────────────┐   │
│  │  ITLB      │    │  DTLB      │    │  STLB (L2)       │   │
│  │  128 ent   │    │  64 ent    │    │  1536 entries     │   │
│  │  4-way SA  │    │  4-way SA  │    │  12-way set-assoc │   │
│  └─────┬──────┘    └─────┬──────┘    └────────┬─────────┘   │
│        └─────────────────┴──────────────────────┘           │
│                                 │ miss                        │
│                       ┌─────────▼────────┐                  │
│                       │  HW Page Walker  │                  │
│                       │  CR3 → PML4      │                  │
│                       │  4-level walk    │                  │
│                       └──────────────────┘                  │
│                                                              │
│  Control registers:                                          │
│    CR0[31] PG   = paging enable                             │
│    CR0[16] WP   = write-protect in ring 0                   │
│    CR2          = faulting linear address                   │
│    CR3          = PML4 physical base + PCID[11:0]           │
│    CR4[5]  PAE  = physical address extension (always set)   │
│    CR4[7]  PGE  = global pages (not flushed by CR3 write)   │
│    CR4[17] PCID = Process Context IDs (INVPCID instruction)  │
│    EFER[11] NXE = execute-disable bit in PTEs               │
└──────────────────────────────────────────────────────────────┘
`

x86-64 MMU hardware features:
• PCID (Process Context ID) — 12-bit tag in CR3[11:0]. With CR4.PCIDE=1, writing CR3 with NOFLUSH=1 (bit 63) does not flush TLB entries tagged with the new PCID. Eliminates full TLB flush on context switch — significant on Spectre-mitigated kernels with KPTI.
• Global pages — PTE[8]=G marks kernel mappings as global; they survive CR3 writes (unless CR4.PGE is toggled). INVLPG does flush global entries. Used for kernel identity map that stays in TLB across process switches.
• NX/XD bit — EFER[11] enables the No-Execute bit (PTE[63]). Prevents code execution from data pages — hardware enforced, no software overhead.
• 2 MB and 1 GB huge pages — set by PS=1 in PD (2 MB) or PDPT (1 GB) entries. Reduce TLB pressure dramatically for large contiguous mappings (GPU framebuffers, kernel heap).
• INVLPG — hardware instruction to flush a single TLB entry. Far faster than a full CR3 reload for targeted unmaps.
• INVPCID — flush by PCID (type 0: single addr+PCID, type 1: all for PCID, type 2: all except globals, type 3: all including globals).
• Hardware A/D bits — Accessed and Dirty bits in PTEs are set by the hardware walker automatically (same as ARM64 AF/DBM). Linux / UIOX read these to implement working-set estimation for page reclaim.

4.4 RISC-V 64 — Sv39 / Sv48 MMU

`
┌──────────────────────────────────────────────────────────────┐
│  RISC-V 64 MMU (SiFive U74 / QEMU virt)                     │
│                                                              │
│  ┌──────────┐   ┌──────────┐   ┌──────────────────────┐    │
│  │  ITLB    │   │  DTLB    │   │  Unified TLB          │   │
│  │  32 ent  │   │  32 ent  │   │  (impl. dependent)    │   │
│  │  FA      │   │  FA      │   │  SiFive U74: 512 ent  │   │
│  └────┬─────┘   └────┬─────┘   └───────────┬──────────┘   │
│       └──────────────┴──────────────────────┘              │
│                              │ miss                          │
│                    ┌─────────▼─────────┐                   │
│                    │   HW PTW          │                   │
│                    │   reads satp.PPN  │                   │
│                    │   Sv39: 3 levels  │                   │
│                    │   Sv48: 4 levels  │                   │
│                    └───────────────────┘                   │
│                                                              │
│  CSR control:                                                │
│    satp:  MODE[63:60] + ASID[59:44] + PPN[43:0]            │
│    stvec: S-mode trap vector (page fault handler entry)     │
│    stval: faulting VA on page fault                         │
│    scause: fault type (12=inst PF, 13=load PF, 15=store PF) │
└──────────────────────────────────────────────────────────────┘
`

RISC-V MMU hardware features:
• satp CSR — the single register controlling the entire MMU: mode field selects Sv32/Sv39/Sv48/Sv57, ASID for tagged TLB entries, PPN for root page table.
• 16-bit ASID — larger than ARM32 (8-bit), same as ARM64. Allows many processes' TLB entries to coexist without full flushes.
• SFENCE.VMA rs1, rs2 — the single RISC-V TLB invalidation instruction. rs1=x0 → all addresses; rs2=x0 → all ASIDs. Can target a specific VA (rs1) and/or specific ASID (rs2) for surgical flushing.
• Software-managed TLB on some implementations — the RISC-V ISA permits implementations where the hardware does not walk page tables and instead takes a trap on every TLB miss (software-filled TLB). QEMU virt and SiFive U74 use hardware walkers, but bare-metal RISC-V MCUs may not.
• No hardware A/D bits on some cores — if PTE.A or PTE.D is 0 when the hardware walker encounters it, some implementations raise a page fault for software to set it; others set it in hardware. The SiFive U74 sets A/D in hardware.
• PMA (Physical Memory Attributes) — a separate hardware mechanism (complementary to the page table) that defines memory type (cacheable/non-cacheable, read/write/execute) for physical address regions. Cannot be overridden by PTEs. UIOX's uioxsocmap.h defines these regions.

5 · Hardware MMU Performance Numbers

| Event | Approximate Cost | Notes |
|---|---|---|
| TLB hit (L1) | 1–2 cycles | Parallel lookup across all entries |
| TLB hit (L2/unified) | 5–12 cycles | Set-associative, slightly slower |
| Page table walk, all cache hits | 30–60 cycles | 4 cache reads × ~8 cycles each |
| Page table walk, one DRAM miss | 80–200 cycles | One L3 miss + DRAM latency |
| Page table walk, all DRAM misses | 400–800 cycles | Rare; kernel typically hot in L3 |
| INVLPG / TLBI VAE1IS single page | 10–30 cycles | Broadcast to cluster on IS variants |
| Full TLB flush (write CR3 / TLBI VMALLE1IS) | 50–300 cycles | Depends on TLB size and pipeline depth |
| Page fault trap (validity fault) | 500–5000+ cycles | Includes TLB miss + SW handler + re-walk |

6 · UIOX Integration Points

| Hardware Event | UIOX Response | File |
|---|---|---|
| TLB miss → HW walk succeeds | Hardware fills TLB, CPU retries | (transparent — no SW involvement) |
| TLB miss → entry invalid (PTEVALID=0) | #PF exception → vfault() | src/pagefault.c |
| TLB miss → permission violation | #PF exception → pfault() | src/pagefault.c |
| exec() / fork() | flushtlbmm — new ASID/CR3 | 10Arch/*/src/archinit.c |
| munmap() / mprotect() | flushtlbrange — targeted INVLPG | src/mm.c |
| Kernel page table change | flushtlbkernelrange | PhyMemLayout.h |
| Low memory → swapper woken | wakeupswapper() → page reclaim | src/swapper.c |
| ASLR at exec() | Randomise TTBR0/satp base VA | 33ProcessControlSubsystem/14sec/ |
| SoC boot (MMU off → on) | Write SCTLR/CR0/satp | 02FwHal/src/uioxfwriscv.c`, arch_init.c |
============================================================
Disabling cache and TLB during bootup is a fundamental requirement for correct early hardware initialisation. Here is a complete explanation covering all four UIOX architectures.

Why You Must Disable Cache and TLB at Boot

Before the MMU and cache are configured, the CPU has no idea which physical addresses are RAM, which are MMIO device registers, and which are reserved. If the cache is on before page tables are set up:

• A store to an MMIO register might be absorbed by the cache and never reach the device
• A load from an uninitialised page table entry might return stale cache data instead of reading real memory
• After copying code/data to a new physical address, an I-cache stale hit can cause execution from the old location

The boot sequence is therefore always: disable cache → disable MMU/TLB → set up page tables → enable MMU → enable cache.

ARM64 — Cache and TLB Disable at Boot
Step 1 — Disable D-cache and I-cache (SCTLREL1)

At reset, SCTLREL1 is architecturally UNKNOWN. The first thing boot code must do is clear the cache and MMU enable bits:

``asm
/ Read current SCTLREL1 /
mrs  x0, sctlrel1

/ Clear:
    bit  0  = M  (MMU enable)
    bit  2  = C  (D-cache enable)
    bit 12  = I  (I-cache enable)
 /
bic  x0, x0, #(1 << 0)    // M  — MMU off
bic  x0, x0, #(1 << 2)    // C  — D-cache off
bic  x0, x0, #(1 << 12)   // I  — I-cache off
msr  sctlrel1, x0
isb                        // synchronise pipeline
`

Step 2 — Invalidate ALL TLB entries (before MMU is on)

Even before enabling the MMU you must flush stale TLB state from a previous boot or warm reset:

`asm
tlbi vmalle1                // invalidate all EL1 TLB entries
dsb  sy                    // ensure invalidation is complete
isb                        // flush pipeline
`

Step 3 — Invalidate D-cache and I-cache

`asm
/ Invalidate all I-cache to point of unification /
ic   iallu
dsb  sy
isb

/ For D-cache: invalidate by set/way (loop over all sets and ways)
  Read CCSIDREL1 to get geometry, then: /
dc   isw, x0               // invalidate by set/way (loop)
dsb  sy
`

Step 4 — Set up page tables, TTBR0/TTBR1, TCR, MAIR

`asm
msr  ttbr0el1, x1         // user page table base
msr  ttbr1el1, x2         // kernel page table base
msr  tcrel1,   x3         // VA range, granule, shareability
msr  mairel1,  x4         // memory attributes
isb
`

Step 5 — Enable MMU + cache together

`asm
mrs  x0, sctlrel1
orr  x0, x0, #(1 << 0)    // M  — MMU on
orr  x0, x0, #(1 << 2)    // C  — D-cache on
orr  x0, x0, #(1 << 12)   // I  — I-cache on
msr  sctlrel1, x0
isb                        // pipeline flush — next instruction uses MMU
`

ARM32 — Cache and TLB Disable at Boot

ARM32 uses CP15 coprocessor instructions to access MMU and cache control registers:

Step 1 — Disable MMU, D-cache, I-cache

`asm
/ Read SCTLR (System Control Register) /
mrc  p15, 0, r0, c1, c0, 0

/ Clear:
    bit  0 = M  (MMU enable)
    bit  2 = C  (D-cache)
    bit 12 = I  (I-cache)
 /
bic  r0, r0, #(1 << 0)    @ MMU off
bic  r0, r0, #(1 << 2)    @ D-cache off
bic  r0, r0, #(1 << 12)   @ I-cache off
mcr  p15, 0, r0, c1, c0, 0
isb
`

Step 2 — Invalidate entire TLB

`asm
/ Unified TLB invalidate all /
mcr  p15, 0, r0, c8, c7, 0    @ TLBIALL — invalidate all TLB entries
dsb                             @ barrier
isb
`

Step 3 — Invalidate I-cache and D-cache

`asm
/ Invalidate entire instruction cache /
mcr  p15, 0, r0, c7, c5, 0    @ ICIALLU — I-cache invalidate all

/ Invalidate entire D-cache (simplified — production uses set/way loop) /
mcr  p15, 0, r0, c7, c6, 0    @ DCISW (if supported) or flush all ways
dsb
isb
`

Step 4 — Enable MMU after page tables are ready

`asm
/ Load TTBR0 with page table base /
mcr  p15, 0, r1, c2, c0, 0    @ TTBR0 = page table physical base

/ Set domain access: domain 0 = client (checks permissions) /
mov  r0, #0x1
mcr  p15, 0, r0, c3, c0, 0    @ DACR

/ Enable MMU + caches /
mrc  p15, 0, r0, c1, c0, 0
orr  r0, r0, #(1 << 0)        @ M — MMU
orr  r0, r0, #(1 << 2)        @ C — D-cache
orr  r0, r0, #(1 << 12)       @ I — I-cache
mcr  p15, 0, r0, c1, c0, 0
isb
`

x86-64 — Cache and TLB Disable at Boot
Step 1 — Disable cache via CR0.CD and CR0.NW

On x86-64, cache control lives in CR0 bits:
• CR0[30] = CD (Cache Disable) — disables L1/L2/L3 fill
• CR0[29] = NW (Not Write-Through) — controls write policy

`asm
/ Read CR0 /
mov  rax, cr0

/ Set CD=1 (cache disable), clear NW=0 (must be 0 when CD=1) /
or   rax, (1 << 30)      ; set CD
and  rax, ~(1 << 29)     ; clear NW (required when CD=1)
mov  cr0, rax

/ Flush existing cache lines from all levels /
wbinvd                   ; write-back and invalidate all caches
`

Step 2 — Flush TLB (write CR3 with current value)

`asm
/ TLB flush: write CR3 to itself (flushes all non-global entries) /
mov  rax, cr3
mov  cr3, rax

/ Or, on systems without paging yet (real mode / protected mode entry):
  TLB is irrelevant until CR0.PG is set /
`

Step 3 — Disable paging (MMU off)

`asm
/ Clear CR0.PG (bit 31) to turn off paging /
mov  rax, cr0
and  rax, ~(1 << 31)     ; PG = 0 — paging off (MMU off)
mov  cr0, rax
`

> Note: x86-64 long mode requires paging to be enabled. You cannot run 64-bit code with CR0.PG=0. The disable/re-enable sequence only applies during the transition from 32-bit protected mode to 64-bit long mode setup.

Step 4 — Set up page tables, load CR3

`asm
/ Point CR3 at PML4 table physical address /
mov  rax, pml4physaddr
mov  cr3, rax             ; also flushes TLB

/ Enable CR4.PAE (required for 64-bit) /
mov  rax, cr4
or   rax, (1 << 5)        ; PAE = 1
mov  cr4, rax
`

Step 5 — Enable paging + cache

`asm
/ Enable paging and cache together /
mov  rax, cr0
or   rax, (1 << 31)       ; PG = 1 — paging on
and  rax, ~(1 << 30)      ; CD = 0 — cache on
mov  cr0, rax

/ Full serialising instruction after CR0 change /
jmp  $+2                  ; near jump to flush pipeline
`

RISC-V 64 — Cache and TLB Disable at Boot

RISC-V has no architectural instruction to disable the cache — cache control is implementation-defined (vendor-specific MMIO or non-standard CSRs). However, the MMU and TLB are fully controlled via satp:

Step 1 — Disable MMU (satp = 0)

Writing zero to satp puts the CPU in Bare mode — no address translation, all accesses use physical addresses directly:

`asm
MODE=0 (Bare) — MMU completely off
csrw satp, zero          # satp = 0 → no translation
sfence.vma zero, zero    # flush entire TLB
`

Step 2 — Flush TLB after satp change

`asm
sfence.vma zero, zero    # rs1=x0 → all addresses
                         # rs2=x0 → all ASIDs
                         # = full TLB flush
`

Step 3 — Disable cache (SiFive U74 / vendor-specific)

On the SiFive U74 (HiFive Unmatched), the L2 cache controller has an MMIO register to disable ways:

`c
/ SiFive L2 cache controller — disable all ways by writing 0 /
#define SIFIVEL2CCWAYS   0x02010008UL
(volatile uint32t )SIFIVEL2CCWAYS = 0u;   / 0 ways = L2 disabled /
`

On QEMU virt (no real cache), this step is a no-op.

Step 4 — Set up page tables, re-enable MMU

`asm
Write root page table physical address + Sv39 mode + ASID into satp
satp[63:60] = 8 (Sv39), satp[59:44] = ASID, satp[43:0] = PPN
la    t0, pagetableroot
srli  t0, t0, 12              # convert PA to PPN (PA >> 12)
li    t1, (8UL << 60)         # MODE = 8 (Sv39)
or    t0, t0, t1
csrw  satp, t0                # write satp → MMU now on
sfence.vma zero, zero         # flush stale TLB entries from Bare mode
`

Summary Comparison

| Step | ARM64 | ARM32 | x86-64 | RISC-V |
|---|---|---|---|---|
| Disable MMU | SCTLREL1[0]=0 | MCR p15,c1 M=0 | CR0[31] PG=0 | csrw satp, zero |
| Disable D-cache | SCTLREL1[2]=0 | MCR p15,c1 C=0 | CR0[30] CD=1 | Vendor MMIO (SiFive L2CC) |
| Disable I-cache | SCTLREL1[12]=0 | MCR p15,c1 I=0 | CR0[30] CD=1 | Vendor MMIO |
| Flush cache | DC ISW loop + IC IALLU | MCR p15,c7 | WBINVD | N/A (arch transparent) |
| Flush TLB | TLBI VMALLE1 + DSB + ISB | MCR p15,0,r,c8,c7,0 | Write CR3 | SFENCE.VMA zero,zero |
| Barrier after | ISB | ISB | JMP $+2 | SFENCE.VMA (acts as barrier) |
| Re-enable order | Set TTBR/TCR/MAIR → SCTLR[0,2,12]=1 + ISB | Set TTBR/DACR → SCTLR[0,2,12]=1 + ISB | Set PML4/CR3/CR4 → CR0[31]=1, CR0[30]=0 | Write satp with MODE≠0 → SFENCE.VMA |

UIOX Boot Code Locations

| File | What it does |
|---|---|
| 10Arch/arm64/src/archinit.c | ARM64 SCTLREL1 / DAIF / GIC setup after EL3 → EL1 drop |
| 10Arch/arm32/src/archinit.c | ARM32 CP15 SCTLR, cache, and GIC init |
| 10Arch/x8664/src/archinit.c | x86-64 CR0/CR3/CR4 paging setup, LAPIC |
| 10Arch/riscv64/src/archinit.c | RISC-V satp=0 at entry, sfence.vma, then Sv39 enable |
| 01uBoot/src/uioxbootmain.c | Stage 1 "HW init" — calls archinit() which does cache/TLB setup |
| 02FwHal/src/uioxfwriscv.c | rvcacheenable() → FENCE.I, rvtlbflush() → SFENCE.VMA |
| 10Arch/arm64/src/uioxsocarm64init.c | arm64cacheflushall() — dsb sy + ic iallu + isb |
| 10Arch/arm32/src/uioxsocarm32init.c | arm32l2c310init()` — invalidate all L2 ways before enable |