# =============================================================================
# 10_BSP/Makefile  — v1.8.0
#
# UIOX Board-Support Package — master Makefile (Linux + macOS).
# Architecture selected directly as a target — no ARCH= variable.
#
# Usage:
#   make arm64 / arm32 / riscv64 / x86_64
#   make arm64   BUILD=dynamic
#   make all  /  make all BUILD=dynamic
#   make clean[-arm64|-arm32|-riscv64|-x86_64]
#   make info[-arm64|-arm32|-riscv64|-x86_64]
#
# v1.8 change vs v1.7:
#   SOC sources are now split into shared (all arches) and arch-specific.
#   Previously the wildcard 03_SoC/src/*.c compiled every backend for every
#   arch, causing "SOC_TIMER0_BASE undeclared" when uiox_soc_arm32.c was
#   compiled with -D__aarch64__ (uiox_soc_map.h has no TIMER0 in arm64 block).
#
# Source directories per arch build:
#   src/                    uiox_bsp_main.c
#   10_Arch/<arch>/src/     arch_init.c  irq.c
#   03_SoC/src/ (shared)    uiox_soc_main/clk/dma/hw/irq/mem/pcie/post/
#                           power/psci/secboot/stdio/tz/kernel_loader
#   03_SoC/src/ (per-arch)  uiox_soc_<arch>.c  uiox_soc_<arch>_init.c
#
# Include directories (all BSP-internal):
#   include/                    uiox_bsp.h
#   03_SoC/include/             uiox_base_types.h  uiox_soc_*.h
#   10_Arch/<arch>/include/     arch_defs.h  hw_types.h  cpu.h  irq.h  mmio.h
#
# @version 1.8.0  @date 2026-07-22
# =============================================================================

BUILD ?= static

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
  #LLVM_BIN  := $(shell brew --prefix llvm 2>/dev/null || echo /opt/homebrew/opt/llvm)/bin
  LLVM_BIN := $(shell \
    if command -v llvm-objcopy >/dev/null 2>&1; then \
        dirname $$(command -v llvm-objcopy); \
    else \
        echo $$(brew --prefix llvm 2>/dev/null || echo /opt/homebrew/opt/llvm)/bin; \
    fi)
  ARM64_CC  := aarch64-elf-gcc
  ARM64_AR  := aarch64-elf-ar
  ARM64_LD  := aarch64-elf-ld
  ARM64_OCP := $(LLVM_BIN)/llvm-objcopy
  ARM64_STR := $(LLVM_BIN)/llvm-strip
  ARM32_CC  := arm-none-eabi-gcc
  ARM32_AR  := arm-none-eabi-ar
  ARM32_LD  := arm-none-eabi-ld
  ARM32_OCP := $(LLVM_BIN)/llvm-objcopy
  ARM32_STR := $(LLVM_BIN)/llvm-strip
  X86_CC    := x86_64-elf-gcc
  X86_AR    := x86_64-elf-ar
  X86_LD    := x86_64-elf-ld
  X86_OCP   := $(LLVM_BIN)/llvm-objcopy
  X86_STR   := $(LLVM_BIN)/llvm-strip
  RV64_CC   := riscv64-elf-gcc
  RV64_AR   := riscv64-elf-ar
  RV64_LD   := riscv64-elf-ld
  RV64_OCP  := $(LLVM_BIN)/llvm-objcopy
  RV64_STR  := $(LLVM_BIN)/llvm-strip
else
  ARM64_CC  := aarch64-linux-gnu-gcc
  ARM64_AR  := aarch64-linux-gnu-ar
  ARM64_LD  := aarch64-linux-gnu-ld
  ARM64_OCP := aarch64-linux-gnu-objcopy
  ARM64_STR := aarch64-linux-gnu-strip
  ARM32_CC  := arm-linux-gnueabihf-gcc
  ARM32_AR  := arm-linux-gnueabihf-ar
  ARM32_LD  := arm-linux-gnueabihf-ld
  ARM32_OCP := arm-linux-gnueabihf-objcopy
  ARM32_STR := arm-linux-gnueabihf-strip
  X86_CC    := x86_64-linux-gnu-gcc
  X86_AR    := x86_64-linux-gnu-ar
  X86_LD    := x86_64-linux-gnu-ld
  X86_OCP   := x86_64-linux-gnu-objcopy
  X86_STR   := x86_64-linux-gnu-strip
  RV64_CC   := riscv64-linux-gnu-gcc
  RV64_AR   := riscv64-linux-gnu-ar
  RV64_LD   := riscv64-linux-gnu-ld
  RV64_OCP  := riscv64-linux-gnu-objcopy
  RV64_STR  := riscv64-linux-gnu-strip
