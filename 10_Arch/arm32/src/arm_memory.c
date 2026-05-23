/*
 * arm_memory.c — ARM memory access implementation
 */

 #include "../include/arm_memory.h"

 /* Flat memory array (16 MB for simulation) */
 #define ARM_SIM_MEM_SIZE 0x01000000u
 static arm_uint8_t g_mem[ARM_SIM_MEM_SIZE];
 
 static arm_uint8_t *mem_ptr(arm_addr_t addr)
 {
     if (addr >= ARM_SIM_MEM_SIZE) return (arm_uint8_t*)0;
     return &g_mem[addr];
 }
 
 arm_word_t arm_mem_read_word(arm_addr_t addr)
 {
     addr &= ARM_WORD_ALIGN_MASK;
     arm_uint8_t *p = mem_ptr(addr);
     if (!p) return 0;
     /* little-endian */
     return (arm_word_t)p[0]
          | ((arm_word_t)p[1] <<  8)
          | ((arm_word_t)p[2] << 16)
          | ((arm_word_t)p[3] << 24);
 }
 
 arm_word_t arm_mem_read_half(arm_addr_t addr)
 {
     addr &= ARM_HALF_ALIGN_MASK;
     arm_uint8_t *p = mem_ptr(addr);
     if (!p) return 0;
     return (arm_word_t)p[0] | ((arm_word_t)p[1] << 8);
 }
 
 arm_word_t arm_mem_read_byte(arm_addr_t addr)
 {
     arm_uint8_t *p = mem_ptr(addr);
     return p ? (arm_word_t)*p : 0;
 }
 
 void arm_mem_write_word(arm_addr_t addr, arm_word_t val)
 {
     addr &= ARM_WORD_ALIGN_MASK;
     arm_uint8_t *p = mem_ptr(addr);
     if (!p) return;
     p[0] = (arm_uint8_t)(val);
     p[1] = (arm_uint8_t)(val >>  8);
     p[2] = (arm_uint8_t)(val >> 16);
     p[3] = (arm_uint8_t)(val >> 24);
 }
 
 void arm_mem_write_half(arm_addr_t addr, arm_word_t val)
 {
     addr &= ARM_HALF_ALIGN_MASK;
     arm_uint8_t *p = mem_ptr(addr);
     if (!p) return;
     p[0] = (arm_uint8_t)(val);
     p[1] = (arm_uint8_t)(val >> 8);
 }
 
 void arm_mem_write_byte(arm_addr_t addr, arm_word_t val)
 {
     arm_uint8_t *p = mem_ptr(addr);
     if (p) *p = (arm_uint8_t)val;
 }
 