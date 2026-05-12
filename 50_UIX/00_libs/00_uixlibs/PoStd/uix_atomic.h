#ifndef __UIX_ATOMIC__H
#define __UIX_ATOMIC__H

#include "uix_types.h"

typedef struct { volatile uix_int32_t value; } uix_atomic_int_t;
typedef struct { volatile uix_int64_t value; } uix_atomic_long_t;
typedef struct { volatile void       *value; } uix_atomic_ptr_t;

#define UIX_MEMORY_ORDER_RELAXED  __ATOMIC_RELAXED
#define UIX_MEMORY_ORDER_ACQUIRE  __ATOMIC_ACQUIRE
#define UIX_MEMORY_ORDER_RELEASE  __ATOMIC_RELEASE
#define UIX_MEMORY_ORDER_ACQ_REL  __ATOMIC_ACQ_REL
#define UIX_MEMORY_ORDER_SEQ_CST  __ATOMIC_SEQ_CST

#define uix_atomic_load(p)          __atomic_load_n(&(p)->value,  __ATOMIC_SEQ_CST)
#define uix_atomic_store(p, v)      __atomic_store_n(&(p)->value, (v), __ATOMIC_SEQ_CST)
#define uix_atomic_add(p, v)        __atomic_fetch_add(&(p)->value, (v), __ATOMIC_SEQ_CST)
#define uix_atomic_sub(p, v)        __atomic_fetch_sub(&(p)->value, (v), __ATOMIC_SEQ_CST)
#define uix_atomic_and(p, v)        __atomic_fetch_and(&(p)->value, (v), __ATOMIC_SEQ_CST)
#define uix_atomic_or(p, v)         __atomic_fetch_or (&(p)->value, (v), __ATOMIC_SEQ_CST)
#define uix_atomic_xor(p, v)        __atomic_fetch_xor(&(p)->value, (v), __ATOMIC_SEQ_CST)
#define uix_atomic_exchange(p, v)   __atomic_exchange_n(&(p)->value, (v), __ATOMIC_SEQ_CST)
#define uix_atomic_cas(p, exp, des) \
    __atomic_compare_exchange_n(&(p)->value, (exp), (des), 0, \
                                __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

#define uix_memory_barrier()   __atomic_thread_fence(__ATOMIC_SEQ_CST)
#define uix_compiler_barrier() __asm__ __volatile__("" ::: "memory")

#define UIX_ATOMIC_INT_INIT(v)  { (v) }
#define UIX_ATOMIC_LONG_INIT(v) { (v) }
#define UIX_ATOMIC_PTR_INIT(v)  { (v) }

#endif /* __UIX_ATOMIC__H */

/* ***This is End of file, there is no more line should be added after this line*** */
