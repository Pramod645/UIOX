#ifndef CPU_DEBUG_H
#define CPU_DEBUG_H
/*
 * cpu_debug.h - CPU debug / breakpoint / watchpoint interface
 */
#include "cpu_types.h"
#include "cpu_context.h"

#define CPU_MAX_BREAKPOINTS  16
#define CPU_MAX_WATCHPOINTS  16

typedef enum cpu_bp_type {
    CPU_BP_EXEC  = 0,   /* instruction breakpoint               */
    CPU_BP_READ  = 1,   /* data watchpoint on read              */
    CPU_BP_WRITE = 2,   /* data watchpoint on write             */
    CPU_BP_RW    = 3,   /* data watchpoint on read or write     */
} cpu_bp_type_t;

typedef struct cpu_breakpoint {
    cpu_addr_t   addr;
    cpu_bp_type_t type;
    cpu_u32_t    byte_mask;   /* which bytes to watch             */
    cpu_bool_t   enabled;
} cpu_breakpoint_t;

typedef void (*cpu_debug_handler_t)(const cpu_context_t *ctx,
                                     cpu_addr_t addr,
                                     cpu_bp_type_t type);

/* -- API ---------------------------------------------------- */
int  cpu_debug_init         (void);
int  cpu_debug_set_bp       (cpu_u32_t slot, cpu_addr_t addr);
int  cpu_debug_clear_bp     (cpu_u32_t slot);
int  cpu_debug_set_wp       (cpu_u32_t slot, cpu_addr_t addr,
                              cpu_u32_t len, cpu_bp_type_t type);
int  cpu_debug_clear_wp     (cpu_u32_t slot);
void cpu_debug_enable       (void);
void cpu_debug_disable      (void);
void cpu_debug_register_handler(cpu_debug_handler_t h);
void cpu_debug_print_state  (void);
cpu_u32_t cpu_debug_num_bp  (void);   /* hardware BP count        */
cpu_u32_t cpu_debug_num_wp  (void);   /* hardware WP count        */

#endif /* CPU_DEBUG_H */
