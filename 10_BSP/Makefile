# =============================================================================
# 10_BSP/Makefile
#
# UIOX Board-Support Package — master Makefile (Linux + macOS).
#
# Architecture is selected directly as a make target — no ARCH= variable.
#
# Usage:
#   make arm64                    # static build, ARM64
#   make arm32                    # static build, ARM32
#   make riscv64                  # static build, RISC-V 64
#   make x86_64                   # static build, x86-64
#
#   make arm64   BUILD=dynamic    # dynamic (secondary-BL) build, ARM64
#   make arm32   BUILD=dynamic    # dynamic build, ARM32
#   make riscv64 BUILD=dynamic    # dynamic build, RISC-V 64
#   make x86_64  BUILD=dynamic    # dynamic build, x86-64
#
#   make all                      # static build for all four architectures
#   make all     BUILD=dynamic    # dynamic build for all four architectures
#
#   make clean-arm64              # clean ARM64 build outputs
#   make clean-arm32              # clean ARM32 build outputs
#   make clean-riscv64            # clean RISC-V 64 build outputs
#   make clean-x86_64             # clean x86-64 build outputs
#   make clean                    # clean all architectures
#
#   make info-arm64               # show toolchain and paths for ARM64
#   make info-arm32               # show toolchain and paths for ARM32
#   make info-riscv64             # show toolchain and paths for RISC-V 64
#   make info-x86_64              # show toolchain and paths for x86-64
#
# Outputs (static):
#   build/<arch>/libbsp.a         static library — linked into kernel
#
# Outputs (dynamic):
#   build/<arch>/uiox_bsp.elf     secondary-bootloader ELF
#   build/<arch>/uiox_bsp.bin     raw binary (stripped)
#
# macOS prerequisites (install once):
#   brew install aarch64-elf-gcc arm-none-eabi-gcc \
#                riscv64-elf-gcc  x86_64-elf-gcc   \
#                llvm
#
# Linux prerequisites (install once):
#   sudo apt install gcc-aarch64-linux-gnu  gcc-arm-linux-gnueabihf \
#                    gcc-riscv64-linux-gnu   gcc-x86-64-linux-gnu    \
#                    binutils-multiarch
#
# @version 1.4.0
# @date    2026-07-21
# =============================================================================

BUILD ?= static     # static | dynamic

# ── Host OS detection ─────────────────────────────────────────────────────────
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
  # macOS — Homebrew bare-metal cross-GCC
  LLVM_BIN  := $(shell brew --prefix llvm 2>/dev/null || echo /opt/homebrew/opt/llvm)/bin

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
  # Linux — standard GNU cross-compiler prefixes
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

# ── Fixed directories (no ARCH variable) ─────────────────────────────────────
BSP_DIR    := $(CURDIR)
SOC_DIR    := $(BSP_DIR)/03_SoC
INC_DIR    := $(BSP_DIR)/include
LINKER_DIR := $(BSP_DIR)/linker

# ── Common source files (arch-specific sources added per target below) ────────
BSP_SRCS_C := $(BSP_DIR)/src/uiox_bsp_main.c

ifeq ($(BUILD),dynamic)
  BSP_SRCS_S := $(BSP_DIR)/src/bsp_entry.S
else
  BSP_SRCS_S :=
endif

