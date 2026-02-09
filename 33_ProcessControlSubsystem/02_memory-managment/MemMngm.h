#ifndef __MEM_MNGM_H
#define __MEM_MNGM_H

/*
2. Addressging
*/
/*
Management
*/
/*
17.Page Frame Reclaiming
*/
// SWAP,memory management policy is called swapping
#define ARCH_32_64  TRUE // True for 32 bit and false for 64

#ifdef ARCH_32_64
#define TABLE 12 // 12 bit
#define DIRECTORY 12 // 12 bit
#elif
#define TABLE 12 // 12 bit
#define DIRECTORY 12 // 12 bit
#endif

#define CACHE 0 // 0-Directive, 1-associative

#define USER_CODE 0x0000
#define USER_DATA 0x0000
#define KERNEL_CODE 0x0000
#define KERNEL_DATA 0x0000

#define KERNEL 0
#define USER 1

#define MODE KERNEL

#define BASE (* 0x0000)
#define b_GRANUALARITY 0
#define b_Systemflag 0
#define TYPE
#define DPL MODE
#define b_P 0
#define D_B 0
#define AVL 0

/*
GDT
*/
#define NULL 0x0
#define RESERVED 1
#define RESERVED 2
#define RESERVED 3
#define NOTE_USED 4
#define NOTE_USED 5
#define TLS1 0x33
#define TLS2 0x3b
#define TLS3 0x43
#define RESERVED 1
#define RESERVED 2
#define RESERVED 3
#define KERNEL_CODE 0x60
#define KERNEL_DATA 0x68
#define USER_CODE 0x73
#define USER_DATA 0x7b

#define TSS 8x0
#define LDT 0x88
#define RESERVED 0x90
#define RESERVED 0x98
#define NOTE_USED 0xa0
#define NOTE_USED 0xa8
#define TLS1 0xb0
#define TLS2 0xb8
#define TLS3 0xc0
#define NOT_USED 
#define NOT_USED 
#define NOT_USED 
#define NOT_USED 
#define NOT_USED 
#define DAFULT_TSS 0xf8


/*
PAGING
*/
#define TABLE_1 0
#define TABLE_2 1
#define TABLE_3 2
#define TABLE_4 3
#define TABLE_5 4
#define TABLE_6 5
#define TABLE_7 6
#define TABLE_8 7
#define TABLE_9 8
#define TABLE_10 9
#define TABLE_11 10
#define TABLE_12 11
#define TABLE_13 12
#define TABLE_14 13
#define TABLE_15 14
#define TABLE_16 15

/*
PAGE DIRECTORY
*/
#define PAGE_DIRECTORY_1 0

#define PAGE_DIRECTORY_16 15

struct segementDescriptor{    
    *ptr BAse;
    bool granualarity;
    bool systeFlag;
     type;


    /* data */
}; // 64 bits, Stored in Gloabl Descriptor table(GDT) or in Local Descrptor Table(LDT), gdt and ldt have gdtr and ldtr control registers
// Segements selectore used for 1. Data segment selector 2.code segment selector and 3. system descriptor selector

struct segmentSelector{
    bool Index;
    bool TI;
    RPL;
}; // 8 bits

void segmentUnit(); //Status of TI, 2. address of segment descriptor x 8(size of segment descriptor)=gdtr or lddt, 3. add offset to the base filed of segment descriptor



/* *************  ##Memory Managament## ******************* */

/******************** 1.Page frame Management **********************************/ 
// 1. page descriptor
union flags{

};

struct pageDescriptor{
    unsigned long flags; // array of flag (PG_locked,PG_error,PG_referenced,PG_updated,PG_dirty,PG_lru,PG_active,PG_slab,PG_highmem,PG_checked,PG_arch_1,PG_reserved,PG_private,PG_writeback,PG_nosave,PG_compound,PG_swapcache,PG_mappedtodisck,PG_PG_reclaim,PG_nosave_free)
    atomic_t _count;        //page frames refrrence counter
    atomic_t _mapcount;         //no. of page table entires to the page
    unsigned long private;       //available to the kernel component to using the page
    struct address_space* mapping;  //used when page is inserted into the page cache
    unsigned long index;            // used by several kernel component with diffirent meanings
    struct list_head lru;  //contains pointer to least recently used doubly linked list of page lru(least recent used)
};

