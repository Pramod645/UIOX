UIOX/
└── 60_Compiler/
    ├── include/
    │   ├── uiox_compiler.h       ← master include
    │   ├── uiox_lexer.h          ← tokeniser
    │   ├── uiox_token.h          ← token types
    │   ├── uiox_parser.h         ← parser / AST builder
    │   ├── uiox_ast.h            ← AST node definitions
    │   ├── uiox_symtab.h         ← symbol table
    │   ├── uiox_types.h          ← type system
    │   ├── uiox_codegen.h        ← code generator
    │   ├── uiox_ir.h             ← intermediate representation
    │   ├── uiox_regalloc.h       ← register allocator
    │   ├── uiox_emit.h           ← instruction emitter
    │   ├── uiox_linker.h         ← linker
    │   ├── uiox_object.h         ← object file format
    │   ├── uiox_section.h        ← section/segment management
    │   └── uiox_error.h          ← diagnostics
    ├── src/
    │   ├── uiox_lexer.c
    │   ├── uiox_parser.c
    │   ├── uiox_ast.c
    │   ├── uiox_symtab.c
    │   ├── uiox_types.c
    │   ├── uiox_codegen.c
    │   ├── uiox_ir.c
    │   ├── uiox_regalloc.c
    │   ├── uiox_emit.c
    │   ├── uiox_linker.c
    │   ├── uiox_object.c
    │   ├── uiox_section.c
    │   ├── uiox_error.c
    │   └── uiox_main.c           ← compiler/linker driver
    └── Makefile
/////////////
#	File	Purpose
1	include/uiox_error.h	Diagnostic system
2	include/uiox_token.h	Token type enum (70+ token kinds)
3	include/uiox_lexer.h	Tokeniser interface
4	include/uiox_ast.h	AST node definitions (40+ node kinds)
5	include/uiox_symtab.h	Symbol table with scoped hash chains
6	include/uiox_parser.h	Recursive-descent parser interface
7	include/uiox_ir.h	Three-address code IR
8	include/uiox_regalloc.h	Linear-scan register allocator
9	include/uiox_section.h	Section/segment management
10	include/uiox_object.h	.uobj object file format
11	include/uiox_emit.h	Machine code emitter (x86_64/ARM64/ARM32)
12	include/uiox_codegen.h	AST → IR lowering
13	include/uiox_linker.h	6-pass linker (ELF64/ELF32/flat/IHEX)
14	include/uiox_compiler.h	Master include + pipeline driver API
15	src/uiox_error.c	Diagnostic emit + colour printing
16	src/uiox_lexer.c	Full C tokeniser with keyword table
17	src/uiox_ast.c	AST alloc, free, print
18	src/uiox_symtab.c	Scoped symbol table push/pop/lookup
19	src/uiox_parser.c	Full recursive-descent C parser
20	src/uiox_ir.c	IR module/func/block/instr builder
21	src/uiox_codegen.c	AST → IR lowering for all node kinds
22	src/uiox_regalloc.c	Linear-scan allocator + phys reg tables
23	src/uiox_section.c	Section grow/write/patch
24	src/uiox_object.c	Object file read/write/print
25	src/uiox_emit.c	x86_64 + ARM64 + ARM32 code emitters
26	src/uiox_linker.c	6-pass linker + ELF64/ELF32/flat/IHEX output
27	src/uiox_main.c	Compiler driver + main() + CLI options
28	Makefile	Build, test, clean
/////////////
# Build the compiler
cd 60_Compiler
make

# Compile a C file for x86_64
./build/uioxcc hello.c -o hello.elf -arch x86_64 -fmt elf64

# Compile for ARM64
./build/uioxcc hello.c -o hello_arm64.elf -arch arm64 -fmt elf64

# Compile for ARM32
./build/uioxcc hello.c -o hello_arm32.elf -arch arm32 -fmt elf32

# Stop at IR dump (no code gen)
./build/uioxcc hello.c -o hello.elf -arch x86_64 -S

# Compile only — produce .uobj object file
./build/uioxcc hello.c -o hello.elf -arch x86_64 -c

# Flat binary (for bare-metal loading)
./build/uioxcc hello.c -o hello.bin -arch arm64 -fmt flat

# Intel HEX output (for embedded flash tools)
./build/uioxcc hello.c -o hello.hex -arch arm32 -fmt ihex

# Verbose pipeline trace
./build/uioxcc hello.c -o hello.elf -arch x86_64 -v

# Run self-test
make test
//////////
The compiler pipeline is: source read → lex → parse → semantic → IR gen → IR opt → register alloc → emit → link, fully matching the UIOX subsystem layering you already have in the repository.