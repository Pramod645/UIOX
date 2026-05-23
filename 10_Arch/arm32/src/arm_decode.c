/*
 * arm_decode.c — ARM instruction decoder
 * Classifies a 32-bit instruction word into its format type.
 */

 #include "../include/arm_arch.h"

 typedef enum arm_instr_class {
     ARM_CLASS_DP_REG,       /* Data processing, register op2    */
     ARM_CLASS_DP_IMM,       /* Data processing, immediate op2   */
     ARM_CLASS_MUL_SHORT,    /* 32-bit multiply                  */
     ARM_CLASS_MUL_LONG,     /* 64-bit multiply                  */
     ARM_CLASS_SWP_INSTR,    /* Data swap                        */
     ARM_CLASS_BRANCH_INSTR, /* B / BL                           */
     ARM_CLASS_BX_INSTR,     /* Branch and exchange              */
     ARM_CLASS_SDT_INSTR,    /* Single data transfer             */
     ARM_CLASS_HWDT_INSTR,   /* Halfword data transfer           */
     ARM_CLASS_BDT_INSTR,    /* Block data transfer              */
     ARM_CLASS_PSR_INSTR,    /* PSR transfer                     */
     ARM_CLASS_SWI_INSTR,    /* Software interrupt               */
     ARM_CLASS_CDP_INSTR,    /* Coprocessor data processing      */
     ARM_CLASS_CDT_INSTR,    /* Coprocessor data transfer        */
     ARM_CLASS_CRT_INSTR,    /* Coprocessor register transfer    */
     ARM_CLASS_UNDEF_INSTR,  /* Undefined                        */
 } arm_instr_class_t;
 
 arm_instr_class_t arm_decode(arm_instr_t instr)
 {
     arm_uint32_t b27_26 = (instr >> 26) & 0x3u;
     arm_uint32_t b25    = (instr >> 25) & 0x1u;
     arm_uint32_t b24    = (instr >> 24) & 0x1u;
     arm_uint32_t b27_24 = (instr >> 24) & 0xFu;
     arm_uint32_t b7_4   = (instr >>  4) & 0xFu;
     arm_uint32_t b27_20 = (instr >> 20) & 0xFFu;
 
     /* SWI: bits[27:24] = 1111 */
     if (b27_24 == 0xFu)
         return ARM_CLASS_SWI_INSTR;
 
     /* Coprocessor: bits[27:26] = 11 */
     if (b27_26 == 0x3u) {
         if (b25 == 0u)
             return ARM_CLASS_CDT_INSTR;
         if ((instr >> 4) & 1u)
             return ARM_CLASS_CRT_INSTR;
         return ARM_CLASS_CDP_INSTR;
     }
 
     /* Branch: bits[27:25] = 101 */
     if (b27_26 == 0x2u && b25 == 0x1u)
         return ARM_CLASS_BRANCH_INSTR;
 
     /* Block data transfer: bits[27:25] = 100 */
     if (b27_26 == 0x2u && b25 == 0x0u)
         return ARM_CLASS_BDT_INSTR;
 
     /* Single data transfer: bits[27:26] = 01 */
     if (b27_26 == 0x1u)
         return ARM_CLASS_SDT_INSTR;
 
     /* bits[27:26] = 00 */
 
     /* Undefined: bits[27:25]=011 bit[4]=1 */
     if ((instr >> 25 & 0x7u) == 0x3u && (instr >> 4 & 1u))
         return ARM_CLASS_UNDEF_INSTR;
 
     /* Multiply long: bits[27:23] = 00001 */
     if ((b27_20 >> 3) == 0x1u && b7_4 == 0x9u)
         return ARM_CLASS_MUL_LONG;
 
     /* Multiply: bits[27:22] = 000000, bits[7:4]=1001 */
     if ((b27_20 >> 2) == 0x0u && b7_4 == 0x9u)
         return ARM_CLASS_MUL_SHORT;
 
     /* Data swap: bits[27:23]=00010, bits[11:4]=00001001 */
     if ((b27_20 >> 3) == 0x2u && b7_4 == 0x9u)
         return ARM_CLASS_SWP_INSTR;
 
     /* BX: bits[27:4] = 000100101111111111110001 */
     if ((instr & 0x0FFFFFF0u) == 0x012FFF10u)
         return ARM_CLASS_BX_INSTR;
 
     /* PSR: bits[27:23]=00010, bit[7]=0, bit[4]=0 */
     if ((b27_20 >> 3) == 0x2u && !(b7_4 & 0x1u))
         return ARM_CLASS_PSR_INSTR;
 
     /* Halfword data transfer: bits[27:25]=000, bit[7]=1, bit[4]=1 */
     if (b27_26 == 0x0u && b25 == 0x0u &&
         (instr >> 7 & 1u) && (instr >> 4 & 1u))
         return ARM_CLASS_HWDT_INSTR;
 
     /* Data processing */
     if (b25)
         return ARM_CLASS_DP_IMM;
     return ARM_CLASS_DP_REG;
 }
 