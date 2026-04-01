#define __STANDERDLIB__H
#ifdef __STANDERDLIB__H

xyz(){ // call from Application.h. Wrapper routine in libs standerd library
    
SYS_XYZ();// call to system call handler 
}

/*
Here's a simple memory management implementation in C that demonstrates custom allocation, deallocation, and basic tracking:

``c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define POOLSIZE 1024  1024  // 1 MB memory pool
#define MAXBLOCKS 1000

typedef struct {
    void address;
    sizet size;
    bool isfree;
} MemoryBlock;

typedef struct {
    char pool[POOLSIZE];
    sizet pooloffset;
    MemoryBlock blocks[MAXBLOCKS];
    int blockcount;
    sizet totalallocated;
    sizet totalfreed;
} MemoryManager;

static MemoryManager manager = {0};

// Initialize the memory manager
void mminit(void) {
    manager.pooloffset = 0;
    manager.blockcount = 0;
    manager.totalallocated = 0;
    manager.totalfreed = 0;
    memset(manager.blocks, 0, sizeof(manager.blocks));
}

// Allocate memory from the pool
void mmalloc(sizet size) {
    if (size == 0) return NULL;
    
    // Align to 8 bytes
    size = (size + 7) & ~7;
    
    // First, try to reuse a freed block
    for (int i = 0; i < manager.blockcount; i++) {
        if (manager.blocks[i].isfree && manager.blocks[i].size >= size) {
            manager.blocks[i].isfree = false;
            manager.totalallocated += manager.blocks[i].size;
            return manager.blocks[i].address;
        }
    }
    
    // Allocate from pool
    if (manager.pooloffset + size > POOLSIZE) {
        fprintf(stderr, "Memory pool exhausted!\n");
        return NULL;
    }
    
    if (manager.blockcount >= MAXBLOCKS) {
        fprintf(stderr, "Max block limit reached!\n");
        return NULL;
    }
    
    void ptr = &manager.pool[manager.pooloffset];
    manager.blocks[manager.blockcount].address = ptr;
    manager.blocks[manager.blockcount].size = size;
    manager.blocks[manager.blockcount].isfree = false;
    manager.blockcount++;
    
    manager.pooloffset += size;
    manager.totalallocated += size;
    
    return ptr;
}

// Allocate and zero-initialize memory
void mmcalloc(sizet count, sizet size) {
    sizet total = count  size;
    void ptr = mmalloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

// Reallocate memory
void mmrealloc(void ptr, sizet newsize) {
    if (!ptr) return mmalloc(newsize);
    if (newsize == 0) {
        mmfree(ptr);
        return NULL;
    }
    
    // Find the block
    for (int i = 0; i < manager.blockcount; i++) {
        if (manager.blocks[i].address == ptr && !manager.blocks[i].isfree) {
            if (manager.blocks[i].size >= newsize) {
                return ptr;  // Current block is large enough
            }
            
            // Allocate new block and copy data
            void newptr = mmalloc(newsize);
            if (newptr) {
                memcpy(newptr, ptr, manager.blocks[i].size);
                mmfree(ptr);
            }
            return newptr;
        }
    }
    
    return NULL;
}

// Free allocated memory
void mmfree(void ptr) {
    if (!ptr) return;
    
    for (int i = 0; i < manager.blockcount; i++) {
        if (manager.blocks[i].address == ptr && !manager.blocks[i].isfree) {
            manager.blocks[i].isfree = true;
            manager.totalfreed += manager.blocks[i].size;
            return;
        }
    }
    
    fprintf(stderr, "Warning: Attempting to free invalid pointer\n");
}

// Print memory statistics
void mmstats(void) {
    int activeblocks = 0;
    sizet activememory = 0;
    
    for (int i = 0; i < manager.blockcount; i++) {
        if (!manager.blocks[i].isfree) {
            activeblocks++;
            activememory += manager.blocks[i].size;
        }
    }
    
    printf("\n=== Memory Manager Statistics ===\n");
    printf("Pool size:        %d bytes\n", POOLSIZE);
    printf("Pool used:        %zu bytes\n", manager.pooloffset);
    printf("Total allocated:  %zu bytes\n", manager.totalallocated);
    printf("Total freed:      %zu bytes\n", manager.totalfreed);
    printf("Active blocks:    %d\n", activeblocks);
    printf("Active memory:    %zu bytes\n", activememory);
    printf("=================================\n");
}

// Detect memory leaks
void mmcheckleaks(void) {
    printf("\n=== Memory Leak Report ===\n");
    int leaks = 0;
    
    for (int i = 0; i < manager.blockcount; i++) {
        if (!manager.blocks[i].isfree) {
            printf("Leak: Block %d, %zu bytes at %p\n", 
                   i, manager.blocks[i].size, manager.blocks[i].address);
            leaks++;
        }
    }
    
    if (leaks == 0) {
        printf("No memory leaks detected.\n");
    } else {
        printf("Total leaks: %d\n", leaks);
    }
    printf("==========================\n");
}

// Example usage
int main(void) {
    mminit();
    
    // Allocate some memory
    int numbers = mmalloc(10  sizeof(int));
    char str = mmcalloc(50, sizeof(char));
    double data = mmalloc(5  sizeof(double));
    
    // Use the memory
    for (int i = 0; i < 10; i++) {
        numbers[i] = i  10;
    }
    strcpy(str, "Hello, Memory Manager!");
    
    printf("Numbers: ");
    for (int i = 0; i < 10; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\nString: %s\n", str);
    
    // Reallocate
    numbers = mmrealloc(numbers, 20  sizeof(int));
    
    // Free some memory
    mmfree(str);
    mmfree(data);
    
    // Show stats
    mmstats();
    
    // Check for leaks (numbers not freed intentionally)
    mmcheckleaks();
    
    // Clean up
    mmfree(numbers);
    mmcheckleaks();
    
    return 0;
}
`

Key features
• Pool-based allocation — Uses a fixed-size memory pool instead of calling malloc repeatedly
• Block tracking — Maintains metadata for each allocation
• Memory reuse — Freed blocks can be reused by later allocations
• Alignment — Allocations are 8-byte aligned for performance
• Leak detection — mmcheckleaks()` reports unfreed memory
• Statistics — Track total allocations, frees, and active memory

Limitations of this simple implementation

This is educational code. A production allocator would need:

• Coalescing — Merge adjacent free blocks to reduce fragmentation
• Better fit algorithms — Best-fit or buddy system instead of first-fit
• Thread safety — Mutex locks for concurrent access
• Variable pool sizing — Request more memory from the OS when needed

Want me to expand on any specific aspect, like adding coalescing or thread safety?
*/

#endif // end of __STANDERDLIB__H