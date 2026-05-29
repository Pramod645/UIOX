/*
 * uiox_main.c - UIOX compiler/linker driver
 * Usage:
 *   uioxcc [options] <input.c> -o <output>
 *
 *   -o <file>          output file
 *   -arch arm64        target architecture (arm64/arm32/x86_64)
 *   -fmt elf64         output format (elf64/elf32/flat/ihex)
 *   -O0 / -O2          optimisation level
 *   -g                 emit debug info
 *   -S                 stop after IR dump
 *   -c                 compile only, write .uobj
 *   -v                 verbose
 *   -T <script>        linker script
 *   -e <sym>           entry symbol (default: _start)
 */
#include "../include/uiox_compiler.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void uiox_options_default(uiox_options_t *opts)
{
    memset(opts, 0, sizeof(*opts));
    opts->output_file  = "uiox.out";
    opts->entry_sym    = "_start";
    opts->arch         = UIOX_TARGET_X86_64;
    opts->output_fmt   = UIOX_LNK_ELF64;
    opts->opt_level    = 2;
}

int uiox_options_parse(uiox_options_t *opts, int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i+1 < argc) {
            opts->output_file = argv[++i];
        } else if (strcmp(argv[i], "-arch") == 0 && i+1 < argc) {
            const char *a = argv[++i];
            if      (strcmp(a, "arm64")  == 0) opts->arch = UIOX_TARGET_ARM64;
            else if (strcmp(a, "arm32")  == 0) opts->arch = UIOX_TARGET_ARM32;
            else if (strcmp(a, "x86_64") == 0) opts->arch = UIOX_TARGET_X86_64;
            else { fprintf(stderr, "unknown arch: %s\n", a); return -1; }
        } else if (strcmp(argv[i], "-fmt") == 0 && i+1 < argc) {
            const char *fm = argv[++i];
            if      (strcmp(fm, "elf64") == 0) opts->output_fmt = UIOX_LNK_ELF64;
            else if (strcmp(fm, "elf32") == 0) opts->output_fmt = UIOX_LNK_ELF32;
            else if (strcmp(fm, "flat")  == 0) opts->output_fmt = UIOX_LNK_FLAT;
            else if (strcmp(fm, "ihex")  == 0) opts->output_fmt = UIOX_LNK_IHEX;
        } else if (strcmp(argv[i], "-T") == 0 && i+1 < argc) {
            opts->linker_script = argv[++i];
        } else if (strcmp(argv[i], "-e") == 0 && i+1 < argc) {
            opts->entry_sym = argv[++i];
        } else if (strcmp(argv[i], "-O0") == 0) {
            opts->opt_level = 0;
        } else if (strcmp(argv[i], "-O2") == 0) {
            opts->opt_level = 2;
        } else if (strcmp(argv[i], "-g") == 0) {
            opts->debug_info = 1;
        } else if (strcmp(argv[i], "-S") == 0) {
            opts->stop_at_ir = 1;
        } else if (strcmp(argv[i], "-c") == 0) {
            opts->stop_at_obj = 1;
        } else if (strcmp(argv[i], "-v") == 0) {
            opts->verbose = 1;
        } else if (strcmp(argv[i], "-Werror") == 0) {
            opts->warn_as_error = 1;
        } else if (argv[i][0] != '-') {
            opts->input_file = argv[i];
        } else {
            fprintf(stderr, "unknown option: %s\n", argv[i]);
            return -1;
        }
    }
    if (!opts->input_file) {
        fprintf(stderr, "uioxcc: no input file\n");
        return -1;
    }
    return 0;
}

void uiox_options_print(const uiox_options_t *opts)
{
    printf("uioxcc %s\n", UIOX_COMPILER_VERSION);
    printf("  input  : %s\n", opts->input_file);
    printf("  output : %s\n", opts->output_file);
    printf("  arch   : %d\n", opts->arch);
    printf("  format : %d\n", opts->output_fmt);
    printf("  opt    : -O%d\n", opts->opt_level);
    printf("  entry  : %s\n", opts->entry_sym);
}

int uiox_compiler_init(uiox_compiler_t *cc, const uiox_options_t *opts)
{
    memset(cc, 0, sizeof(*cc));
    cc->opts = *opts;
    uiox_diag_init(&cc->diag);
    return 0;
}

void uiox_compiler_free(uiox_compiler_t *cc)
{
    uiox_ast_free(cc->ast);
    uiox_codegen_free(&cc->codegen);
    uiox_object_free(&cc->obj);
    uiox_linker_free(&cc->linker);
    uiox_diag_free(&cc->diag);
}

int uiox_stage_read(uiox_compiler_t *cc)
{
    FILE *f = fopen(cc->opts.input_file, "rb");
    if (!f) {
        UIOX_FATAL(&cc->diag, cc->opts.input_file, 0, 0,
                   "cannot open input file");
        return -1;
    }
    cc->src_len = (int)fread(cc->src_buf, 1,
                             sizeof(cc->src_buf) - 1, f);
    fclose(f);
    cc->src_buf[cc->src_len] = '\0';
    if (cc->opts.verbose)
        printf("[read] %d bytes from %s\n",
               cc->src_len, cc->opts.input_file);
    return 0;
}

int uiox_stage_lex(uiox_compiler_t *cc)
{
    uiox_lexer_init(&cc->lexer, cc->src_buf, cc->src_len,
                    cc->opts.input_file, &cc->diag);
    if (cc->opts.verbose) printf("[lex] done\n");
    return 0;
}

