#10_Arch/x86_64/
#├── include/
#│   ├── arch.h
#│   ├── cpu.h
#│   ├── mmu.h
#│   ├── idt.h
#│   ├── gdt.h
#│   └── io.h
#├── src/
#│   ├── boot.S
#│   ├── boot.c
#│   ├── cpu.c
#│   ├── mmu.c
#│   ├── gdt.c
#│   ├── idt.c
#│   └── io.c
#├── Makefile
#└── x86_64.md

# ============================================================
#  10_Arch/x86_64/Makefile
#
#  Builds:
#    obj/  — object files
#    lib/  — libarch_x86_64.a  (static)
#
#  Mirrors: 10_Arch/arm32/Makefile
# ============================================================

ARCH      := x86_64
ARCHDIR   := $(shell pwd)

# ── Toolchain ────────────────────────────────────────────── #
CC        := gcc
AS        := gcc          # use gcc to assemble (handles .S → .o)
AR        := ar
ARFLAGS   := rcs
OBJCOPY   := objcopy

# ── Flags ────────────────────────────────────────────────── #
CFLAGS    := \
    -std=c11              \
    -Wall                 \
    -Wextra               \
    -Wpedantic            \
    -Wmissing-prototypes  \
    -Wstrict-prototypes   \
    -ffreestanding        \
    -fno-stack-protector  \
    -fno-builtin          \
    -mno-red-zone         \
    -O2                   \
    -g                    \
    -Iinclude
#    -mcmodel=kernel       #\

ASFLAGS   := \
    -Wall                 \
    -ffreestanding        \
    -O2                   \
    -Iinclude

# ── Directories ──────────────────────────────────────────── #
SRCDIR    := src
INCDIR    := include
OBJDIR    := obj
LIBDIR    := lib

# ── Sources ──────────────────────────────────────────────── #
C_SRCS    := $(wildcard $(SRCDIR)/*.c)
ASM_SRCS  := $(wildcard $(SRCDIR)/*.S)

C_OBJS    := $(patsubst $(SRCDIR)/%.c,  $(OBJDIR)/%.o, $(C_SRCS))
ASM_OBJS  := $(patsubst $(SRCDIR)/%.S,  $(OBJDIR)/%.o, $(ASM_SRCS))

OBJS      := $(C_OBJS) $(ASM_OBJS)
STATIC    := $(LIBDIR)/libarch_$(ARCH).a

# ── Targets ──────────────────────────────────────────────── #
.PHONY: all clean dirs

all: dirs $(STATIC)

$(STATIC): $(OBJS) | $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "  [AR]  $@"

# ── Compile C ────────────────────────────────────────────── #
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "  [CC]  $<"

# ── Assemble ─────────────────────────────────────────────── #
$(OBJDIR)/%.o: $(SRCDIR)/%.S | $(OBJDIR)
	$(AS) $(ASFLAGS) -c $< -o $@
	@echo "  [AS]  $<"

# ── Directories ──────────────────────────────────────────── #
dirs: $(OBJDIR) $(LIBDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(LIBDIR):
	mkdir -p $(LIBDIR)

# ── Clean ────────────────────────────────────────────────── #
clean:
	rm -rf $(OBJDIR) $(LIBDIR)
	@echo "  [CLEAN] $(ARCH) arch"
