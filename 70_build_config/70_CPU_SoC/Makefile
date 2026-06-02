# =============================================================
# 70_CPU_SoC/Makefile
# Builds the UIOX CPU/SoC stack for ARM64, ARM32, or x86_64.
#
# Usage:
#   make ARCH=arm64    (default)
#   make ARCH=x86_64
#   make ARCH=riscv64
# =============================================================

ARCH   ?= arm64
CC_ARM64   = aarch64-linux-gnu-gcc
CC_X86     = gcc
CC_RISCV   = riscv64-linux-gnu-gcc

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
    src/cpu_features.c    \
    src/cpu_cache.c       \
    src/cpu_mmu.c         \
    src/cpu_irq.c         \
    src/cpu_timer.c       \
    src/cpu_power.c       \
    src/cpu_smp.c         \
    src/cpu_context.c     \
    src/cpu_debug.c       \
    src/cpu_io.c          \
    src/cpu_soc.c         \
    src/arch/cortex_a76.c \
    src/arch/x86_64_cpu.c \
    src/arch/riscv64.c    \
    src/drivers/cpu_drv_gic.c   \
    src/drivers/cpu_drv_apic.c  \
    src/drivers/cpu_drv_plic.c  \
    src/drivers/cpu_drv_timer.c \
    src/drivers/cpu_drv_uart.c

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
