#ifndef ARM64_EXCEPTIONS_H
#define ARM64_EXCEPTIONS_H
/*
 * arm64_exceptions.h — AArch64 exception model
 * Reference: ARMv8-A Architecture Reference Manual, Section D1
 *
 * AArch64 exception model (vs ARM32 8-entry vector table):
 *   - Four exception levels: EL0 (user), EL1 (OS), EL2 (VMM), EL3 (TF)
 *   - Exception types: Synchronous, IRQ, FIQ, SError
 *   - Vector table has 16 entries (4 types × 4 sources), each 128 bytes
 *   - VBAR_ELn holds the base of the vector table for ELn
 *   - ESR_ELn (Exception Syndrome Register) explains WHY exception taken
 *   - FAR_ELn (Fault Address Register) holds faulting VA
 *   - No ARM32-style FIQ separate register banking
 */

#include "arm64_types.h"

/* ── Exception Types ─────────────────────────────────────── */
typedef enum arm64_exception_type {
    ARM64_EXC_SYNC      = 0,   /* Synchronous (SVC, data abort, ...) */
    ARM64_EXC_IRQ       = 1,   /* Interrupt Request                  */
    ARM64_EXC_FIQ       = 2,   /* Fast Interrupt Request             */
    ARM64_EXC_SERROR    = 3,   /* System Error (async abort)         */
} arm64_exception_type_t;

/* ── Exception Sources (which EL generated the exception) ── */
typedef enum arm64_exception_source {
    ARM64_SRC_SAME_EL_SP0  = 0, /* Same EL using SP_EL0             */
    ARM64_SRC_SAME_EL_SPn  = 1, /* Same EL using SP_ELn             */
    ARM64_SRC_LOWER_AARCH64= 2, /* Lower EL using AArch64            */
    ARM64_SRC_LOWER_AARCH32= 3, /* Lower EL using AArch32            */
} arm64_exception_source_t;

/* Number of entries in vector table (4 types × 4 sources) */
#define ARM64_NUM_VECTORS   16u

/* ── ESR_ELn — Exception Syndrome Register ──────────────── */
/* Bits [31:26] EC = Exception Class */
#define ARM64_ESR_EC_SHIFT          26
#define ARM64_ESR_EC_MASK           (0x3Fu << ARM64_ESR_EC_SHIFT)
#define ARM64_ESR_ISS_MASK          0x1FFFFFFu

/* Exception Class values */
#define ARM64_EC_UNKNOWN            0x00  /* Unknown reason           */
#define ARM64_EC_WF                 0x01  /* WFI/WFE trapped          */
#define ARM64_EC_MCR_MRC            0x03  /* MCR/MRC to CP15 (AArch32)*/
#define ARM64_EC_SVC_AARCH32        0x11  /* SVC from AArch32         */
#define ARM64_EC_SVC_AARCH64        0x15  /* SVC from AArch64         */
#define ARM64_EC_HVC_AARCH64        0x16  /* HVC from AArch64         */
#define ARM64_EC_SMC_AARCH64        0x17  /* SMC from AArch64         */
#define ARM64_EC_SYS_INSTR          0x18  /* MSR/MRS/SYS trapped      */
#define ARM64_EC_INSTR_ABORT_LOW    0x20  /* Instr abort lower EL     */
#define ARM64_EC_INSTR_ABORT_SAME   0x21  /* Instr abort same EL      */
#define ARM64_EC_PC_ALIGN           0x22  /* PC alignment fault       */
#define ARM64_EC_DATA_ABORT_LOW     0x24  /* Data abort lower EL      */
#define ARM64_EC_DATA_ABORT_SAME    0x25  /* Data abort same EL       */
#define ARM64_EC_SP_ALIGN           0x26  /* SP alignment fault       */
#define ARM64_EC_FP_AARCH64         0x2C  /* FP exception AArch64     */
#define ARM64_EC_SERROR             0x2F  /* SError interrupt         */
#define ARM64_EC_BREAKPOINT_LOW     0x30  /* Breakpoint lower EL      */
#define ARM64_EC_BREAKPOINT_SAME    0x31  /* Breakpoint same EL       */
#define ARM64_EC_SOFTSS_LOW         0x32  /* Software step lower EL   */
#define ARM64_EC_SOFTSS_SAME        0x33  /* Software step same EL    */
#define ARM64_EC_WATCHPOINT_LOW     0x34  /* Watchpoint lower EL      */
#define ARM64_EC_WATCHPOINT_SAME    0x35  /* Watchpoint same EL       */
#define ARM64_EC_BRK                0x3C  /* BRK instruction AArch64  */

/* ── Exception handler function pointer ─────────────────── */
typedef void (*arm64_exc_handler_t)(arm64_exception_type_t   type,
                                     arm64_exception_source_t src,
                                     arm64_uint64_t           esr,
                                     arm64_addr_t             far,
                                     arm64_addr_t             elr);

/* ── Exception state structure ───────────────────────────── */
typedef struct arm64_exception_state {
    arm64_uint64_t  esr_el1;   /* Exception syndrome, EL1           */
    arm64_uint64_t  esr_el2;   /* Exception syndrome, EL2           */
    arm64_uint64_t  esr_el3;   /* Exception syndrome, EL3           */
    arm64_addr_t    far_el1;   /* Fault address, EL1                */
    arm64_addr_t    far_el2;   /* Fault address, EL2                */
    arm64_addr_t    far_el3;   /* Fault address, EL3                */
    arm64_addr_t    vbar_el1;  /* Vector base, EL1                  */
    arm64_addr_t    vbar_el2;  /* Vector base, EL2                  */
    arm64_addr_t    vbar_el3;  /* Vector base, EL3                  */
} arm64_exception_state_t;

/* ── Vector table entry address ─────────────────────────── */
#define ARM64_VECTOR_ADDR(vbar, type, src) \
    ((arm64_addr_t)(vbar) + \
     (arm64_uint64_t)(src) * 0x200ULL + \
     (arm64_uint64_t)(type) * 0x80ULL)

/* ── Exception function declarations ────────────────────── */
void arm64_exception_take   (arm64_exception_type_t   type,
                              arm64_exception_source_t src,
                              arm64_uint64_t           esr_val);
void arm64_exception_return (void);
void arm64_exception_set_handler(arm64_exception_type_t type,
                                  arm64_exc_handler_t    handler);

#endif /* ARM64_EXCEPTIONS_H */
