#ifndef UIOX_LD_SCRIPT_H
#define UIOX_LD_SCRIPT_H
/*
 * uiox_ld_script.h - UIOX linker script parser
 * Supports a simplified GNU ld-compatible linker script syntax:
 *
 *   ENTRY(_start)
 *   MEMORY { ROM (rx) : ORIGIN = 0x0, LENGTH = 1M }
 *   SECTIONS {
 *       .text : { *(.text*) } > ROM
 *       .data : { *(.data*) } > RAM
 *       .bss  (NOLOAD) : { *(.bss*) } > RAM
 *   }
 */
#include "uiox_ld_types.h"
#include "uiox_ld_diag.h"

#define ULD_SCRIPT_MAX_REGIONS   16
#define ULD_SCRIPT_MAX_SECTIONS  64
#define ULD_SCRIPT_MAX_CMDS      256

/* -- Memory region ------------------------------------------ */
typedef struct uld_mem_region {
    char       name[ULD_NAME_MAX];
    uld_u32_t  flags;              /* rx / rwx / r                */
    uld_addr_t origin;
    uld_u64_t  length;
    uld_addr_t current;            /* current allocation pointer  */
} uld_mem_region_t;

/* -- Section placement command ------------------------------ */
typedef struct uld_sect_cmd {
    char       out_name[ULD_NAME_MAX];   /* output section name   */
    char       region[ULD_NAME_MAX];     /* memory region (> ROM) */
    uld_addr_t at_addr;                  /* explicit load addr     */
    uld_bool_t noload;
    uld_bool_t has_at;
    /* input section patterns */
    char       patterns[16][ULD_NAME_MAX];
    uld_u32_t  pattern_count;
} uld_sect_cmd_t;

/* -- Parsed linker script ----------------------------------- */
typedef struct uld_script {
    char              entry_sym[ULD_NAME_MAX];
    uld_mem_region_t  regions[ULD_SCRIPT_MAX_REGIONS];
    uld_u32_t         region_count;
    uld_sect_cmd_t    sect_cmds[ULD_SCRIPT_MAX_SECTIONS];
    uld_u32_t         sect_cmd_count;
    uld_bool_t        has_entry;
} uld_script_t;

int  uld_script_parse    (uld_script_t *sc, const char *path,
                           uld_diag_ctx_t *diag);
int  uld_script_parse_str(uld_script_t *sc, const char *text,
                           uld_diag_ctx_t *diag);
void uld_script_free     (uld_script_t *sc);
void uld_script_print    (const uld_script_t *sc);

/* Built-in default scripts for each arch */
const char *uld_script_default_x86_64(void);
const char *uld_script_default_arm64 (void);
const char *uld_script_default_arm32 (void);

#endif /* UIOX_LD_SCRIPT_H */