int uiox_stage_parse(uiox_compiler_t *cc)
{
    uiox_parser_t p;
    uiox_parser_init(&p, &cc->lexer, &cc->diag);
    cc->ast = uiox_parse_translation_unit(&p);
    uiox_parser_free(&p);
    if (uiox_diag_has_error(&cc->diag)) return -1;
    if (cc->opts.verbose) {
        printf("[parse] AST:\n");
        uiox_ast_print(cc->ast, 0);
    }
    return 0;
}

int uiox_stage_sema(uiox_compiler_t *cc)
{
    /* Semantic analysis placeholder — type checking done
       during codegen for this version */
    if (cc->opts.verbose) printf("[sema] done\n");
    return 0;
}

int uiox_stage_irgen(uiox_compiler_t *cc)
{
    uiox_codegen_init(&cc->codegen, cc->opts.arch, &cc->diag);
    uiox_codegen_run(&cc->codegen, cc->ast, &cc->obj);
    if (uiox_diag_has_error(&cc->diag)) return -1;
    if (cc->opts.stop_at_ir || cc->opts.verbose) {
        printf("[ir]\n");
        uiox_ir_module_print(&cc->codegen.ir);
    }
    if (cc->opts.stop_at_ir) return 1; /* stop here */
    return 0;
}

int uiox_stage_optir(uiox_compiler_t *cc)
{
    /* IR optimisation placeholder (dead-code elim, const-fold) */
    if (cc->opts.verbose) printf("[optir] level=%d\n", cc->opts.opt_level);
    return 0;
}

int uiox_stage_regalloc(uiox_compiler_t *cc)
{
    const uiox_phys_reg_t *pregs;
    int npregs;
    switch (cc->opts.arch) {
        case UIOX_TARGET_ARM64:
            pregs  = uiox_regs_arm64;
            npregs = uiox_nregs_arm64;
            break;
        case UIOX_TARGET_ARM32:
            pregs  = uiox_regs_arm32;
            npregs = uiox_nregs_arm32;
            break;
        default:
            pregs  = uiox_regs_x86_64;
            npregs = uiox_nregs_x86_64;
            break;
    }
    uiox_regalloc_init(&cc->codegen.ra, pregs, npregs);
    for (uiox_ir_func_t *f = cc->codegen.ir.funcs; f; f = f->next)
        uiox_regalloc_run(&cc->codegen.ra, f);
    if (cc->opts.verbose) printf("[regalloc] done\n");
    return 0;
}

int uiox_stage_emit(uiox_compiler_t *cc)
{
    uiox_object_init(&cc->obj, cc->opts.input_file,
                     (unsigned int)cc->opts.arch);
    uiox_emit_init(&cc->codegen.emitter,
                   cc->opts.arch, &cc->obj,
                   &cc->codegen.ra);
    uiox_emit_module(&cc->codegen.emitter, &cc->codegen.ir);
    if (cc->opts.stop_at_obj) {
        char objpath[512];
        snprintf(objpath, sizeof(objpath), "%s.uobj",
                 cc->opts.input_file);
        uiox_object_write(&cc->obj, objpath);
        printf("[emit] object: %s\n", objpath);
        return 1;
    }
    if (cc->opts.verbose) printf("[emit] done\n");
    return 0;
}

int uiox_stage_link(uiox_compiler_t *cc)
{
    uiox_linker_init(&cc->linker, &cc->diag,
                     cc->opts.arch, cc->opts.output_fmt);
    uiox_linker_set_entry (&cc->linker, cc->opts.entry_sym);
    uiox_linker_set_output(&cc->linker, cc->opts.output_file);
    uiox_linker_add_object(&cc->linker, &cc->obj);
    int rc = uiox_linker_run(&cc->linker);
    if (cc->opts.verbose) {
        uiox_object_print(&cc->obj);
        printf("[link] rc=%d\n", rc);
    }
    return rc;
}

int uiox_compiler_run(uiox_compiler_t *cc)
{
    int rc;
    if ((rc = uiox_stage_read    (cc)) != 0) goto done;
    if ((rc = uiox_stage_lex     (cc)) != 0) goto done;
    if ((rc = uiox_stage_parse   (cc)) != 0) goto done;
    if ((rc = uiox_stage_sema    (cc)) != 0) goto done;
    if ((rc = uiox_stage_irgen   (cc)) != 0) goto done;
    if ((rc = uiox_stage_optir   (cc)) != 0) goto done;
    if ((rc = uiox_stage_regalloc(cc)) != 0) goto done;
    if ((rc = uiox_stage_emit    (cc)) != 0) goto done;
    if ((rc = uiox_stage_link    (cc)) != 0) goto done;
done:
    uiox_diag_print(&cc->diag);
    return uiox_diag_has_error(&cc->diag) ? 1 : 0;
}

int main(int argc, char **argv)
{
    uiox_options_t opts;
    uiox_options_default(&opts);

    if (uiox_options_parse(&opts, argc, argv) < 0) {
        fprintf(stderr,
            "Usage: uioxcc [options] <input.c> -o <output>\n"
            "  -arch arm64|arm32|x86_64\n"
            "  -fmt  elf64|elf32|flat|ihex\n"
            "  -O0 / -O2\n"
            "  -g          debug info\n"
            "  -S          dump IR and stop\n"
            "  -c          compile only (.uobj)\n"
            "  -T <script> linker script\n"
            "  -e <sym>    entry symbol\n"
            "  -v          verbose\n"
            "  -Werror     warnings as errors\n");
        return 1;
    }

    if (opts.verbose) uiox_options_print(&opts);

    uiox_compiler_t cc;
    uiox_compiler_init(&cc, &opts);
    int rc = uiox_compiler_run(&cc);
    uiox_compiler_free(&cc);
    
    return rc;
}
