/*
 * arm64_memory.c — AArch64 memory access implementation
 */
#include "../include/arm64_memory.h"

/* Flat memory array (64 MB for simulation) */
#define ARM64_SIM_MEM_SIZE  0x04000000ULL

static arm64_uint8_t g_mem[ARM64_SIM_MEM_SIZE];

static arm64_uint8_t *mem_ptr(arm64_addr_t addr)
{
    if (addr >= ARM64_SIM_MEM_SIZE) return (arm64_uint8_t *)0;
    return &g_mem[addr];
}

/* ── 64-bit (qword) ──────────────────────────────────────── */
arm64_word_t arm64_mem_read_qword(arm64_addr_t addr)
{
    addr &= ARM64_QWORD_ALIGN_MASK;
    arm64_uint8_t *p = mem_ptr(addr);
    if (!p) return 0;
    return  (arm64_word_t)p[0]
          | ((arm64_word_t)p[1] <<  8)
          | ((arm64_word_t)p[2] << 16)
          | ((arm64_word_t)p[3] << 24)
          | ((arm64_word_t)p[4] << 32)
          | ((arm64_word_t)p[5] << 40)
          | ((arm64_word_t)p[6] << 48)
          | ((arm64_word_t)p[7] << 56);
}

void arm64_mem_write_qword(arm64_addr_t addr, arm64_word_t val)
{
    addr &= ARM64_QWORD_ALIGN_MASK;
    arm64_uint8_t *p = mem_ptr(addr);
    if (!p) return;
    p[0] = (arm64_uint8_t)(val);
    p[1] = (arm64_uint8_t)(val >>  8);
    p[2] = (arm64_uint8_t)(val >> 16);
    p[3] = (arm64_uint8_t)(val >> 24);
    p[4] = (arm64_uint8_t)(val >> 32);
    p[5] = (arm64_uint8_t)(val >> 40);
    p[6] = (arm64_uint8_t)(val >> 48);
    p[7] = (arm64_uint8_t)(val >> 56);
}

/* ── 32-bit (dword) ──────────────────────────────────────── */
arm64_word_t arm64_mem_read_dword(arm64_addr_t addr)
{
    addr &= ARM64_DWORD_ALIGN_MASK;
    arm64_uint8_t *p = mem_ptr(addr);
    if (!p) return 0;
    return  (arm64_word_t)p[0]
          | ((arm64_word_t)p[1] <<  8)
          | ((arm64_word_t)p[2] << 16)
          | ((arm64_word_t)p[3] << 24);
}

void arm64_mem_write_dword(arm64_addr_t addr, arm64_word_t val)
{
    addr &= ARM64_DWORD_ALIGN_MASK;
    arm64_uint8_t *p = mem_ptr(addr);
    if (!p) return;
    p[0] = (arm64_uint8_t)(val);
    p[1] = (arm64_uint8_t)(val >>  8);
    p[2] = (arm64_uint8_t)(val >> 16);
    p[3] = (arm64_uint8_t)(val >> 24);
}

/* ── 16-bit (word) ───────────────────────────────────────── */
arm64_word_t arm64_mem_read_word(arm64_addr_t addr)
{
    addr &= ARM64_WORD_ALIGN_MASK;
    arm64_uint8_t *p = mem_ptr(addr);
    if (!p) return 0;
    return (arm64_word_t)p[0] | ((arm64_word_t)p[1] << 8);
}

void arm64_mem_write_word(arm64_addr_t addr, arm64_word_t val)
{
    addr &= ARM64_WORD_ALIGN_MASK;
    arm64_uint8_t *p = mem_ptr(addr);
    if (!p) return;
    p[0] = (arm64_uint8_t)(val);
    p[1] = (arm64_uint8_t)(val >> 8);
}

/* ── 8-bit (byte) ────────────────────────────────────────── */
arm64_word_t arm64_mem_read_byte(arm64_addr_t addr)
{
    arm64_uint8_t *p = mem_ptr(addr);
    return p ? (arm64_word_t)*p : 0;
}

void arm64_mem_write_byte(arm64_addr_t addr, arm64_word_t val)
{
    arm64_uint8_t *p = mem_ptr(addr);
    if (p) *p = (arm64_uint8_t)val;
}
