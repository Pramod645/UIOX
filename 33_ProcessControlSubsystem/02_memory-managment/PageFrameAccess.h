#ifndef __MEM_MNGM_H
#define __MEM_MNGM_H

/*
1. how the kernel handles dynamic memory by keeping track of free and busy page frames?
It is in MemMngm.

2. how every process in User Mode has its own address space and has its requests for memory satisfied
by the kernel one page at a time, so that page frames can be assigned to the
process at the very last possible moment.

3. how the kernel makes use of dynamic memory to implement both memory and disk caches.
*/ 
/*
1. virtual memory subsystem by discussing page frame reclaiming.

*/ 
/*
1. The Page Frame Reclaiming Algorithm.
    why the kernel needs to reclaim page frames and what strategy it uses to achieve this.

2.Reverse Mapping
    data structures used by the kernel to locate
quickly all the Page Table entries that point to the same page frame.

3.Swapping.: it covers the swap subsystem, a kernel component used to save anonymous.

*/


#endif