#ifndef ARM64_OPCODES_H
#define ARM64_OPCODES_H
/*
 * arm64_opcodes.h — AArch64 instruction opcode constants
 * Reference: ARMv8-A Architecture Reference Manual, Chapter C3–C7
 *
 * AArch64 instruction encoding uses a fixed 32-bit word.
 * The top-level decode uses bits [28:25] as the primary group.
 */

#include "arm64_types.h"

/* ── Top-level instruction groups (bits [28:25]) ─────────── */
#define ARM64_GRP_RESERVED          0x0   /* [28:25]=0000              */
#define ARM64_GRP_DATA_PROC_IMM     0x4   /* [28:25]=100x (DPI)        */
#define ARM64_GRP_BRANCH_SYS