//2.NUMA
struct  nodeDesriptor{  // all node are stored in singly linked list 
    struct zone[];
    struct zonelist[];
    int nr_zones;
    struct page* node_mem_map;
    struct bootmem_data* bdata;
    unsigned long node_start_pfn;
    unsigned long node_present_pages;
    unsigned long node_spanned_pages;
    int node_id;
    pg_data_t* pgdata_next;
    wait_queue_head_t kswapd_wait;
    struct task_struct* kswapd;
    int kswapd_max_order;
    
};

// 3.Memory Zones
#define ZONE_DMA     //page frames of memory below 16 MB
#define ZONE_NORMAL  //page frames of memory at and above 16 MB and below 896 MB
#define ZONE_HIGHMEM //page frames of memory at and above 896 MB

struct zoneDescriptor{
    unsigned long free_pages;
    unsigned long pages_min;
    unsigned long pages_low;
    unsigned long pages_high;
    unsigned long lowmem_reserve[];
    struct per_cpu_pageset pageset[];
    spinlock_t lock;
    struct free_area free_area[];
    spinlock_t lru_lock;
    struct list head active_list;
    struct list head inactive_list;
    unsigned long nr_scan_active;
    unsigned long nr_scan_inactive;
    unsigned long nr_active;
    unsigned long nr_inactive;
    unsigned long pages_scanned;
    int all_unreclaimable;
    int temp_priority;
    int prev_priority;
    wait_queue_head_t* wait_table;
    unsigned long wait_table_size;
    unsigned long wait_table_bits;
    struct pglist_data* zone_pgdat;
    struct page * zone_mem_map;
    unsigned long zone_start_pfn;
    unsigned long spanned_pages;
    unsigned long present_pages;
    char * name;
};

// 4.Pool  of reserved page frames
reserved pool size = square root of(16 × directly mapped memory) (kilobytes)

//5.The zoned page frame allocator

alloc_pages(gfp_mask, order);
alloc_page(gfp_mask); //alloc_pages(gfp_mask, 0);
__get_free_pages(gfp_mask, order);
__get_free_page(gfp_mask); //Macro used to get a single page frame; it expands to:__get_free_pages(gfp_mask, 0)
get_zeroed_page(gfp_mask); //alloc_pages(gfp_mask | __GFP_ZERO, 0)
__get_dma_pages(gfp_mask, order) ;// __get_free_pages(gfp_mask | __GFP_DMA, order)

union requestflags{
    __GFP_DMA;
    __GFP_HIGHMEM;
    __GFP_WAIT;
    __GFP_HIGH;
    __GFP_IO;
    __GFP_FS;
    __GFP_COLD;
    __GFP_NOWARN;
    __GFP_REPEAT;
    __GFP_NOFAIL;
    _GFP_NORETRY;
    __GFP_NO_GROW;
    __GFP_COMP;
    __GFP_ZERO;
};  

union gpFlagName{
    GFP_ATOMIC;
    GFP_NOIO;
    GFP_NOFS;
    GFP_KERNEL;
    GFP_USER;
    GFP_HIGHUSER;
};
__free_pages(page, order);
free_pages(addr, order);
__free_page(page);
free_page(addr);


//6.Kernel mapping of hight memory frames
void * kmap(struct page * page);
void * kmap_high(struct page * page);
map_new_virtual();
void kunmap_high(struct page * page);
void * kmap_atomic(struct page * page, enum km_type type);

//7.The buddy system algorithm
/*
it must deal with a well-known memory management
problem called external fragmentation: frequent requests and releases of groups
of contiguous page frames of different sizes may lead to a situation in which several
small blocks of free page frames are “scattered” inside blocks of allocated page
frames.
*/
_ _rmqueue();
_ _free_pages_bulk( );


//8.the Per-CPU Page frame Cache
//hot CACHE
//cold CACHE

struct per_cpu_pages_Descriptor{
    int count;
    int low;
    int high;
    int batch;
    struct list_head list;
};

buffered_rmqueue();

//9.The Zone Allocator



/******************** 2.Memory area Management **********************************/ 

//internal fragmentation

//1.The Slab Allocator 
 // it for internal to buddy system to utilize the memory  intrenal to buddy

//2.Cache Descriptor 

get_zeroed_page( );

struct kmem_list3{
    struct list_head slabs_partial;//Doubly linked circular list of slab descriptors with both free and nonfree objects
    struct list_head slabs_full; //Doubly linked circular list of slab descriptors with no free objects
    struct list_head slabs_free;// Doubly linked circular list of slab descriptors with free objects only
    unsigned long free_objects;
    int free_touched;
    unsigned long next_reap;
    struct array_cache * shared;//Pointer to a local cache shared by all CPUs
};

