================================================================================================================
Tagged Memory address in AArch64 UIOX
================================================================================================================

Author: Pramod Kumar <xxxxx>
Memory Addess:
                1. Logical addresses
                2. Linear addresses also know as virtual addresses
                3. Physical addresses 


                        
Logical Address ===> [Segment Unit]Linear Address ===>[Pagging Unit]Physical Address ===>[DRAM or Main Memory]

Logical Address: 16 bit segment selector + 32 bits offsets, machine language instructions to specify the address of an operand or of an instruction
Linear Addess: A single 32-bit unsigned integer that can be used to address up to 4 GB—that is, up to 4,294,967,296 memory cells
Physical Addess: address memory cells in memory chips.

Seggment Selector/ Segmentations Registers/ Segment Identifier
================================================================================================================
|15 to 3 index    |TI|RPL1|RPL0|     
================================================================================================================
Seggment Registers: Code Segment(CS),stack Seggment(SS) and Data Seggment(DS)  

Linear Addesses: 

PAGING 32bits: = DIRECTORY:10 + TABLE:10 + OFFSET:12
PAGING 64bits: = DIRECTORY:10 + TABLE:10 + OFFSET:12

2^12 = 4KB

64 bit Arch:
48 bits(For ARM ARCH) Address range upto 256 TABLE
48-Offset:12 = 36
PAGING 64bits: = DIRECTORY:18 + TABLE:18 + OFFSET:12

ARM ARCH= Pagging size:4kb, Adresses bit 48,Number of level 3,linear adresses split =9+9+9+9+12

Physical Address:
Physical Addess:RAM supported by a processor is limited by the number of address
pins connected to the address bus.
Physical Address Extension (PAE) Paging Mechanism:

CACHE:
Logical Address ===> [Segment Unit]Linear Address ===>[Pagging Unit] ==>[CACHE]Physical Address ===>[DRAM or Main Memory]

Cache entries=Tags+flags
Address=TAg + Cache controler subset index + offset within lines

When accessing a RAM memory cell, the CPU extracts the subset index from the physical address and compares the tags of all lines in the subset with the high-order bits of the physical address.
In a write-through, the controller always writes into both RAM and the cache line, effectively switching off the cache for write operations.
 In a write-back, which offers more immediate efficiency, only the cache line is updated and the contents of the RAM are left unchanged. After a write-back, of course, the RAM must eventually be updated.
 The cache controller writes the cache line back into RAM only when the CPU executes an instruction requiring a flush of cache entries or when a FLUSH hardware signal occurs (usually after a cache miss).
When a cache miss occurs, the cache line is written to memory, if necessary, and the correct line is fetched from RAM into the cache entry.
The cache controller writes the cache line back into RAM only when the CPU executes an instruction requiring a flush of cache entries or when a FLUSH hardware signal occurs (usually after a cache miss).
When a cache miss occurs, the cache line is written to memory, if necessary, and the correct line is fetched from RAM into the cache entry.


UNIFORM PAGGING for 32 and 64 Bit Arch:
Linear Addess 64 bits = Global Dir + Upper Dir +Middle Dir + Table + Offset
Linear Addess 32 bits = Global Dir + Upper Dir:0bit + Middle Dir:0bit + Table + Offset
==============================================================================================================
Memory Management   
==============================================================================================================
1.How Kernel allocate dynamic memory to itself by below these three techniques
    a.Page frame Management(Handle physically contineous memory area)
        1.Page descriptors 2.NUMA, 3.Memory Zones, 4.Pool  of reserved page frames
        5.The zoned page frame allocator 6.Kernel mapping of hight memory frames
        7.The buddy system algorithm 8. the Per-CPU Page frame Cache  
    b.Memory area Management(Handle physically contineous memory area)
        1.The Slab Allocator 2.Cache Descriptor 3.Slab descriptor 4.Object descriptor
        5.Aligning Objects in Memory 6.Slab Coloring 7.Local cache of free Slab Objects
        8.Allocating Slab Object 9.Freeing a Slab Object 10. Memory Pools
    c.Noncontineous Memory Area Managment 
        1.Linear address of Noncontineous Memory Areas 2.Descriptor of Noncontineous Memory Areas
        3.Allocating a Noncontineous Memory Area 4.Releasing a Noncontineous memory Area
        5.

techniques#Memory Zones, Kernel mappings,the budy system, the slab cache and memory pool