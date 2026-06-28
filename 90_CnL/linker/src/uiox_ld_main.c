/*
 * uiox_ld_main.c - uioxld driver entry point
 *
 * Usage:
 *   uioxld [options] file1.o file2.o ... -o output
 *
 *   -o  <file>         output file         (default: uiox.out)
 *   -arch <arch>       arm64|arm32|x86_64  (default: x86_64)
 *   -fmt  <fmt>        elf64|elf32|flat|ihex|srec
 *   -T  <script>       linker script file
 *   -e  <sym>          entry symbol        (default: _start)
 *   -Map <file>        write map file
 *   -Ttext <addr>      override .text base address (hex)
 *   -l  <archive.a>    add static archive
 *   --gc-sections      remove unused sections
 *   --strip-debug      remove debug sections
 *   -v                 verbose output
 *   --warn-undef       warn on undefined symbols (not error)
 *   --version          print version and exit
 *   --help             print usage and exit
 */
#include "../include/uiox_linker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UIOXLD_VERSION "0.1.0"

/* ── Defaults ──────────────────────────────────────────────── */
void uld_options_default(uld_options_t *opts)
{
    memset(opts, 0, sizeof(*opts));
    opts->output_path  = "uiox.out";
    opts->entry_sym    = "_start";
    opts->arch         = ULD_ARCH_X86_64;
    opts->fmt          = ULD_FMT_ELF64;
    opts->endian       = ULD_ENDIAN_LITTLE;
    opts->gc_sections  = ULD_FALSE;
    opts->strip_debug  = ULD_FALSE;
    opts->verbose      = ULD_FALSE;
    opts->print_map    = ULD_FALSE;
    opts->warn_undef   = ULD_FALSE;
    opts->text_base    = 0;
}

/* ── CLI parser ────────────────────────────────────────────── */
int uld_options_parse(uld_options_t *opts, int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        /* ── output file ─────────────────────────────────── */
        if (strcmp(argv[i], "-o") == 0 && i+1 < argc) {
            opts->output_path = argv[++i];

        /* ── target architecture ─────────────────────────── */
        } else if (strcmp(argv[i], "-arch") == 0 && i+1 < argc) {
            const char *a = argv[++i];
            if      (strcmp(a, "arm64")  == 0) opts->arch = ULD_ARCH_ARM64;
            else if (strcmp(a, "arm32")  == 0) opts->arch = ULD_ARCH_ARM32;
            else if (strcmp(a, "x86_64") == 0) opts->arch = ULD_ARCH_X86_64;
            else {
                fprintf(stderr, "uioxld: unknown arch '%s'\n", a);
                return -1;
            }

        /* ── output format ───────────────────────────────── */
        } else if (strcmp(argv[i], "-fmt") == 0 && i+1 < argc) {
            const char *fm = argv[++i];
            if      (strcmp(fm, "elf64") == 0) opts->fmt = ULD_FMT_ELF64;
            else if (strcmp(fm, "elf32") == 0) opts->fmt = ULD_FMT_ELF32;
            else if (strcmp(fm, "flat")  == 0) opts->fmt = ULD_FMT_FLAT;
            else if (strcmp(fm, "ihex")  == 0) opts->fmt = ULD_FMT_IHEX;
            else if (strcmp(fm, "srec")  == 0) opts->fmt = ULD_FMT_SREC;
            else {
                fprintf(stderr, "uioxld: unknown format '%s'\n", fm);
                return -1;
            }

        /* ── linker script ───────────────────────────────── */
        } else if (strcmp(argv[i], "-T") == 0 && i+1 < argc) {
            opts->script_path = argv[++i];

        /* ── entry symbol ────────────────────────────────── */
        } else if (strcmp(argv[i], "-e") == 0 && i+1 < argc) {
            opts->entry_sym = argv[++i];

        /* ── map file ────────────────────────────────────── */
        } else if (strcmp(argv[i], "-Map") == 0 && i+1 < argc) {
            opts->map_path  = argv[++i];
            opts->print_map = ULD_TRUE;

        /* ── .text base address ──────────────────────────── */
        } else if (strcmp(argv[i], "-Ttext") == 0 && i+1 < argc) {
            const char *s = argv[++i];
            if (s[0]=='0' && (s[1]=='x'||s[1]=='X'))
                opts->text_base = (uld_addr_t)strtoull(s+2, NULL, 16);
            else
                opts->text_base = (uld_addr_t)strtoull(s, NULL, 10);

        /* ── static archive ──────────────────────────────── */
        } else if (strcmp(argv[i], "-l") == 0 && i+1 < argc) {
            if (opts->ar_count < ULD_MAX_ARCHIVES)
                opts->ar_paths[opts->ar_count++] = argv[++i];

        /* ── flags ───────────────────────────────────────── */
        } else if (strcmp(argv[i], "--gc-sections") == 0) {
            opts->gc_sections = ULD_TRUE;
        } else if (strcmp(argv[i], "--strip-debug") == 0) {
            opts->strip_debug = ULD_TRUE;
        } else if (strcmp(argv[i], "-v") == 0 ||
                   strcmp(argv[i], "--verbose") == 0) {
            opts->verbose = ULD_TRUE;
        } else if (strcmp(argv[i], "--warn-undef") == 0) {
            opts->warn_undef = ULD_TRUE;

        /* ── version ─────────────────────────────────────── */
        } else if (strcmp(argv[i], "--version") == 0) {
            printf("uioxld %s\n", UIOXLD_VERSION);
            printf("UIOX Linker — supports ELF64/ELF32/flat/IHEX/SREC\n");
            printf("Targets: x86_64 / arm64 / arm32\n");
            exit(0);

        /* ── help ────────────────────────────────────────── */
        } else if (strcmp(argv[i], "--help") == 0 ||
                   strcmp(argv[i], "-h") == 0) {
            printf(
                "Usage: uioxld [options] file.o ... -o output\n"
                "\n"
                "Options:\n"
                "  -o  <file>         output file (default: uiox.out)\n"
                "  -arch <arch>       arm64 | arm32 | x86_64\n"
                "  -fmt  <fmt>        elf64 | elf32 | flat | ihex | srec\n"
                "  -T  <script>       linker script\n"
                "  -e  <sym>          entry symbol (default: _start)\n"
                "  -Map <file>        write map file\n"
                "  -Ttext <addr>      .text base address (hex)\n"
                "  -l  <archive.a>    add static archive\n"
                "  --gc-sections      remove unused sections\n"
                "  --strip-debug      strip debug sections\n"
                "  -v / --verbose     verbose output\n"
                "  --warn-undef       warn on undefined (not error)\n"
                "  --version          print version\n"
                "  --help             this message\n"
                "\n"
                "Examples:\n"
                "  uioxld main.o lib.a -o kernel.elf -arch x86_64\n"
                "  uioxld main.o -o kernel.elf -arch arm64 -fmt elf64\n"
                "  uioxld main.o -o kernel.bin -arch arm32 -fmt flat\n"
                "  uioxld main.o -o kernel.hex -arch arm32 -fmt ihex\n"
                "  uioxld main.o -o kernel.elf -T my.ld -Map kernel.map\n"
            );
            exit(0);

        /* ── input object file ───────────────────────────── */
        } else if (argv[i][0] != '-') {
            if (opts->obj_count < ULD_MAX_OBJECTS)
                opts->obj_paths[opts->obj_count++] = argv[i];
            else {
                fprintf(stderr, "uioxld: too many input files\n");
                return -1;
            }

        } else {
            fprintf(stderr, "uioxld: unknown option: %s\n", argv[i]);
            return -1;
        }
    }

    if (opts->obj_count == 0 && opts->ar_count == 0) {
        fprintf(stderr, "uioxld: no input files\n");
        return -1;
    }
    return 0;
}

