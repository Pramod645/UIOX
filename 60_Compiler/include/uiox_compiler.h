#ifndef UIOX_COMPILER_H
#define UIOX_COMPILER_H
/*
 * uiox_compiler.h - UIOX compiler master include and driver API
 */
#include "uiox_error.h"
#include "uiox_token.h"
#include "uiox_lexer.h"
#include "uiox_ast.h"
#include "uiox_parser.h"
#include "uiox_symtab.h"
#include "uiox_ir.h"
#include "uiox_codegen.h"
#include "uiox_regalloc.h"
#include "uiox_emit.h"
#include "uiox_object.h"
#include "uiox_section.h"
#include "uiox_linker.h"

#define UIOX_COMPILER_NAME     "uioxcc"
#define UIOX_COMPILER_VERSION  "0.1.0"

/* -- Compiler options --------------------------------------- */
typedef struct uiox_options {
    const char        *input_file;
    const char        *output_file;
    const char        *linker_script;
    const char        *entry_sym;
    uiox_target_arch_t arch;
    uiox_lnk_output_t  output_fmt;
    int                opt_level;      /* 0=none, 1=basic, 2=full */
    int                debug_info;     /* 1=emit debug sections   */
    int                verbose;
    int                stop_at_ir;     /* dump IR and stop        */
    int                stop_at_asm;    /* dump asm and stop       */
    int                stop_at_obj;    /* write .uobj and stop    */
    int                link_only;      /* input=.uobj, output=elf */
    int                warn_as_error;
} uiox_options_t;

/* -- Compiler pipeline context ------------------------------ */
typedef struct uiox_compiler {
    uiox_options_t  opts;
    uiox_diag_ctx_t diag;
    uiox_lexer_t    lexer;
    uiox_ast_node_t *ast;
    uiox_codegen_t  codegen;
    uiox_object_t   obj;
    uiox_linker_t   linker;
    char            src_buf[1024 * 1024]; /* 1 MB source buffer  */
    int             src_len;
} uiox_compiler_t;

/* -- Driver API --------------------------------------------- */
void uiox_options_default  (uiox_options_t *opts);
int  uiox_options_parse    (uiox_options_t *opts, int argc, char **argv);
void uiox_options_print    (const uiox_options_t *opts);

int  uiox_compiler_init    (uiox_compiler_t *cc, const uiox_options_t *opts);
void uiox_compiler_free    (uiox_compiler_t *cc);
int  uiox_compiler_run     (uiox_compiler_t *cc);

/* Pipeline stages (called in order by uiox_compiler_run) */
int  uiox_stage_read       (uiox_compiler_t *cc); /* read source file  */
int  uiox_stage_lex        (uiox_compiler_t *cc); /* tokenise          */
int  uiox_stage_parse      (uiox_compiler_t *cc); /* build AST         */
int  uiox_stage_sema       (uiox_compiler_t *cc); /* semantic analysis */
int  uiox_stage_irgen      (uiox_compiler_t *cc); /* AST -> IR         */
int  uiox_stage_optir      (uiox_compiler_t *cc); /* IR optimise       */
int  uiox_stage_regalloc   (uiox_compiler_t *cc); /* register alloc    */
int  uiox_stage_emit       (uiox_compiler_t *cc); /* emit machine code */
int  uiox_stage_link       (uiox_compiler_t *cc); /* link to ELF       */

#endif /* UIOX_COMPILER_H */
