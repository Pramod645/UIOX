Memory-Mapped I/O (MMIO):
What it is

Memory-Mapped I/O (MMIO) is an address-mapping technique where hardware device registers are assigned fixed ranges within the CPU's physical address space. When the CPU reads from or writes to those addresses, the transaction is routed to the hardware peripheral — not to RAM. No special I/O instructions are needed; ordinary load / store instructions do all the work.

How a transaction flows

1 — Virtual → Physical address translation
The CPU always works with virtual addresses (VAs). Before anything reaches the hardware bus, the MMU (Memory Management Unit) translates the VA into a physical address (PA) using a:
• TLB (Translation Lookaside Buffer) — a fast cache of recent translations
• Page-table walk — performed automatically on a TLB miss

2 — MMIO regions in the physical address map
The SoC designer carves the physical address space into fixed regions:

``
+-----------------------------------------+
|   SoC Physical Address Map              |
+-----------------------------------------+
| 0x00000000 - 0x0FFFFFFF : DDR (RAM)  |
| 0x10000000 - 0x10000FFF : UART MMIO  |
| 0x12340000 - 0x12340FFF : Device MMIO|
| ...                                     |
+-----------------------------------------+
`

On UIOX, the OS reads these ranges from the Device Tree (DT), reserves them, and creates virtual mappings with device-type memory attributes (non-cacheable, strongly ordered).

3 — Interconnect routing
Once the PA is emitted onto the SoC bus/interconnect, the interconnect decodes the PA and routes the transaction to the correct peripheral — UART, GPIO, timer, etc. — instead of DDR.

Key points at a glance

| Concept | Detail |
|---|---|
| No special instructions | Ordinary LD/ST (ARM), MOV (x86), LW/SW (RISC-V) |
| Address space shared | Device registers live in the same address space as RAM |
| MMU involvement | Every access goes through VA→PA translation |
| Device Tree | On Linux/embedded: the SoC layout is described in DT and passed to the kernel at boot |
| Memory attributes | MMIO pages are marked non-cacheable — accesses must reach the device, not a CPU cache |
| vs. Port I/O | x86 also has a separate 64 KB I/O port space (IN/OUT` instructions); MMIO replaces this on all modern SoCs and on ARM/RISC-V entirely |

MMIO.png