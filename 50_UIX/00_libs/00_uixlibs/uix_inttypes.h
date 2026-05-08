
#ifndef __INTTYPES__H
#define __INTTYPES__H
/*
stddef.h
*/
/* This is for only STDLIB */

#include "features.h"

#if  (define __GLIBC)

#ifdef _cplusplus
extern "C" {
#endif



#ifdef cplusplus
}
#endif


#endif /* End  of STDLIB*/



#ifndef UIX_INTTYPES_H
#define UIX_INTTYPES_H

#include "uix_stdint.h"

/* Printf format macros */
#define UIX_PRId8   "d" // printf format for int32_t — maps to PRId32 from C99 <inttypes.h>
#define UIX_PRId16  "d"
#define UIX_PRId32  "d"
#define UIX_PRId64  "lld" // printf format for int64_t — ll prefix for long long
#define UIX_PRIu8   "u"
#define UIX_PRIu16  "u"
#define UIX_PRIu32  "u"
#define UIX_PRIu64  "llu"
#define UIX_PRIx8   "x"
#define UIX_PRIx16  "x"
#define UIX_PRIx32  "x"
#define UIX_PRIx64  "llx"
#define UIX_PRIX8   "X"
#define UIX_PRIX16  "X"
#define UIX_PRIX32  "X"
#define UIX_PRIX64  "llX"
#define UIX_PRIdMAX "lld"
#define UIX_PRIuMAX "llu"
#define UIX_PRIxMAX "llx"
#define UIX_PRIdPTR "ld"
#define UIX_PRIuPTR "lu"

/* Scanf format macros */
#define UIX_SCNd8   "hhd"
#define UIX_SCNd16  "hd"
#define UIX_SCNd32  "d" //scanf format for int32_t
#define UIX_SCNd64  "lld"
#define UIX_SCNu8   "hhu"
#define UIX_SCNu16  "hu"
#define UIX_SCNu32  "u" 
#define UIX_SCNu64  "llu"

/* Conversion functions */
uix_intmax_t  uix_imaxabs (uix_intmax_t j);
uix_intmax_t  uix_strtoimax(const char *s, char **ep, int base); //Converts string to intmax_t — POSIX strtoimax() from <inttypes.h>
uix_uintmax_t uix_strtoumax(const char *s, char **ep, int base);

#endif /* UIX_INTTYPES_H */



#endif /* End of __INTTYPES__H */
/* ***This is End of file, there is no more line should be added after this line*** */