SOC_SRCS_C := $(wildcard $(SOC_DIR)/src/*.c)

# ── Linker script (dynamic build only) ───────────────────────────────────────
ifeq ($(BUILD),static)
  LDSCRIPT :=
else
  LDSCRIPT := $(LINKER_DIR)/bsp_dynamic.ld
endif

# ── Common compile flags (arch-specific flags appended per target) ────────────
COMMON_CFLAGS_BASE := \
    -std=c11            \
    -Wall -Wextra -Werror \
    -ffreestanding      \
    -fno-builtin        \
    -fno-stack-protector \
    -nostdinc           \
    -nostdlib           \
    -Os                 \
    -g                  \
    -I$(INC_DIR)        \
    -I$(SOC_DIR)/include

ifeq ($(BUILD),dynamic)
  COMMON_CFLAGS_BASE += -DUIOX_BSP_DYNAMIC_BUILD -ffunction-sections -fdata-sections
endif

# =============================================================================
# Internal build macro
# Usage: $(call do-build, ARCH_NAME, CC, AR, LD, OBJCOPY, STRIP,
#                          ARCH_CFLAGS, ARCH_ASFLAGS, CPUDEF, LD_EMUL)
# =============================================================================
define do-build
  _ARCH     := $(1)
  _CC       := $(2)
  _AR       := $(3)
  _LD       := $(4)
  _OBJCOPY  := $(5)
  _STRIP    := $(6)
  _CFLAGS   := $(7)
  _ASFLAGS  := $(8)
  _CPUDEF   := $(9)
  _LDEMUL   := $(10)

  _ARCH_DIR  := $(BSP_DIR)/10_Arch/$(_ARCH)
  _BUILD_DIR := $(BSP_DIR)/build/$(_ARCH)

  _ALL_SRCS_C := $(BSP_SRCS_C) $(wildcard $(_ARCH_DIR)/src/*.c) $(SOC_SRCS_C)
  _ALL_SRCS_S := $(BSP_SRCS_S) $(wildcard $(_ARCH_DIR)/src/*.S)

  _OBJS_C := $(patsubst %.c, $(_BUILD_DIR)/%.o, $(notdir $(_ALL_SRCS_C)))
  _OBJS_S := $(patsubst %.S, $(_BUILD_DIR)/%.o, $(notdir $(_ALL_SRCS_S)))
  _OBJS   := $(_OBJS_C) $(_OBJS_S)

  _CFLAGS_ALL := $(COMMON_CFLAGS_BASE) $(_CFLAGS) $(_CPUDEF) -I$(_ARCH_DIR)/include

  ifeq ($(BUILD),static)
    _TARGET := $(_BUILD_DIR)/libbsp.a
  else
    _TARGET_ELF := $(_BUILD_DIR)/uiox_bsp.elf
    _TARGET_BIN := $(_BUILD_DIR)/uiox_bsp.bin
    _TARGET     := $(_TARGET_ELF) $(_TARGET_BIN)
  endif
endef

# =============================================================================
# Per-architecture named targets
# =============================================================================

# ── arm64 ─────────────────────────────────────────────────────────────────────
.PHONY: arm64
arm64: _do_arm64
	@echo "[BSP] arm64 $(BUILD) build complete → build/arm64/"

.PHONY: _do_arm64
_do_arm64:
	@mkdir -p $(BSP_DIR)/build/arm64
	@command -v $(ARM64_CC) >/dev/null 2>&1 || 	    { echo "ERROR: $(ARM64_CC) not found"; 	      echo "  Linux: sudo apt install gcc-aarch64-linux-gnu"; 	      echo "  macOS: brew install aarch64-elf-gcc"; exit 1; }
	$(MAKE) -f $(BSP_DIR)/Makefile _build       	    _ARCH=arm64                                	    CC=$(ARM64_CC) AS=$(ARM64_CC)             	    AR=$(ARM64_AR) LD=$(ARM64_LD)             	    OBJCOPY=$(ARM64_OCP) STRIP=$(ARM64_STR)   	    ARCH_CFLAGS="-march=armv8-a -mabi=lp64 -mlittle-endian" 	    ARCH_ASFLAGS="-march=armv8-a"              	    ARCH_CPUDEF="-D__aarch64__"                	    LD_EMUL=aarch64elf                         	    BUILD=$(BUILD)

# ── arm32 ─────────────────────────────────────────────────────────────────────
.PHONY: arm32
arm32: _do_arm32
	@echo "[BSP] arm32 $(BUILD) build complete → build/arm32/"

.PHONY: _do_arm32
_do_arm32:
	@mkdir -p $(BSP_DIR)/build/arm32
	@command -v $(ARM32_CC) >/dev/null 2>&1 || 	    { echo "ERROR: $(ARM32_CC) not found"; 	      echo "  Linux: sudo apt install gcc-arm-linux-gnueabihf"; 	      echo "  macOS: brew install arm-none-eabi-gcc"; exit 1; }
	$(MAKE) -f $(BSP_DIR)/Makefile _build               	    _ARCH=arm32                                        	    CC=$(ARM32_CC) AS=$(ARM32_CC)                     	    AR=$(ARM32_AR) LD=$(ARM32_LD)                     	    OBJCOPY=$(ARM32_OCP) STRIP=$(ARM32_STR)           	    ARCH_CFLAGS="-march=armv7-a -marm -mfloat-abi=soft -mlittle-endian" 	    ARCH_ASFLAGS="-march=armv7-a -marm"                	    ARCH_CPUDEF="-D__arm__"                            	    LD_EMUL=armelf                                     	    BUILD=$(BUILD)

# ── riscv64 ───────────────────────────────────────────────────────────────────
.PHONY: riscv64
riscv64: _do_riscv64
	@echo "[BSP] riscv64 $(BUILD) build complete → build/riscv64/"

.PHONY: _do_riscv64
_do_riscv64:
	@mkdir -p $(BSP_DIR)/build/riscv64
	@command -v $(RV64_CC) >/dev/null 2>&1 || 	    { echo "ERROR: $(RV64_CC) not found"; 	      echo "  Linux: sudo apt install gcc-riscv64-linux-gnu"; 	      echo "  macOS: brew install riscv64-elf-gcc"; exit 1; }
	$(MAKE) -f $(BSP_DIR)/Makefile _build                    	    _ARCH=riscv64                                           	    CC=$(RV64_CC) AS=$(RV64_CC)                           	    AR=$(RV64_AR) LD=$(RV64_LD)                           	    OBJCOPY=$(RV64_OCP) STRIP=$(RV64_STR)                 	    ARCH_CFLAGS="-march=rv64imafdc -mabi=lp64d -mcmodel=medany" 	    ARCH_ASFLAGS="-march=rv64imafdc -mabi=lp64d"           	    ARCH_CPUDEF="-D__riscv -D__riscv_xlen=64"              	    LD_EMUL=elf64lriscv                                     	    BUILD=$(BUILD)

# ── x86_64 ────────────────────────────────────────────────────────────────────
.PHONY: x86_64
x86_64: _do_x86_64
	@echo "[BSP] x86_64 $(BUILD) build complete → build/x86_64/"

.PHONY: _do_x86_64
_do_x86_64:
	@mkdir -p $(BSP_DIR)/build/x86_64
	@command -v $(X86_CC) >/dev/null 2>&1 || 	    { echo "ERROR: $(X86_CC) not found"; 	      echo "  Linux: sudo apt install gcc-x86-64-linux-gnu"; 	      echo "  macOS: brew install x86_64-elf-gcc"; exit 1; }
	$(MAKE) -f $(BSP_DIR)/Makefile _build                          	    _ARCH=x86_64                                                  	    CC=$(X86_CC) AS=$(X86_CC)                                    	    AR=$(X86_AR) LD=$(X86_LD)                                    	    OBJCOPY=$(X86_OCP) STRIP=$(X86_STR)                          	    ARCH_CFLAGS="-m64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pic -mcmodel=kernel" 	    ARCH_ASFLAGS="-m64"                                           	    ARCH_CPUDEF="-D__x86_64__"                                    	    LD_EMUL=elf_x86_64                                            	    BUILD=$(BUILD)

# =============================================================================
# Internal _build target — invoked recursively with all vars resolved.
# Never call this directly.
# =============================================================================
_ARCH_DIR  = $(BSP_DIR)/10_Arch/$(_ARCH)
_BUILD_DIR = $(BSP_DIR)/build/$(_ARCH)

_ALL_SRCS_C = $(BSP_SRCS_C) $(wildcard $(_ARCH_DIR)/src/*.c) $(SOC_SRCS_C)
_ALL_SRCS_S = $(BSP_SRCS_S) $(wildcard $(_ARCH_DIR)/src/*.S)

_OBJS_C = $(patsubst %.c, $(_BUILD_DIR)/%.o, $(notdir $(_ALL_SRCS_C)))
_OBJS_S = $(patsubst %.S, $(_BUILD_DIR)/%.o, $(notdir $(_ALL_SRCS_S)))
_OBJS   = $(_OBJS_C) $(_OBJS_S)

_CFLAGS_ALL = $(COMMON_CFLAGS_BASE) $(ARCH_CFLAGS) $(ARCH_CPUDEF)               -I$(_ARCH_DIR)/include

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

# ── Object rules ─────────────────────────────────────────────────────────────
$(_BUILD_DIR)/%.o: %.c
	@echo "[CC]  $<"
	$(CC) $(_CFLAGS_ALL) -c $< -o $@

$(_BUILD_DIR)/%.o: %.S
	@echo "[AS]  $<"
	$(AS) $(_CFLAGS_ALL) $(ARCH_ASFLAGS) $(ARCH_CPUDEF) -I$(INC_DIR) -c $< -o $@

# ── Static library ─────────────────────────────────────────────────────────────
$(_BUILD_DIR)/libbsp.a: $(_OBJS)
	@echo "[AR]  $@"
	$(AR) rcs $@ $^

# ── Standalone ELF ────────────────────────────────────────────────────────────
$(_TARGET_ELF): $(_OBJS) $(LDSCRIPT)
	@echo "[LD]  $@"
	$(LD) -T $(LDSCRIPT) -Map=$(_BUILD_DIR)/uiox_bsp.map \
	      --gc-sections $(_OBJS) -o $@

# ── Raw binary ────────────────────────────────────────────────────────────────
$(_TARGET_BIN): $(_TARGET_ELF)
	@echo "[BIN] $@"
	$(OBJCOPY) -O binary --strip-all $< $@

# =============================================================================
# Convenience targets
# =============================================================================

# ── all: build every architecture ────────────────────────────────────────────
.PHONY: all
all: arm64 arm32 riscv64 x86_64
	@echo ""
	@echo "[BSP] all: $(BUILD) build complete for all architectures"

# ── clean per arch ────────────────────────────────────────────────────────────
.PHONY: clean-arm64 clean-arm32 clean-riscv64 clean-x86_64

clean-arm64:
	@echo "[CLN] build/arm64"
	rm -rf $(BSP_DIR)/build/arm64

clean-arm32:
	@echo "[CLN] build/arm32"
	rm -rf $(BSP_DIR)/build/arm32

clean-riscv64:
	@echo "[CLN] build/riscv64"
	rm -rf $(BSP_DIR)/build/riscv64

clean-x86_64:
	@echo "[CLN] build/x86_64"
	rm -rf $(BSP_DIR)/build/x86_64

# ── clean all ────────────────────────────────────────────────────────────────
.PHONY: clean
clean: clean-arm64 clean-arm32 clean-riscv64 clean-x86_64
	@echo "[CLN] all build outputs removed"

# ── info per arch ─────────────────────────────────────────────────────────────
.PHONY: info-arm64 info-arm32 info-riscv64 info-x86_64

info-arm64:
	@echo "HOST     = $(UNAME_S)"
	@echo "ARCH     = arm64"
	@echo "CC       = $(ARM64_CC)"
	@echo "AR       = $(ARM64_AR)"
	@echo "LD       = $(ARM64_LD)"
	@echo "OBJCOPY  = $(ARM64_OCP)"
	@echo "BUILD    = $(BUILD)"
	@echo "OUT      = build/arm64/"

info-arm32:
	@echo "HOST     = $(UNAME_S)"
	@echo "ARCH     = arm32"
	@echo "CC       = $(ARM32_CC)"
	@echo "AR       = $(ARM32_AR)"
	@echo "LD       = $(ARM32_LD)"
	@echo "OBJCOPY  = $(ARM32_OCP)"
	@echo "BUILD    = $(BUILD)"
	@echo "OUT      = build/arm32/"

info-riscv64:
	@echo "HOST     = $(UNAME_S)"
	@echo "ARCH     = riscv64"
	@echo "CC       = $(RV64_CC)"
	@echo "AR       = $(RV64_AR)"
	@echo "LD       = $(RV64_LD)"
	@echo "OBJCOPY  = $(RV64_OCP)"
	@echo "BUILD    = $(BUILD)"
	@echo "OUT      = build/riscv64/"

info-x86_64:
	@echo "HOST     = $(UNAME_S)"
	@echo "ARCH     = x86_64"
	@echo "CC       = $(X86_CC)"
	@echo "AR       = $(X86_AR)"
	@echo "LD       = $(X86_LD)"
	@echo "OBJCOPY  = $(X86_OCP)"
	@echo "BUILD    = $(BUILD)"
	@echo "OUT      = build/x86_64/"
