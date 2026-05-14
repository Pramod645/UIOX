
#ifndef __UIX_STDATOMIC__H
#define __UIX_STDATOMIC__H
/*
stddef.h
*/
/* This is for only STDLIB */

//#include "uix_features.h" //??


#include "../sys/uix_types.h"

typedef volatile uix_int32_t  uix_atomic_int; //Atomic integer — C11 _Atomic int, used in lock-free programming
typedef volatile uix_uint32_t uix_atomic_uint;
typedef volatile uix_int64_t  uix_atomic_long;
typedef volatile uix_uint64_t uix_atomic_ulong;
typedef volatile uix_bool_t   uix_atomic_bool;

#define uix_atomic_load_explicit(p,o)      __atomic_load_n((p),(o)) //Atomically loads value — equivalent to C11 atomic_load(), uses __ATOMIC_SEQ_CST
#define uix_atomic_store_explicit(p,v,o)   __atomic_store_n((p),(v),(o))
#define uix_atomic_exchange_explicit(p,v,o) __atomic_exchange_n((p),(v),(o))
#define uix_atomic_fetch_add_explicit(p,v,o) __atomic_fetch_add((p),(v),(o))
#define uix_atomic_fetch_sub_explicit(p,v,o) __atomic_fetch_sub((p),(v),(o))
#define uix_atomic_fetch_and_explicit(p,v,o) __atomic_fetch_and((p),(v),(o))
#define uix_atomic_fetch_or_explicit(p,v,o)  __atomic_fetch_or ((p),(v),(o))
#define uix_atomic_compare_exchange_strong_explicit(p,e,d,s,f) \
    __atomic_compare_exchange_n((p),(e),(d),0,(s),(f))

#define uix_memory_order_relaxed __ATOMIC_RELAXED
#define uix_memory_order_acquire __ATOMIC_ACQUIRE
#define uix_memory_order_release __ATOMIC_RELEASE
#define uix_memory_order_acq_rel __ATOMIC_ACQ_REL
#define uix_memory_order_seq_cst __ATOMIC_SEQ_CST //Strongest memory order — guarantees global ordering across all threads

#define uix_atomic_load(p)          uix_atomic_load_explicit((p), uix_memory_order_seq_cst)
#define uix_atomic_store(p,v)       uix_atomic_store_explicit((p),(v), uix_memory_order_seq_cst) //Atomically stores value — sequentially consistent, prevents reordering
#define uix_atomic_fetch_add(p,v)   uix_atomic_fetch_add_explicit((p),(v), uix_memory_order_seq_cst) //Atomic add and return old value — maps to atomic_fetch_add()
#define uix_atomic_fetch_sub(p,v)   uix_atomic_fetch_sub_explicit((p),(v), uix_memory_order_seq_cst)
#define uix_atomic_thread_fence(o)  __atomic_thread_fence(o)



#endif /* End of __UIX_STDATOMIC__H */
/* ***This is End of file, there is no more line should be added after this line*** */
