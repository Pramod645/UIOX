# =============================================================
# 60_Compiler/Makefile
# Builds the UIOX compiler (uioxcc) and linker (uioxld).
# =============================================================

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -g3 -O2 \
           -I./include
AR      = ar
ARFLAGS = rcs

# -- Source files ---------------------------------------------
SRCS = src/uiox_error.c      \
       src/uiox_lexer.c      \
       src/uiox_ast.c        \
       src/uiox_symtab.c     \
       src/uiox_parser.c     \
       src/uiox_ir.c         \
       src/uiox_codegen.c    \
       src/uiox_regalloc.c   \
       src/uiox_section.c    \
       src/uiox_object.c     \
       src/uiox_emit.c       \
       src/uiox_linker.c

MAIN_SRC = src/uiox_main.c

OBJS     = $(patsubst src/%.c, build/%.o, $(SRCS))
MAIN_OBJ = build/uiox_main.o

LIB      = build/libuioxcc.a
BIN      = build/uioxcc

.PHONY: all clean test

all: $(BIN)

# -- Static library (all except main) -------------------------
$(LIB): $(OBJS)
	@mkdir -p build
	$(AR) $(ARFLAGS) $@ $^
	@echo "  AR  $@"

# -- Compiler binary ------------------------------------------
$(BIN): $(MAIN_OBJ) $(LIB)
	$(CC) $(CFLAGS) -o $@ $^
	@echo "  LD  $@"
	@echo ""
	@echo "  uioxcc built successfully."
	@echo "  Usage: ./build/uioxcc input.c -o output [-arch arm64|arm32|x86_64]"

# -- Pattern rule: .c -> .o -----------------------------------
build/%.o: src/%.c
	@mkdir -p build
	@echo "  CC  $<"
	$(CC) $(CFLAGS) -c -o $@ $<

# -- Test: compile a sample C file ----------------------------
test: $(BIN)
	@echo "=== UIOX compiler self-test ==="
	echo 'int add(int a, int b) { return a + b; } \
	      int main(void) { return add(1, 2); }' \
	     > /tmp/uiox_test.c
	./$(BIN) /tmp/uiox_test.c -o /tmp/uiox_test.elf \
	         -arch x86_64 -fmt elf64 -v
	@echo "=== Test passed ==="

clean:
	rm -rf build/