struct kmem_cache_t_Descriptor{
    struct array_cache * array[];
    unsigned int batchcount;
    unsigned int limit;
    struct kmem_list3 lists;
    unsigned int objsize;
    unsigned int flags;
    unsigned int num;
    unsigned int free_limit;
    spinlock_t spinlock;
    unsigned int gfporder;
    unsigned int gfpflags;
    size_t colour;
    unsigned int colour_off;
    unsigned int colour_next;
    kmem_cache_t * slabp_cache;
    unsigned int slab_size;
    unsigned int dflags;
    void * ctor;
    void * dtor;
    const char * name;
    struct list_head next;
};



//3.Slab descriptor

struct slab_descriptor{
    struct list_head list;//Pointers for one of the three doubly linked list of slab descriptors (either the slabs_full, slabs_partial, or slabs_free)
    unsigned long colouroff;//Offset of the first object in the slab
    void * s_mem; //Address of first object (either allocated or free) in the slab
    unsigned int inuse;
    unsigned int free;
};

void * kmem_getpages(kmem_cache_t *cachep, int flags);
void kmem_freepages(kmem_cache_t *cachep, void *addr);
void slab_destroy(kmem_cache_t *cachep, slab_t *slabp);


//4.Object descriptor
   //External object descriptors     
   //Internal object descriptors

struct kmem_bufctl_t_Descriptor{

};



//5.Aligning Objects in Memory 


kmem_cache_create( );


//6.Slab Coloring 

//slab length = (num × osize) + dsize + free




//7.Local cache of free Slab Objects
struct array_cache{
    unsigned int avail;// Number of pointers to available objects in the local cache. The field also acts
//as the index of the first free slot in the cache.
    unsigned int limit;// Size of the local cache—that is, the maximum number of pointers in the local cache.
    unsigned int batchcount;// Chunk size for local cache refill or emptying.
    unsigned int touched; //Flag set to 1 if the local cache has been recently used.
};     

//8..Allocating Slab Object 

void * kmem_cache_alloc(kmem_cache_t *cachep, int flags);


//9.Freeing a Slab Object 

void kmem_cache_free(kmem_cache_t *cachep, void *objp);

// Genaral porpose objects
void * kmalloc(size_t size, int flags);
void kfree(const void *objp);


//10. Memory Pools
/*
a memory pool allows a kernel
component—such as the block device subsystem—to allocate some dynamic
memory to be used only in low-on-memory emergencies.
*/

struct mempool_t_object{
    spinlock_t lock;
    int min_nr;
    int curr_nr;
    void ** elements;// Pointer to an array of pointers to the reserved elements
    void * pool_data;// Private data available to the pool’s owner
    mempool_alloc_t * alloc;// Method to allocate an element
    mempool_free_t * free;// Method to free an element
    wait_queue_head_t wait;// Wait queue used when the memory pool is empty
};



/******************** 3.Noncontineous Memory Area Managment **********************************/ 

/*
The main advantage of this schema is
to avoid external fragmentation, while the disadvantage is that it is necessary to fiddle
with the kernel Page Tables.
the size of a noncontiguous memory area must be a multiple of 4,096.
*/

//1.Linear address of Noncontineous Memory Areas 
/*
VMALLOC_START macro defines the starting address of the linear space reserved for
noncontiguous memory areas, while VMALLOC_END defines its ending address.
VMALLOC_OFFSET
PKMAP_BASE

*/


//2.Descriptor of Noncontineous Memory Areas

struct vm_struct_Descriptor{
    void * addr; //Linear address of the first memory cell of the area
    unsigned long size;// Size of the area plus 4,096 (inter-area safety interval)
    unsigned long flags;// Type of memory mapped by the noncontiguous memory area
    struct page ** pages; // Pointer to array of nr_pages pointers to page descriptors
    unsigned int nr_pages; // Number of pages filled by the area
    unsigned long phys_addr; // Set to 0 unless the area has been created to map the I/O shared memory of a hardware device
    struct vm_struct * next; // Pointer to next vm_struct structure
};
        


//3.Allocating a Noncontineous Memory Area 

void * vmalloc(unsigned long size);



//4.Releasing a Noncontineous memory Area





#endif