endif

BSP_DIR    := $(CURDIR)
SOC_DIR    := $(BSP_DIR)/03_SoC
INC_DIR    := $(BSP_DIR)/include
LINKER_DIR := $(BSP_DIR)/linker

BSP_SRCS_C := $(BSP_DIR)/src/uiox_bsp_main.c\
              $(BSP_DIR)/src/uiox_bsp_stubs.c

# ── SoC sources — SPLIT into shared and arch-specific ────────────────────────
# Shared: compiled for every arch. These files use only generic SOC APIs
# and do not reference arch-specific SOC_* peripheral addresses.
SOC_SHARED_SRCS_C := \
    $(SOC_DIR)/src/uiox_soc_main.c        \
    $(SOC_DIR)/src/uiox_soc_clk.c         \
    $(SOC_DIR)/src/uiox_soc_dma.c         \
    $(SOC_DIR)/src/uiox_soc_hw.c          \
    $(SOC_DIR)/src/uiox_soc_irq.c         \
    $(SOC_DIR)/src/uiox_soc_mem.c         \
    $(SOC_DIR)/src/uiox_soc_pcie.c        \
    $(SOC_DIR)/src/uiox_soc_post.c        \
    $(SOC_DIR)/src/uiox_soc_power.c       \
    $(SOC_DIR)/src/uiox_soc_psci.c        \
    $(SOC_DIR)/src/uiox_soc_secboot.c     \
    $(SOC_DIR)/src/uiox_soc_stdio.c       \
    $(SOC_DIR)/src/uiox_soc_tz.c          \
    $(SOC_DIR)/src/uiox_kernel_loader.c

# Arch-specific SoC backend: selected in the _build section via _ARCH.
# Only the matching backend is compiled — other arch files are excluded.
# (Selection happens inside _build below.)

ifeq ($(BUILD),dynamic)
  BSP_SRCS_S := $(BSP_DIR)/src/bsp_entry.S
else
  BSP_SRCS_S :=
endif

ifeq ($(BUILD),static)
  LDSCRIPT :=
else
  LDSCRIPT := $(LINKER_DIR)/bsp_dynamic.ld
endif

COMMON_CFLAGS_BASE := \
    -std=c11              \
    -Wall -Wextra -Werror \
    -ffreestanding        \
    -fno-builtin          \
    -fno-stack-protector  \
    -nostdinc             \
    -nostdlib             \
    -Os                   \
    -g                    \
    -I$(INC_DIR)          \
    -I$(SOC_DIR)/include

ifeq ($(BUILD),dynamic)
  COMMON_CFLAGS_BASE += -DUIOX_BSP_DYNAMIC_BUILD -ffunction-sections -fdata-sections
endif

# =============================================================================
# Per-architecture named targets
# =============================================================================

.PHONY: arm64
arm64: _do_arm64
	@echo "[BSP] arm64 $(BUILD) done → build/arm64/"

