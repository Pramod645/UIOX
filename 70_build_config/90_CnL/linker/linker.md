60_Linker/
├── include/
│   ├── uiox_ld_types.h       ← base types
│   ├── uiox_ld_section.h     ← section management
│   ├── uiox_ld_symbol.h      ← symbol table
│   ├── uiox_ld_reloc.h       ← relocation engine
│   ├── uiox_ld_object.h      ← object file reader
│   ├── uiox_ld_archive.h     ← static library (.a) reader
│   ├── uiox_ld_script.h      ← linker script parser
│   ├── uiox_ld_map.h         ← map file generator
│   ├── uiox_ld_elf.h         ← ELF64/ELF32 writer
│   ├── uiox_ld_diag.h        ← diagnostics
│   └── uiox_linker.h         ← master include + driver API
├── src/
│   ├── uiox_ld_section.c
│   ├── uiox_ld_symbol.c
│   ├── uiox_ld_reloc.c
│   ├── uiox_ld_object.c
│   ├── uiox_ld_archive.c
│   ├── uiox_ld_script.c
│   ├── uiox_ld_map.c
│   ├── uiox_ld_elf.c
│   ├── uiox_ld_diag.c
│   ├── uiox_linker.c
│   └── uiox_ld_main.c        ← uioxld driver entry point
└── Makefile
===========================================================
#	File	Purpose
1	include/uiox_ld_types.h	Base integer types, arch/format enums, limits
2	include/uiox_ld_diag.h	Coloured diagnostic emit (note/warn/error/fatal)
3	include/uiox_ld_section.h	Input + output section structs, merge API
4	include/uiox_ld_symbol.h	Global symbol table, bind/type/vis enums
5	include/uiox_ld_reloc.h	21 relocation types, reloc table, apply engine
6	include/uiox_ld_object.h	ELF + UIOX native object file reader
7	include/uiox_ld_archive.h	BSD/GNU .a archive reader
8	include/uiox_ld_script.h	GNU ld-compatible linker script parser
9	include/uiox_ld_map.h	Map file generator
10	include/uiox_ld_elf.h	ELF64/ELF32/flat/IHEX/SREC output structures
11	include/uiox_linker.h	Master include, options struct, 10-pass pipeline API
12	src/uiox_ld_diag.c	Diagnostic list, colour print
13	src/uiox_ld_section.c	Section grow/append/patch/print
14	src/uiox_ld_symbol.c	Hash table, define, undef check, abs symbols
15	src/uiox_ld_reloc.c	Full reloc application for x86_64/ARM32/ARM64
16	src/uiox_ld_object.c	ELF64 section/symbol/reloc reader
17	src/uiox_ld_archive.c	ar format scan, member extract
18	src/uiox_ld_script.c	ENTRY/MEMORY/SECTIONS parser + 3 default scripts
19	src/uiox_ld_map.c	Full map file with per-object contributions
20	src/uiox_ld_elf.c	ELF64 + ELF32 + flat + IHEX + SREC writers
21	src/uiox_linker.c	10-pass pipeline: load→merge→collect→layout→resolve→reloc→gc→emit→map
22	src/uiox_ld_main.c	CLI driver, option parser, main()
23	Makefile	Build, test, install targets
=============================================================================================
Quick start:
cd 60_Linker
make
./build/uioxld --version
./build/uioxld main.o lib.a -o kernel.elf -arch x86_64 -fmt elf64 -v
./build/uioxld main.o -o kernel.elf -arch arm64 -T ../linker/uiox_arm64.ld -Map kernel.map
./build/uioxld main.o -o kernel.bin -arch arm32 -fmt flat
./build/uioxld main.o -o kernel.hex -arch arm32 -fmt ihex
=============================================================================================
# Build
cd 60_Linker
make

# Check version
./build/uioxld --version

# Link x86_64 ELF64
./build/uioxld main.o -o kernel.elf \
    -arch x86_64 -fmt elf64 \
    -e _start -v

# Link ARM64 with linker script + map file
./build/uioxld main.o -o kernel.elf \
    -arch arm64 -fmt elf64 \
    -T ../linker/uiox_arm64.ld \
    -Map kernel.map -v

# Link ARM32 flat binary
./build/uioxld main.o -o kernel.bin \
    -arch arm32 -fmt flat

# Link ARM32 Intel HEX (for flash tools)
./build/uioxld main.o -o kernel.hex \
    -arch arm32 -fmt ihex

# Link with a static archive
./build/uioxld main.o libkernel.a \
    -o kernel.elf -arch x86_64

# GC unused sections + strip debug
./build/uioxld main.o -o kernel.elf \
    -arch x86_64 --gc-sections --strip-debug