/* ── Print options summary ─────────────────────────────────── */
void uld_options_print(const uld_options_t *opts)
{
    static const char *arch_str[] = { "x86_64", "arm64", "arm32" };
    static const char *fmt_str[]  = { "elf64", "elf32", "flat", "ihex", "srec" };

    printf("uioxld %s\n", UIOXLD_VERSION);
    printf("  arch    : %s\n", arch_str[opts->arch]);
    printf("  format  : %s\n", fmt_str[opts->fmt]);
    printf("  output  : %s\n", opts->output_path);
    printf("  entry   : %s\n", opts->entry_sym);
    if (opts->script_path)
        printf("  script  : %s\n", opts->script_path);
    if (opts->map_path)
        printf("  map     : %s\n", opts->map_path);
    if (opts->text_base)
        printf("  Ttext   : 0x%llx\n", (unsigned long long)opts->text_base);
    printf("  inputs  : %u object(s), %u archive(s)\n",
           opts->obj_count, opts->ar_count);
    for (uld_u32_t i = 0; i < opts->obj_count; i++)
        printf("    obj[%u] = %s\n", i, opts->obj_paths[i]);
    for (uld_u32_t i = 0; i < opts->ar_count; i++)
        printf("    ar[%u]  = %s\n", i, opts->ar_paths[i]);
}

/* ── main ──────────────────────────────────────────────────── */
int main(int argc, char **argv)
{
    uld_options_t opts;
    uld_options_default(&opts);

    if (argc < 2) {
        fprintf(stderr,
            "uioxld %s — UIOX Linker\n"
            "Run 'uioxld --help' for usage.\n",
            UIOXLD_VERSION);
        return 1;
    }

    if (uld_options_parse(&opts, argc, argv) < 0)
        return 1;

    if (opts.verbose)
        uld_options_print(&opts);

    uld_ctx_t ctx;
    uld_ctx_init(&ctx, &opts);

    int rc = uld_ctx_run(&ctx);

    uld_ctx_free(&ctx);
    return rc;
}