.PHONY: _do_arm64
_do_arm64:
	@mkdir -p $(BSP_DIR)/build/arm64
	@command -v $(ARM64_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(ARM64_CC) not found"; \
	      echo "  Linux: sudo apt install gcc-aarch64-linux-gnu"; \
	      echo "  macOS: brew install aarch64-elf-gcc"; exit 1; }
	$(MAKE) -f $(BSP_DIR)/Makefile _build         \
	    _ARCH=arm64                                  \
	    CC=$(ARM64_CC)  AS=$(ARM64_CC)              \
	    AR=$(ARM64_AR)  LD=$(ARM64_LD)              \
	    OBJCOPY=$(ARM64_OCP) STRIP=$(ARM64_STR)     \
	    ARCH_CFLAGS="-march=armv8-a -mabi=lp64 -mlittle-endian" \
	    ARCH_ASFLAGS="-march=armv8-a"                \
	    ARCH_CPUDEF="-D__aarch64__"                  \
	    LD_EMUL=aarch64elf  BUILD=$(BUILD)

.PHONY: arm32
arm32: _do_arm32
	@echo "[BSP] arm32 $(BUILD) done → build/arm32/"

.PHONY: _do_arm32
_do_arm32:
	@mkdir -p $(BSP_DIR)/build/arm32
	@command -v $(ARM32_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(ARM32_CC) not found"; \
	      echo "  Linux: sudo apt install gcc-arm-linux-gnueabihf"; \
	      echo "  macOS: brew install arm-none-eabi-gcc"; exit 1; }
	$(MAKE) -f $(BSP_DIR)/Makefile _build                     \
	    _ARCH=arm32                                              \
	    CC=$(ARM32_CC)  AS=$(ARM32_CC)                          \
	    AR=$(ARM32_AR)  LD=$(ARM32_LD)                          \
	    OBJCOPY=$(ARM32_OCP) STRIP=$(ARM32_STR)                 \
	    ARCH_CFLAGS="-march=armv7-a -marm -mfloat-abi=soft -mlittle-endian" \
	    ARCH_ASFLAGS="-march=armv7-a -marm"                     \
	    ARCH_CPUDEF="-D__arm__"  LD_EMUL=armelf  BUILD=$(BUILD)

.PHONY: riscv64
riscv64: _do_riscv64
	@echo "[BSP] riscv64 $(BUILD) done → build/riscv64/"

.PHONY: _do_riscv64
_do_riscv64:
	@mkdir -p $(BSP_DIR)/build/riscv64
	@command -v $(RV64_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(RV64_CC) not found"; \
	      echo "  Linux: sudo apt install gcc-riscv64-linux-gnu"; \
	      echo "  macOS: brew install riscv64-elf-gcc"; exit 1; }
	$(MAKE) -f $(BSP_DIR)/Makefile _build                         \
	    _ARCH=riscv64                                                \
	    CC=$(RV64_CC)  AS=$(RV64_CC)                               \
	    AR=$(RV64_AR)  LD=$(RV64_LD)                               \
	    OBJCOPY=$(RV64_OCP) STRIP=$(RV64_STR)                      \
	    ARCH_CFLAGS="-march=rv64imafdc -mabi=lp64d -mcmodel=medany" \
	    ARCH_ASFLAGS="-march=rv64imafdc -mabi=lp64d"                \
	    ARCH_CPUDEF="-D__riscv -D__riscv_xlen=64"                   \
	    LD_EMUL=elf64lriscv  BUILD=$(BUILD)

.PHONY: x86_64
x86_64: _do_x86_64
	@echo "[BSP] x86_64 $(BUILD) done → build/x86_64/"

.PHONY: _do_x86_64
_do_x86_64:
	@mkdir -p $(BSP_DIR)/build/x86_64
	@command -v $(X86_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(X86_CC) not found"; \
	      echo "  Linux: sudo apt install gcc-x86-64-linux-gnu"; \
	      echo "  macOS: brew install x86_64-elf-gcc"; exit 1; }
	$(MAKE) -f $(BSP_DIR)/Makefile _build                                    \
	    _ARCH=x86_64                                                            \
	    CC=$(X86_CC)  AS=$(X86_CC)  AR=$(X86_AR)  LD=$(X86_LD)              \
	    OBJCOPY=$(X86_OCP) STRIP=$(X86_STR)                                   \
	    ARCH_CFLAGS="-m64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pic -mcmodel=kernel" \
	    ARCH_ASFLAGS="-m64"  ARCH_CPUDEF="-D__x86_64__"                        \
	    LD_EMUL=elf_x86_64  BUILD=$(BUILD)

# =============================================================================
# Internal _build target
# =============================================================================
_ARCH_DIR  = $(BSP_DIR)/10_Arch/$(_ARCH)
_BUILD_DIR = $(BSP_DIR)/build/$(_ARCH)

# ── Arch-specific SoC backend selection ──────────────────────────────────────
# Only the matching backend is compiled. Other arch backends are excluded so
# uiox_soc_map.h always selects the right peripheral address block.
ifeq ($(_ARCH),arm64)
  _SOC_ARCH_SRCS := $(SOC_DIR)/src/uiox_soc_arm64.c $(SOC_DIR)/src/uiox_soc_arm64_init.c
endif
ifeq ($(_ARCH),arm32)
  _SOC_ARCH_SRCS := $(SOC_DIR)/src/uiox_soc_arm32.c $(SOC_DIR)/src/uiox_soc_arm32_init.c
endif
ifeq ($(_ARCH),riscv64)
  _SOC_ARCH_SRCS := $(SOC_DIR)/src/uiox_soc_riscv64.c $(SOC_DIR)/src/uiox_soc_riscv64_init.c
endif
ifeq ($(_ARCH),x86_64)
  _SOC_ARCH_SRCS := $(SOC_DIR)/src/uiox_soc_x86.c $(SOC_DIR)/src/uiox_soc_x86_init.c
endif

_ALL_SRCS_C = $(BSP_SRCS_C)                     \
              $(wildcard $(_ARCH_DIR)/src/*.c)   \
              $(SOC_SHARED_SRCS_C)                \
              $(_SOC_ARCH_SRCS)

_ALL_SRCS_S = $(BSP_SRCS_S) $(wildcard $(_ARCH_DIR)/src/*.S)

_OBJS_C = $(patsubst %.c, $(_BUILD_DIR)/%.o, $(notdir $(_ALL_SRCS_C)))
_OBJS_S = $(patsubst %.S, $(_BUILD_DIR)/%.o, $(notdir $(_ALL_SRCS_S)))
_OBJS   = $(_OBJS_C) $(_OBJS_S)

# -I<arch>/include resolves: arch_defs.h hw_types.h cpu.h irq.h mmio.h
_CFLAGS_ALL = $(COMMON_CFLAGS_BASE) \
              $(ARCH_CFLAGS)         \
              $(ARCH_CPUDEF)         \
              -I$(_ARCH_DIR)/include

ifeq ($(BUILD),static)
  _TARGET := $(_BUILD_DIR)/libbsp.a
else
  _TARGET_ELF := $(_BUILD_DIR)/uiox_bsp.elf
  _TARGET_BIN := $(_BUILD_DIR)/uiox_bsp.bin
  _TARGET     := $(_TARGET_ELF) $(_TARGET_BIN)
endif

vpath %.c $(BSP_DIR)/src $(_ARCH_DIR)/src $(SOC_DIR)/src
vpath %.S $(BSP_DIR)/src $(_ARCH_DIR)/src

.PHONY: _build
_build: $(_TARGET)
	@echo "[BSP] _build done: $(_ARCH) $(BUILD)"

$(_BUILD_DIR)/%.o: %.c
	@echo "[CC]  $<"
	$(CC) $(_CFLAGS_ALL) -c $< -o $@

$(_BUILD_DIR)/%.o: %.S
	@echo "[AS]  $<"
	$(AS) $(_CFLAGS_ALL) $(ARCH_ASFLAGS) $(ARCH_CPUDEF) -I$(INC_DIR) -c $< -o $@

$(_BUILD_DIR)/libbsp.a: $(_OBJS)
	@echo "[AR]  $@"
	$(AR) rcs $@ $^

$(_TARGET_ELF): $(_OBJS) $(LDSCRIPT)
	@echo "[LD]  $@"
	$(LD) -T $(LDSCRIPT) -Map=$(_BUILD_DIR)/uiox_bsp.map \
	      --gc-sections $(_OBJS) -o $@

$(_TARGET_BIN): $(_TARGET_ELF)
	@echo "[BIN] $@"
	$(OBJCOPY) -O binary --strip-all $< $@

# =============================================================================
# Convenience targets
# =============================================================================

.PHONY: all
all: arm64 arm32 riscv64 x86_64
	@echo "" && echo "[BSP] all $(BUILD) builds complete"

.PHONY: clean-arm64 clean-arm32 clean-riscv64 clean-x86_64 clean
clean-arm64:   ; rm -rf $(BSP_DIR)/build/arm64
clean-arm32:   ; rm -rf $(BSP_DIR)/build/arm32
clean-riscv64: ; rm -rf $(BSP_DIR)/build/riscv64
clean-x86_64:  ; rm -rf $(BSP_DIR)/build/x86_64
clean: clean-arm64 clean-arm32 clean-riscv64 clean-x86_64
	@echo "[CLN] all build outputs removed"

.PHONY: info-arm64 info-arm32 info-riscv64 info-x86_64
info-arm64:
	@echo "HOST=$(UNAME_S) ARCH=arm64   CC=$(ARM64_CC) BUILD=$(BUILD)"
info-arm32:
	@echo "HOST=$(UNAME_S) ARCH=arm32   CC=$(ARM32_CC) BUILD=$(BUILD)"
info-riscv64:
	@echo "HOST=$(UNAME_S) ARCH=riscv64 CC=$(RV64_CC)  BUILD=$(BUILD)"
info-x86_64:
	@echo "HOST=$(UNAME_S) ARCH=x86_64  CC=$(X86_CC)   BUILD=$(BUILD)"
