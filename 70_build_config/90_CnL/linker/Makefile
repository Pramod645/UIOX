# =============================================================
# 60_Linker/Makefile
# Builds the UIOX linker (uioxld)
# =============================================================

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -g3 -O2 -I./include
AR      = ar
ARFLAGS = rcs

# -- Source files ---------------------------------------------
LIB_SRCS = \
    src/uiox_ld_diag.c    \
    src/uiox_ld_section.c \
    src/uiox_ld_symbol.c  \
    src/uiox_ld_reloc.c   \
    src/uiox_ld_object.c  \
    src/uiox_ld_archive.c \
    src/uiox_ld_script.c  \
    src/uiox_ld_map.c     \
    src/uiox_ld_elf.c     \
    src/uiox_linker.c

MAIN_SRC  = src/uiox_ld_main.c

LIB_OBJS  = $(patsubst src/%.c, build/%.o, $(LIB_SRCS))
MAIN_OBJ  = build/uiox_ld_main.o

LIB       = build/libuioxld.a
BIN       = build/uioxld

.PHONY: all clean test install

all: $(BIN)

# -- Static library -------------------------------------------
$(LIB): $(LIB_OBJS)
	@mkdir -p build
	$(AR) $(ARFLAGS) $@ $^
	@echo "  AR  $@"

# -- Linker binary --------------------------------------------
$(BIN): $(MAIN_OBJ) $(LIB)
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $^
	@echo ""
	@echo "  uioxld built: $@"

# -- Pattern rule ---------------------------------------------
build/%.o: src/%.c
	@mkdir -p build
	@echo "  CC  $<"
	$(CC) $(CFLAGS) -c -o $@ $<

# -- Self-test ------------------------------------------------
test: $(BIN)
	@echo "=== uioxld self-test ==="
	./$(BIN) --version
	@echo ""
	@echo "--- link test (x86_64 ELF64) ---"
	@echo "Skipping object link test (no .o available in isolation)"
	@echo "Run: uioxld main.o -o out.elf -arch x86_64 -fmt elf64 -v"
	@echo "=== DONE ==="

# -- Install to /usr/local/bin --------------------------------
install: $(BIN)
	cp $(BIN) /usr/local/bin/uioxld
	@echo "Installed: /usr/local/bin/uioxld"

clean:
	rm -rf build/
	@echo "Clean done."
