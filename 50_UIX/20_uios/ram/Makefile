# =============================================================
# 70_CPU_SoC/Makefile
# Builds the UIOX CPU/SoC stack for ARM64, ARM32, or x86_64.
#
# Usage:
#   make ARCH=arm64    (default)
#   make ARCH=x86_64
#   make ARCH=riscv64
# =============================================================
#"aarch64-linux-gnu-gcc". Did you mean aarch64-elf-gcc?
#brew install aarch64-elf-gcc 
#"riscv64-linux-gnu-gcc". Did you mean riscv64-elf-gcc?
ARCH   ?= arm64
#CC_ARM64   = aarch64-linux-gnu-gcc
#CC_ARM64   = arm-none-eabi-gcc
CC_ARM64   = aarch64-elf-gcc
CC_X86     = gcc
#CC_RISCV   = riscv64-linux-gnu-gcc
CC_RISCV   = riscv64-elf-gcc

ifeq ($(ARCH),arm64)
  CC      = $(CC_ARM64)
  ARCH_DEF= -DUIOX_ARCH_ARM64
  MARCH   = -march=armv8-a
else ifeq ($(ARCH),x86_64)
  CC      = $(CC_X86)
  ARCH_DEF= -DUIOX_ARCH_X86_64
  MARCH   = -m64
else ifeq ($(ARCH),riscv64)
  CC      = $(CC_RISCV)
  ARCH_DEF= -DUIOX_ARCH_RISCV64
  MARCH   = -march=rv64gc -mabi=lp64d
endif

AR      = ar
CFLAGS  = -Wall -Wextra -std=c99 -g3 -O2 \
           -ffreestanding -fno-builtin \
           -fno-stack-protector -nostdlib \
           $(MARCH) $(ARCH_DEF) \
           -I./include

LIB_SRCS = \
    $(SRCDIR)/uiox_ram_hw.c     \
    $(SRCDIR)/uiox_ram_buf.c    \
    $(SRCDIR)/uiox_ram_if.c     \
    $(SRCDIR)/uiox_ram_ecc.c    \
    $(SRCDIR)/uiox_ram_mgr.c    \
    $(SRCDIR)/uiox_ram_subsys.c \
    $(SRCDIR)/uiox_ram_device.c

LIB_OBJS = $(patsubst src/%.c, build/$(ARCH)/%.o, $(LIB_SRCS))
LIB      = build/$(ARCH)/libcpu_soc.a

.PHONY: all clean

all: $(LIB)
	@echo ""
	@echo "  libcpu_soc.a built for $(ARCH): $(LIB)"

$(LIB): $(LIB_OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

build/$(ARCH)/%.o: src/%.c
	@mkdir -p $(dir $@)
	@echo "  CC [$( ARCH)] $<"
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf build/
