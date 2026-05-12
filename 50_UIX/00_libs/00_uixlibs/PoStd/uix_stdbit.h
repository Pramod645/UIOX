
#ifndef __UIX_STDBIT__H
#define __UIX_STDBIT__H
/*
stddef.h
*/
/* This is for only STDLIB */

//#include "uix_features.h"


#include "sys/uix_types.h"

UIX_INLINE int uix_popcount32(uix_uint32_t v)  { return __builtin_popcount(v); } //Count set bits — maps to __builtin_popcount(), used in hash functions
UIX_INLINE int uix_popcount64(uix_uint64_t v)  { return __builtin_popcountll(v); }
UIX_INLINE int uix_clz32(uix_uint32_t v)       { return v ? __builtin_clz(v)   : 32; } //Count leading zeros — maps to __builtin_clz(), used in log2 calculations
UIX_INLINE int uix_clz64(uix_uint64_t v)       { return v ? __builtin_clzll(v) : 64; }
UIX_INLINE int uix_ctz32(uix_uint32_t v)       { return v ? __builtin_ctz(v)   : 32; } //Count trailing zeros — maps to __builtin_ctz(), used in lowest-set-bit operations
UIX_INLINE int uix_ctz64(uix_uint64_t v)       { return v ? __builtin_ctzll(v) : 64; }

UIX_INLINE int uix_stdc_bit_width(uix_uint64_t v)//Minimum bits to represent value — C23 stdc_bit_width()
    { return 64 - uix_clz64(v); }

UIX_INLINE uix_uint32_t uix_rotl32(uix_uint32_t v, int n) //Rotate left — bitwise rotation used in cryptography and hashing
    { return (v<<(n&31))|(v>>(32-(n&31))); }
UIX_INLINE uix_uint32_t uix_rotr32(uix_uint32_t v, int n)
    { return (v>>(n&31))|(v<<(32-(n&31))); }
UIX_INLINE uix_uint64_t uix_rotl64(uix_uint64_t v, int n)
    { return (v<<(n&63))|(v>>(64-(n&63))); }
UIX_INLINE uix_uint64_t uix_rotr64(uix_uint64_t v, int n)
    { return (v>>(n&63))|(v<<(64-(n&63))); }

#ifndef UIX_INLINE
#define UIX_INLINE static inline
#endif


#endif /* End of __UIX_STDBIT__H */
/* ***This is End of file, there is no more line should be added after this line*** */
