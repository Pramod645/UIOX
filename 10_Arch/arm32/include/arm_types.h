#ifndef ARM_TYPES_H
#define ARM_TYPES_H

/*
 * arm_types.h — Base types for ARM instruction set encoding
 * Reference: ARM Instruction Set (ARM DDI 0100)
 */

typedef unsigned char      arm_uint8_t;
typedef signed   char      arm_int8_t;
typedef unsigned short     arm_uint16_t;
typedef signed   short     arm_int16_t;
typedef unsigned int       arm_uint32_t;
typedef signed   int       arm_int32_t;
typedef unsigned long long arm_uint64_t;
typedef signed   long long arm_int64_t;

/* ARM word = 32 bits */
typedef arm_uint32_t arm_word_t;
typedef arm_uint32_t arm_addr_t;
typedef arm_uint32_t arm_instr_t;

/* register number: 0-15 */
typedef arm_uint8_t  arm_reg_t;

/* Shift amount: 0-31 */
typedef arm_uint8_t  arm_shift_t;

/* Immediate value */
typedef arm_uint32_t arm_imm_t;

/* Boolean */
typedef int arm_bool_t;
#define ARM_TRUE  1
#define ARM_FALSE 0

#ifndef NULL
#define NULL ((void*)0)
#endif

#endif /* ARM_TYPES_H */
